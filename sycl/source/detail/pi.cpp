//===-- pi.cpp - PI utilities implementation -------------------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <CL/sycl/detail/common.hpp>
#include <CL/sycl/detail/pi.hpp>
#include <cstdarg>
#include <iostream>
#include <map>

namespace cl {
namespace sycl {
namespace detail {
namespace pi {

// Check for manually selected BE at run-time.
bool piUseBackend(PiBackend Backend) {
  static const char *GetEnv = std::getenv("SYCL_BE");
  static const PiBackend Use =
    std::map<std::string, PiBackend>{
      { "PI_OPENCL", SYCL_BE_PI_OPENCL },
      { "PI_OTHER",  SYCL_BE_PI_OTHER }
      // Any other value would yield PI_OPENCL (current default)
    }[ GetEnv ? GetEnv : "PI_OPENCL"];
  return Backend == Use;
}

// Definitions of the PI dispatch entries, they will be initialized
// at their first use with piInitialize.
#define _PI_API(api) decltype(::api) * api = nullptr;
#include <CL/sycl/detail/pi.def>

// TODO: implement real plugins (ICD-like?)
// For now this has the effect of redirecting to built-in PI OpenCL plugin.
void piInitialize() {
  static bool Initialized = false;
  if (Initialized) {
    return;
  }
  if (!piUseBackend(SYCL_BE_PI_OPENCL)) {
    piDie("Unknown SYCL_BE");
  }
  #define _PI_API(api)                          \
    extern const decltype(::api) * api##OclPtr; \
    api = api##OclPtr;
  #include <CL/sycl/detail/pi.def>

  Initialized = true;
}

// Report error and no return (keeps compiler from printing warnings).
// TODO: Probably change that to throw a catchable exception,
//       but for now it is useful to see every failure.
//
[[noreturn]] void piDie(const char *Message) {
  std::cerr << "pi_die: " << Message << std::endl;
  std::terminate();
}

void piAssert(bool Condition, const char *Message) {
  if (!Condition)
    piDie(Message);
}

bool PiCall::m_TraceEnabled = (std::getenv("SYCL_PI_TRACE") != nullptr);

// Emits trace before the start of PI call
PiCall::PiCall(const char *Trace) {
  if (m_TraceEnabled && Trace) {
    std::cerr << "PI ---> " << Trace << std::endl;
  }
}
// Emits trace after the end of PI call
PiCall::~PiCall() {
  if (m_TraceEnabled) {
    std::cerr << "PI <--- " << m_Result << std::endl;
  }
}
// Records and returns the result of PI call
RT::PiResult PiCall::get(RT::PiResult Result) {
  m_Result = Result;
  return Result;
}
template<typename Exception>
void PiCall::check(RT::PiResult Result) {
  m_Result = Result;
  // TODO: remove dependency on CHECK_OCL_CODE_THROW.
  CHECK_OCL_CODE_THROW(Result, Exception);
}

template void PiCall::check<cl::sycl::runtime_error>(RT::PiResult);
template void PiCall::check<cl::sycl::compile_program_error>(RT::PiResult);


} // namespace pi
} // namespace detail
} // namespace sycl
} // namespace cl
