; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-unknown-unknown -mattr=+sse2 | FileCheck %s --check-prefixes=CHECK,X86
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+sse2 | FileCheck %s --check-prefixes=CHECK,X64

; Only bottom 16 bits are set - upper 48 bits are zero.
define <2 x i64> @combine_psadbw_shift(<16 x i8> %0, <16 x i8> %1) {
; CHECK-LABEL: combine_psadbw_shift:
; CHECK:       # %bb.0:
; CHECK-NEXT:    xorps %xmm0, %xmm0
; CHECK-NEXT:    ret{{[l|q]}}
  %3 = tail call <2 x i64> @llvm.x86.sse2.psad.bw(<16 x i8> %0, <16 x i8> %1)
  %4 = lshr <2 x i64> %3, <i64 48, i64 48>
  ret <2 x i64> %4
}

; Propagate the demanded result elements to the 8 aliasing source elements.
define i64 @combine_psadbw_demandedelt(<16 x i8> %0, <16 x i8> %1) {
; X86-LABEL: combine_psadbw_demandedelt:
; X86:       # %bb.0:
; X86-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[0,1,3,2]
; X86-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[0,1,3,2]
; X86-NEXT:    psadbw %xmm0, %xmm1
; X86-NEXT:    movd %xmm1, %eax
; X86-NEXT:    pshufd {{.*#+}} xmm0 = xmm1[1,1,2,3]
; X86-NEXT:    movd %xmm0, %edx
; X86-NEXT:    retl
;
; X64-LABEL: combine_psadbw_demandedelt:
; X64:       # %bb.0:
; X64-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[0,1,3,2]
; X64-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[0,1,3,2]
; X64-NEXT:    psadbw %xmm0, %xmm1
; X64-NEXT:    movq %xmm1, %rax
; X64-NEXT:    retq
  %3 = shufflevector <16 x i8> %0, <16 x i8> %0, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 12, i32 13, i32 14, i32 15, i32 8, i32 9, i32 10, i32 11>
  %4 = shufflevector <16 x i8> %1, <16 x i8> %1, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 12, i32 13, i32 14, i32 15, i32 8, i32 9, i32 10, i32 11>
  %5 = tail call <2 x i64> @llvm.x86.sse2.psad.bw(<16 x i8> %3, <16 x i8> %4)
  %6 = extractelement <2 x i64> %5, i32 0
  ret i64 %6
}

declare <2 x i64> @llvm.x86.sse2.psad.bw(<16 x i8>, <16 x i8>)

