; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -passes=slp-vectorizer -mtriple=x86_64-- -mcpu=x86-64    -S | FileCheck %s --check-prefixes=SSE2
; RUN: opt < %s -passes=slp-vectorizer -mtriple=x86_64-- -mcpu=x86-64-v2 -S | FileCheck %s --check-prefixes=SSE42
; RUN: opt < %s -passes=slp-vectorizer -mtriple=x86_64-- -mcpu=x86-64-v3 -S | FileCheck %s --check-prefixes=AVX2
; RUN: opt < %s -passes=slp-vectorizer -mtriple=x86_64-- -mcpu=x86-64-v4 -S | FileCheck %s --check-prefixes=AVX512

; PR51746
; typedef int v4si __attribute__ ((vector_size (16)));
;
; inline int reduce_and4(int acc, v4si v1, v4si v2, v4si v3, v4si v4) {
;   acc &= v1[0] & v1[1] & v1[2] & v1[3];
;   acc &= v2[0] & v2[1] & v2[2] & v2[3];
;   acc &= v3[0] & v3[1] & v3[2] & v3[3];
;   acc &= v4[0] & v4[1] & v4[2] & v4[3];
;   return acc;
; }

define i32 @reduce_and4(i32 %acc, <4 x i32> %v1, <4 x i32> %v2, <4 x i32> %v3, <4 x i32> %v4) {
; SSE2-LABEL: @reduce_and4(
; SSE2-NEXT:  entry:
; SSE2-NEXT:    [[TMP0:%.*]] = shufflevector <4 x i32> [[V4:%.*]], <4 x i32> [[V3:%.*]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE2-NEXT:    [[TMP1:%.*]] = shufflevector <4 x i32> [[V2:%.*]], <4 x i32> poison, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE2-NEXT:    [[TMP2:%.*]] = shufflevector <16 x i32> [[TMP0]], <16 x i32> [[TMP1]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 16, i32 17, i32 18, i32 19, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE2-NEXT:    [[TMP3:%.*]] = shufflevector <4 x i32> [[V1:%.*]], <4 x i32> poison, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE2-NEXT:    [[TMP4:%.*]] = shufflevector <16 x i32> [[TMP2]], <16 x i32> [[TMP3]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 16, i32 17, i32 18, i32 19>
; SSE2-NEXT:    [[TMP5:%.*]] = call i32 @llvm.vector.reduce.and.v16i32(<16 x i32> [[TMP4]])
; SSE2-NEXT:    [[OP_RDX:%.*]] = and i32 [[TMP5]], [[ACC:%.*]]
; SSE2-NEXT:    ret i32 [[OP_RDX]]
;
; SSE42-LABEL: @reduce_and4(
; SSE42-NEXT:  entry:
; SSE42-NEXT:    [[TMP0:%.*]] = shufflevector <4 x i32> [[V4:%.*]], <4 x i32> [[V3:%.*]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE42-NEXT:    [[TMP1:%.*]] = shufflevector <4 x i32> [[V2:%.*]], <4 x i32> poison, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE42-NEXT:    [[TMP2:%.*]] = shufflevector <16 x i32> [[TMP0]], <16 x i32> [[TMP1]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 16, i32 17, i32 18, i32 19, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE42-NEXT:    [[TMP3:%.*]] = shufflevector <4 x i32> [[V1:%.*]], <4 x i32> poison, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE42-NEXT:    [[TMP4:%.*]] = shufflevector <16 x i32> [[TMP2]], <16 x i32> [[TMP3]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 16, i32 17, i32 18, i32 19>
; SSE42-NEXT:    [[TMP5:%.*]] = call i32 @llvm.vector.reduce.and.v16i32(<16 x i32> [[TMP4]])
; SSE42-NEXT:    [[OP_RDX:%.*]] = and i32 [[TMP5]], [[ACC:%.*]]
; SSE42-NEXT:    ret i32 [[OP_RDX]]
;
; AVX2-LABEL: @reduce_and4(
; AVX2-NEXT:  entry:
; AVX2-NEXT:    [[TMP0:%.*]] = shufflevector <4 x i32> [[V4:%.*]], <4 x i32> [[V3:%.*]], <8 x i32> <i32 1, i32 0, i32 2, i32 3, i32 5, i32 4, i32 6, i32 7>
; AVX2-NEXT:    [[TMP1:%.*]] = shufflevector <4 x i32> [[V2:%.*]], <4 x i32> [[V1:%.*]], <8 x i32> <i32 1, i32 0, i32 2, i32 3, i32 5, i32 4, i32 6, i32 7>
; AVX2-NEXT:    [[TMP2:%.*]] = shufflevector <8 x i32> [[TMP0]], <8 x i32> [[TMP1]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; AVX2-NEXT:    [[TMP3:%.*]] = call i32 @llvm.vector.reduce.and.v16i32(<16 x i32> [[TMP2]])
; AVX2-NEXT:    [[OP_RDX:%.*]] = and i32 [[TMP3]], [[ACC:%.*]]
; AVX2-NEXT:    ret i32 [[OP_RDX]]
;
; AVX512-LABEL: @reduce_and4(
; AVX512-NEXT:  entry:
; AVX512-NEXT:    [[TMP0:%.*]] = shufflevector <4 x i32> [[V4:%.*]], <4 x i32> [[V3:%.*]], <8 x i32> <i32 1, i32 0, i32 2, i32 3, i32 5, i32 4, i32 6, i32 7>
; AVX512-NEXT:    [[TMP1:%.*]] = shufflevector <4 x i32> [[V2:%.*]], <4 x i32> [[V1:%.*]], <8 x i32> <i32 1, i32 0, i32 2, i32 3, i32 5, i32 4, i32 6, i32 7>
; AVX512-NEXT:    [[RDX_OP:%.*]] = and <8 x i32> [[TMP0]], [[TMP1]]
; AVX512-NEXT:    [[OP_RDX:%.*]] = call i32 @llvm.vector.reduce.and.v8i32(<8 x i32> [[RDX_OP]])
; AVX512-NEXT:    [[OP_RDX1:%.*]] = and i32 [[OP_RDX]], [[ACC:%.*]]
; AVX512-NEXT:    ret i32 [[OP_RDX1]]
;
entry:
  %vecext = extractelement <4 x i32> %v1, i64 0
  %vecext1 = extractelement <4 x i32> %v1, i64 1
  %vecext2 = extractelement <4 x i32> %v1, i64 2
  %vecext4 = extractelement <4 x i32> %v1, i64 3
  %vecext7 = extractelement <4 x i32> %v2, i64 0
  %vecext8 = extractelement <4 x i32> %v2, i64 1
  %vecext10 = extractelement <4 x i32> %v2, i64 2
  %vecext12 = extractelement <4 x i32> %v2, i64 3
  %vecext15 = extractelement <4 x i32> %v3, i64 0
  %vecext16 = extractelement <4 x i32> %v3, i64 1
  %vecext18 = extractelement <4 x i32> %v3, i64 2
  %vecext20 = extractelement <4 x i32> %v3, i64 3
  %vecext23 = extractelement <4 x i32> %v4, i64 0
  %vecext24 = extractelement <4 x i32> %v4, i64 1
  %vecext26 = extractelement <4 x i32> %v4, i64 2
  %vecext28 = extractelement <4 x i32> %v4, i64 3
  %and25 = and i32 %vecext1, %acc
  %and27 = and i32 %and25, %vecext
  %and29 = and i32 %and27, %vecext2
  %and17 = and i32 %and29, %vecext4
  %and19 = and i32 %and17, %vecext8
  %and21 = and i32 %and19, %vecext7
  %and9 = and i32 %and21, %vecext10
  %and11 = and i32 %and9, %vecext12
  %and13 = and i32 %and11, %vecext16
  %and = and i32 %and13, %vecext15
  %and3 = and i32 %and, %vecext18
  %and5 = and i32 %and3, %vecext20
  %and6 = and i32 %and5, %vecext24
  %and14 = and i32 %and6, %vecext23
  %and22 = and i32 %and14, %vecext26
  %and30 = and i32 %and22, %vecext28
  ret i32 %and30
}

; int reduce_and4_transpose(int acc, v4si v1, v4si v2, v4si v3, v4si v4) {
;   acc &= v1[0] & v2[0] & v3[0] & v4[0];
;   acc &= v1[1] & v2[1] & v3[1] & v4[1];
;   acc &= v1[2] & v2[2] & v3[2] & v4[2];
;   acc &= v1[3] & v2[3] & v3[3] & v4[3];
;   return acc;
; }

define i32 @reduce_and4_transpose(i32 %acc, <4 x i32> %v1, <4 x i32> %v2, <4 x i32> %v3, <4 x i32> %v4) {
; SSE2-LABEL: @reduce_and4_transpose(
; SSE2-NEXT:    [[TMP1:%.*]] = shufflevector <4 x i32> [[V4:%.*]], <4 x i32> [[V3:%.*]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE2-NEXT:    [[TMP2:%.*]] = shufflevector <4 x i32> [[V2:%.*]], <4 x i32> poison, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE2-NEXT:    [[TMP3:%.*]] = shufflevector <16 x i32> [[TMP1]], <16 x i32> [[TMP2]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 16, i32 17, i32 18, i32 19, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE2-NEXT:    [[TMP4:%.*]] = shufflevector <4 x i32> [[V1:%.*]], <4 x i32> poison, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE2-NEXT:    [[TMP5:%.*]] = shufflevector <16 x i32> [[TMP3]], <16 x i32> [[TMP4]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 16, i32 17, i32 18, i32 19>
; SSE2-NEXT:    [[TMP6:%.*]] = call i32 @llvm.vector.reduce.and.v16i32(<16 x i32> [[TMP5]])
; SSE2-NEXT:    [[OP_RDX:%.*]] = and i32 [[TMP6]], [[ACC:%.*]]
; SSE2-NEXT:    ret i32 [[OP_RDX]]
;
; SSE42-LABEL: @reduce_and4_transpose(
; SSE42-NEXT:    [[TMP1:%.*]] = shufflevector <4 x i32> [[V4:%.*]], <4 x i32> [[V3:%.*]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE42-NEXT:    [[TMP2:%.*]] = shufflevector <4 x i32> [[V2:%.*]], <4 x i32> poison, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE42-NEXT:    [[TMP3:%.*]] = shufflevector <16 x i32> [[TMP1]], <16 x i32> [[TMP2]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 16, i32 17, i32 18, i32 19, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE42-NEXT:    [[TMP4:%.*]] = shufflevector <4 x i32> [[V1:%.*]], <4 x i32> poison, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
; SSE42-NEXT:    [[TMP5:%.*]] = shufflevector <16 x i32> [[TMP3]], <16 x i32> [[TMP4]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 16, i32 17, i32 18, i32 19>
; SSE42-NEXT:    [[TMP6:%.*]] = call i32 @llvm.vector.reduce.and.v16i32(<16 x i32> [[TMP5]])
; SSE42-NEXT:    [[OP_RDX:%.*]] = and i32 [[TMP6]], [[ACC:%.*]]
; SSE42-NEXT:    ret i32 [[OP_RDX]]
;
; AVX2-LABEL: @reduce_and4_transpose(
; AVX2-NEXT:    [[TMP1:%.*]] = shufflevector <4 x i32> [[V4:%.*]], <4 x i32> [[V3:%.*]], <8 x i32> <i32 3, i32 2, i32 1, i32 0, i32 7, i32 6, i32 5, i32 4>
; AVX2-NEXT:    [[TMP2:%.*]] = shufflevector <4 x i32> [[V2:%.*]], <4 x i32> [[V1:%.*]], <8 x i32> <i32 3, i32 2, i32 1, i32 0, i32 7, i32 6, i32 5, i32 4>
; AVX2-NEXT:    [[TMP3:%.*]] = shufflevector <8 x i32> [[TMP1]], <8 x i32> [[TMP2]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; AVX2-NEXT:    [[TMP4:%.*]] = call i32 @llvm.vector.reduce.and.v16i32(<16 x i32> [[TMP3]])
; AVX2-NEXT:    [[OP_RDX:%.*]] = and i32 [[TMP4]], [[ACC:%.*]]
; AVX2-NEXT:    ret i32 [[OP_RDX]]
;
; AVX512-LABEL: @reduce_and4_transpose(
; AVX512-NEXT:    [[TMP1:%.*]] = shufflevector <4 x i32> [[V4:%.*]], <4 x i32> [[V3:%.*]], <8 x i32> <i32 3, i32 2, i32 1, i32 0, i32 7, i32 6, i32 5, i32 4>
; AVX512-NEXT:    [[TMP2:%.*]] = shufflevector <4 x i32> [[V2:%.*]], <4 x i32> [[V1:%.*]], <8 x i32> <i32 3, i32 2, i32 1, i32 0, i32 7, i32 6, i32 5, i32 4>
; AVX512-NEXT:    [[RDX_OP:%.*]] = and <8 x i32> [[TMP1]], [[TMP2]]
; AVX512-NEXT:    [[OP_RDX:%.*]] = call i32 @llvm.vector.reduce.and.v8i32(<8 x i32> [[RDX_OP]])
; AVX512-NEXT:    [[OP_RDX1:%.*]] = and i32 [[OP_RDX]], [[ACC:%.*]]
; AVX512-NEXT:    ret i32 [[OP_RDX1]]
;
  %vecext = extractelement <4 x i32> %v1, i64 0
  %vecext1 = extractelement <4 x i32> %v2, i64 0
  %vecext2 = extractelement <4 x i32> %v3, i64 0
  %vecext4 = extractelement <4 x i32> %v4, i64 0
  %vecext7 = extractelement <4 x i32> %v1, i64 1
  %vecext8 = extractelement <4 x i32> %v2, i64 1
  %vecext10 = extractelement <4 x i32> %v3, i64 1
  %vecext12 = extractelement <4 x i32> %v4, i64 1
  %vecext15 = extractelement <4 x i32> %v1, i64 2
  %vecext16 = extractelement <4 x i32> %v2, i64 2
  %vecext18 = extractelement <4 x i32> %v3, i64 2
  %vecext20 = extractelement <4 x i32> %v4, i64 2
  %vecext23 = extractelement <4 x i32> %v1, i64 3
  %vecext24 = extractelement <4 x i32> %v2, i64 3
  %vecext26 = extractelement <4 x i32> %v3, i64 3
  %vecext28 = extractelement <4 x i32> %v4, i64 3
  %and = and i32 %vecext23, %acc
  %and3 = and i32 %and, %vecext15
  %and5 = and i32 %and3, %vecext7
  %and6 = and i32 %and5, %vecext
  %and9 = and i32 %and6, %vecext24
  %and11 = and i32 %and9, %vecext16
  %and13 = and i32 %and11, %vecext8
  %and14 = and i32 %and13, %vecext1
  %and17 = and i32 %and14, %vecext26
  %and19 = and i32 %and17, %vecext18
  %and21 = and i32 %and19, %vecext10
  %and22 = and i32 %and21, %vecext2
  %and25 = and i32 %and22, %vecext28
  %and27 = and i32 %and25, %vecext20
  %and29 = and i32 %and27, %vecext12
  %and30 = and i32 %and29, %vecext4
  ret i32 %and30
}
