; NOTE: Assertions have been autogenerated by utils/update_test_checks.py UTC_ARGS: --version 5
; RUN: opt -S --passes=slp-vectorizer -mtriple=x86_64-unknown-linux-gnu -mcpu=cascadelake < %s | FileCheck %s

define void @test(ptr %0, i1 %1, i1 %2) {
; CHECK-LABEL: define void @test(
; CHECK-SAME: ptr [[TMP0:%.*]], i1 [[TMP1:%.*]], i1 [[TMP2:%.*]]) #[[ATTR0:[0-9]+]] {
; CHECK-NEXT:    br label %[[BB4:.*]]
; CHECK:       [[BB4]]:
; CHECK-NEXT:    [[TMP5:%.*]] = phi <2 x i32> [ [[TMP12:%.*]], %[[TMP7:.*]] ], [ zeroinitializer, [[TMP3:%.*]] ]
; CHECK-NEXT:    [[TMP6:%.*]] = shufflevector <2 x i32> [[TMP5]], <2 x i32> poison, <4 x i32> <i32 0, i32 0, i32 0, i32 1>
; CHECK-NEXT:    br i1 [[TMP1]], label %[[TMP7]], label %[[BB15:.*]]
; CHECK:       [[TMP7]]:
; CHECK-NEXT:    [[TMP8:%.*]] = load ptr, ptr [[TMP0]], align 8
; CHECK-NEXT:    [[TMP9:%.*]] = getelementptr i8, ptr [[TMP8]], i64 16
; CHECK-NEXT:    [[TMP10:%.*]] = load <2 x i32>, ptr [[TMP9]], align 1
; CHECK-NEXT:    [[TMP11:%.*]] = or <2 x i32> [[TMP10]], splat (i32 1)
; CHECK-NEXT:    [[TMP12]] = shufflevector <2 x i32> [[TMP11]], <2 x i32> <i32 1, i32 poison>, <2 x i32> <i32 2, i32 1>
; CHECK-NEXT:    [[TMP13:%.*]] = shufflevector <2 x i32> [[TMP11]], <2 x i32> poison, <4 x i32> <i32 0, i32 1, i32 poison, i32 poison>
; CHECK-NEXT:    [[TMP14:%.*]] = shufflevector <4 x i32> <i32 0, i32 0, i32 poison, i32 poison>, <4 x i32> [[TMP13]], <4 x i32> <i32 0, i32 1, i32 4, i32 5>
; CHECK-NEXT:    br i1 [[TMP2]], label %[[BB16:.*]], label %[[BB4]]
; CHECK:       [[BB15]]:
; CHECK-NEXT:    br label %[[BB16]]
; CHECK:       [[BB16]]:
; CHECK-NEXT:    [[TMP16:%.*]] = phi <4 x i32> [ [[TMP6]], %[[BB15]] ], [ [[TMP14]], %[[TMP7]] ]
; CHECK-NEXT:    [[TMP17:%.*]] = load volatile ptr, ptr null, align 8
; CHECK-NEXT:    [[TMP18:%.*]] = getelementptr i8, ptr [[TMP17]], i64 176
; CHECK-NEXT:    store <4 x i32> [[TMP16]], ptr [[TMP18]], align 8
; CHECK-NEXT:    ret void
;
  br label %4

4:
  %5 = phi i32 [ %14, %7 ], [ 0, %3 ]
  %6 = phi i32 [ 1, %7 ], [ 0, %3 ]
  br i1 %1, label %7, label %15

7:
  %8 = load ptr, ptr %0, align 8
  %9 = getelementptr i8, ptr %8, i64 16
  %10 = load i32, ptr %9, align 1
  %11 = or i32 %10, 1
  %12 = getelementptr i8, ptr %8, i64 20
  %13 = load i32, ptr %12, align 1
  %14 = or i32 %13, 1
  br i1 %2, label %16, label %4

15:
  br label %16

16:
  %17 = phi i32 [ %6, %15 ], [ 0, %7 ]
  %18 = phi i32 [ %6, %15 ], [ %11, %7 ]
  %19 = phi i32 [ %5, %15 ], [ %14, %7 ]
  %20 = load volatile ptr, ptr null, align 8
  %21 = getelementptr i8, ptr %20, i64 176
  store i32 %17, ptr %21, align 8
  %22 = getelementptr i8, ptr %20, i64 180
  store i32 %17, ptr %22, align 4
  %23 = getelementptr i8, ptr %20, i64 184
  store i32 %18, ptr %23, align 8
  %24 = getelementptr i8, ptr %20, i64 188
  store i32 %19, ptr %24, align 4
  ret void
}
