//===--- RangeSelector.cpp - RangeSelector implementations ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Tooling/Refactoring/RangeSelector.h"
#include "clang/AST/Expr.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/Refactoring/SourceCode.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/Error.h"
#include <string>
#include <utility>
#include <vector>

using namespace clang;
using namespace tooling;

using ast_matchers::MatchFinder;
using ast_type_traits::ASTNodeKind;
using ast_type_traits::DynTypedNode;
using llvm::Error;
using llvm::StringError;

using MatchResult = MatchFinder::MatchResult;

static Error invalidArgumentError(Twine Message) {
  return llvm::make_error<StringError>(llvm::errc::invalid_argument, Message);
}

static Error typeError(StringRef ID, const ASTNodeKind &Kind) {
  return invalidArgumentError("mismatched type (node id=" + ID +
                              " kind=" + Kind.asStringRef() + ")");
}

static Error typeError(StringRef ID, const ASTNodeKind &Kind,
                       Twine ExpectedType) {
  return invalidArgumentError("mismatched type: expected one of " +
                              ExpectedType + " (node id=" + ID +
                              " kind=" + Kind.asStringRef() + ")");
}

static Error missingPropertyError(StringRef ID, Twine Description,
                                  StringRef Property) {
  return invalidArgumentError(Description + " requires property '" + Property +
                              "' (node id=" + ID + ")");
}

static Expected<DynTypedNode> getNode(const ast_matchers::BoundNodes &Nodes,
                                      StringRef ID) {
  auto &NodesMap = Nodes.getMap();
  auto It = NodesMap.find(ID);
  if (It == NodesMap.end())
    return invalidArgumentError("ID not bound: " + ID);
  return It->second;
}

// FIXME: handling of macros should be configurable.
static SourceLocation findPreviousTokenStart(SourceLocation Start,
                                             const SourceManager &SM,
                                             const LangOptions &LangOpts) {
  if (Start.isInvalid() || Start.isMacroID())
    return SourceLocation();

  SourceLocation BeforeStart = Start.getLocWithOffset(-1);
  if (BeforeStart.isInvalid() || BeforeStart.isMacroID())
    return SourceLocation();

  return Lexer::GetBeginningOfToken(BeforeStart, SM, LangOpts);
}

// Finds the start location of the previous token of kind \p TK.
// FIXME: handling of macros should be configurable.
static SourceLocation findPreviousTokenKind(SourceLocation Start,
                                            const SourceManager &SM,
                                            const LangOptions &LangOpts,
                                            tok::TokenKind TK) {
  while (true) {
    SourceLocation L = findPreviousTokenStart(Start, SM, LangOpts);
    if (L.isInvalid() || L.isMacroID())
      return SourceLocation();

    Token T;
    if (Lexer::getRawToken(L, T, SM, LangOpts, /*IgnoreWhiteSpace=*/true))
      return SourceLocation();

    if (T.is(TK))
      return T.getLocation();

    Start = L;
  }
}

static SourceLocation findOpenParen(const CallExpr &E, const SourceManager &SM,
                                    const LangOptions &LangOpts) {
  SourceLocation EndLoc =
      E.getNumArgs() == 0 ? E.getRParenLoc() : E.getArg(0)->getBeginLoc();
  return findPreviousTokenKind(EndLoc, SM, LangOpts, tok::TokenKind::l_paren);
}

RangeSelector tooling::node(StringRef ID) {
  return [ID](const MatchResult &Result) -> Expected<CharSourceRange> {
    Expected<DynTypedNode> Node = getNode(Result.Nodes, ID);
    if (!Node)
      return Node.takeError();
    return Node->get<Stmt>() != nullptr && Node->get<Expr>() == nullptr
               ? getExtendedRange(*Node, tok::TokenKind::semi, *Result.Context)
               : CharSourceRange::getTokenRange(Node->getSourceRange());
  };
}

RangeSelector tooling::statement(StringRef ID) {
  return [ID](const MatchResult &Result) -> Expected<CharSourceRange> {
    Expected<DynTypedNode> Node = getNode(Result.Nodes, ID);
    if (!Node)
      return Node.takeError();
    return getExtendedRange(*Node, tok::TokenKind::semi, *Result.Context);
  };
}

RangeSelector tooling::range(RangeSelector Begin, RangeSelector End) {
  return [Begin, End](const MatchResult &Result) -> Expected<CharSourceRange> {
    Expected<CharSourceRange> BeginRange = Begin(Result);
    if (!BeginRange)
      return BeginRange.takeError();
    Expected<CharSourceRange> EndRange = End(Result);
    if (!EndRange)
      return EndRange.takeError();
    SourceLocation B = BeginRange->getBegin();
    SourceLocation E = EndRange->getEnd();
    // Note: we are precluding the possibility of sub-token ranges in the case
    // that EndRange is a token range.
    if (Result.SourceManager->isBeforeInTranslationUnit(E, B)) {
      return invalidArgumentError("Bad range: out of order");
    }
    return CharSourceRange(SourceRange(B, E), EndRange->isTokenRange());
  };
}

RangeSelector tooling::range(StringRef BeginID, StringRef EndID) {
  return tooling::range(node(BeginID), node(EndID));
}

RangeSelector tooling::member(StringRef ID) {
  return [ID](const MatchResult &Result) -> Expected<CharSourceRange> {
    Expected<DynTypedNode> Node = getNode(Result.Nodes, ID);
    if (!Node)
      return Node.takeError();
    if (auto *M = Node->get<clang::MemberExpr>())
      return CharSourceRange::getTokenRange(
          M->getMemberNameInfo().getSourceRange());
    return typeError(ID, Node->getNodeKind(), "MemberExpr");
  };
}

RangeSelector tooling::name(StringRef ID) {
  return [ID](const MatchResult &Result) -> Expected<CharSourceRange> {
    Expected<DynTypedNode> N = getNode(Result.Nodes, ID);
    if (!N)
      return N.takeError();
    auto &Node = *N;
    if (const auto *D = Node.get<NamedDecl>()) {
      if (!D->getDeclName().isIdentifier())
        return missingPropertyError(ID, "name", "identifier");
      SourceLocation L = D->getLocation();
      auto R = CharSourceRange::getTokenRange(L, L);
      // Verify that the range covers exactly the name.
      // FIXME: extend this code to support cases like `operator +` or
      // `foo<int>` for which this range will be too short.  Doing so will
      // require subcasing `NamedDecl`, because it doesn't provide virtual
      // access to the \c DeclarationNameInfo.
      if (getText(R, *Result.Context) != D->getName())
        return CharSourceRange();
      return R;
    }
    if (const auto *E = Node.get<DeclRefExpr>()) {
      if (!E->getNameInfo().getName().isIdentifier())
        return missingPropertyError(ID, "name", "identifier");
      SourceLocation L = E->getLocation();
      return CharSourceRange::getTokenRange(L, L);
    }
    if (const auto *I = Node.get<CXXCtorInitializer>()) {
      if (!I->isMemberInitializer() && I->isWritten())
        return missingPropertyError(ID, "name", "explicit member initializer");
      SourceLocation L = I->getMemberLocation();
      return CharSourceRange::getTokenRange(L, L);
    }
    return typeError(ID, Node.getNodeKind(),
                     "DeclRefExpr, NamedDecl, CXXCtorInitializer");
  };
}

namespace {
// Creates a selector from a range-selection function \p Func, which selects a
// range that is relative to a bound node id.  \c T is the node type expected by
// \p Func.
template <typename T, CharSourceRange (*Func)(const MatchResult &, const T &)>
class RelativeSelector {
  std::string ID;

public:
  RelativeSelector(StringRef ID) : ID(ID) {}

  Expected<CharSourceRange> operator()(const MatchResult &Result) {
    Expected<DynTypedNode> N = getNode(Result.Nodes, ID);
    if (!N)
      return N.takeError();
    if (const auto *Arg = N->get<T>())
      return Func(Result, *Arg);
    return typeError(ID, N->getNodeKind());
  }
};
} // namespace

// Returns the range of the statements (all source between the braces).
static CharSourceRange getStatementsRange(const MatchResult &,
                                          const CompoundStmt &CS) {
  return CharSourceRange::getCharRange(CS.getLBracLoc().getLocWithOffset(1),
                                       CS.getRBracLoc());
}

RangeSelector tooling::statements(StringRef ID) {
  return RelativeSelector<CompoundStmt, getStatementsRange>(ID);
}

// Returns the range of the source between the call's parentheses.
static CharSourceRange getCallArgumentsRange(const MatchResult &Result,
                                             const CallExpr &CE) {
  return CharSourceRange::getCharRange(
      findOpenParen(CE, *Result.SourceManager, Result.Context->getLangOpts())
          .getLocWithOffset(1),
      CE.getRParenLoc());
}

RangeSelector tooling::callArgs(StringRef ID) {
  return RelativeSelector<CallExpr, getCallArgumentsRange>(ID);
}

// Returns the range of the elements of the initializer list. Includes all
// source between the braces.
static CharSourceRange getElementsRange(const MatchResult &,
                                        const InitListExpr &E) {
  return CharSourceRange::getCharRange(E.getLBraceLoc().getLocWithOffset(1),
                                       E.getRBraceLoc());
}

RangeSelector tooling::initListElements(StringRef ID) {
  return RelativeSelector<InitListExpr, getElementsRange>(ID);
}

RangeSelector tooling::expansion(RangeSelector S) {
  return [S](const MatchResult &Result) -> Expected<CharSourceRange> {
    Expected<CharSourceRange> SRange = S(Result);
    if (!SRange)
      return SRange.takeError();
    return Result.SourceManager->getExpansionRange(*SRange);
  };
}
