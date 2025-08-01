; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -passes=slp-vectorizer -S < %s -mtriple=riscv64-unknown-linux-gnu -mcpu=sifive-p670 -pass-remarks-output=%t| FileCheck %s
; RUN: FileCheck --input-file=%t %s --check-prefix=YAML

; YAML-LABEL: --- !Passed
; YAML-NEXT: Pass:            slp-vectorizer
; YAML-NEXT: Name:            VectorizedHorizontalReduction
; YAML-NEXT: Function:        test
; YAML-NEXT: Args:
; YAML-NEXT:   - String:          'Vectorized horizontal reduction with cost '
; YAML-NEXT:   - Cost:            '-35'
; YAML-NEXT:   - String:          ' and with tree size '
; YAML-NEXT:   - TreeSize:        '1'
; YAML-NEXT: ...
; YAML-NEXT: --- !Passed
; YAML-NEXT: Pass:            slp-vectorizer
; YAML-NEXT: Name:            VectorizedHorizontalReduction
; YAML-NEXT: Function:        test
; YAML-NEXT: Args:
; YAML-NEXT:   - String:          'Vectorized horizontal reduction with cost '
; YAML-NEXT:   - Cost:            '-15'
; YAML-NEXT:   - String:          ' and with tree size '
; YAML-NEXT:   - TreeSize:        '1'
; YAML-NEXT: ...
; YAML-NEXT: --- !Passed
; YAML-NEXT: Pass:            slp-vectorizer
; YAML-NEXT: Name:            VectorizedHorizontalReduction
; YAML-NEXT: Function:        test
; YAML-NEXT: Args:
; YAML-NEXT:   - String:          'Vectorized horizontal reduction with cost '
; YAML-NEXT:   - Cost:            '-6'
; YAML-NEXT:   - String:          ' and with tree size '
; YAML-NEXT:   - TreeSize:        '1'
; YAML-NEXT:...
define float @test(ptr %x) {
; CHECK-LABEL: @test(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[ARRAYIDX:%.*]] = getelementptr inbounds float, ptr [[X:%.*]], i64 1
; CHECK-NEXT:    [[TMP0:%.*]] = load <16 x float>, ptr [[ARRAYIDX]], align 4
; CHECK-NEXT:    [[ARRAYIDX_16:%.*]] = getelementptr inbounds float, ptr [[X]], i64 17
; CHECK-NEXT:    [[TMP1:%.*]] = load <8 x float>, ptr [[ARRAYIDX_16]], align 4
; CHECK-NEXT:    [[ARRAYIDX_24:%.*]] = getelementptr inbounds float, ptr [[X]], i64 25
; CHECK-NEXT:    [[TMP2:%.*]] = load <4 x float>, ptr [[ARRAYIDX_24]], align 4
; CHECK-NEXT:    [[ARRAYIDX_28:%.*]] = getelementptr inbounds float, ptr [[X]], i64 29
; CHECK-NEXT:    [[TMP3:%.*]] = load float, ptr [[ARRAYIDX_28]], align 4
; CHECK-NEXT:    [[ARRAYIDX_29:%.*]] = getelementptr inbounds float, ptr [[X]], i64 30
; CHECK-NEXT:    [[TMP4:%.*]] = load float, ptr [[ARRAYIDX_29]], align 4
; CHECK-NEXT:    [[TMP5:%.*]] = shufflevector <16 x float> [[TMP0]], <16 x float> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK-NEXT:    [[RDX_OP:%.*]] = fadd fast <8 x float> [[TMP5]], [[TMP1]]
; CHECK-NEXT:    [[TMP6:%.*]] = shufflevector <8 x float> [[RDX_OP]], <8 x float> poison, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
; CHECK-NEXT:    [[TMP7:%.*]] = shufflevector <16 x float> [[TMP0]], <16 x float> [[TMP6]], <16 x i32> <i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK-NEXT:    [[RDX_OP4:%.*]] = shufflevector <16 x float> [[TMP7]], <16 x float> poison, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; CHECK-NEXT:    [[RDX_OP5:%.*]] = fadd fast <4 x float> [[RDX_OP4]], [[TMP2]]
; CHECK-NEXT:    [[TMP9:%.*]] = shufflevector <4 x float> [[RDX_OP5]], <4 x float> poison, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
; CHECK-NEXT:    [[TMP8:%.*]] = shufflevector <16 x float> [[TMP7]], <16 x float> [[TMP9]], <16 x i32> <i32 16, i32 17, i32 18, i32 19, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK-NEXT:    [[OP_RDX1:%.*]] = call fast float @llvm.vector.reduce.fadd.v16f32(float 0.000000e+00, <16 x float> [[TMP8]])
; CHECK-NEXT:    [[OP_RDX2:%.*]] = fadd fast float [[OP_RDX1]], [[TMP3]]
; CHECK-NEXT:    [[OP_RDX3:%.*]] = fadd fast float [[OP_RDX2]], [[TMP4]]
; CHECK-NEXT:    ret float [[OP_RDX3]]
;
  entry:
  %arrayidx = getelementptr inbounds float, ptr %x, i64 1
  %0 = load float, ptr %arrayidx, align 4
  %arrayidx.1 = getelementptr inbounds float, ptr %x, i64 2
  %1 = load float, ptr %arrayidx.1, align 4
  %add.1 = fadd fast float %1, %0
  %arrayidx.2 = getelementptr inbounds float, ptr %x, i64 3
  %2 = load float, ptr %arrayidx.2, align 4
  %add.2 = fadd fast float %2, %add.1
  %arrayidx.3 = getelementptr inbounds float, ptr %x, i64 4
  %3 = load float, ptr %arrayidx.3, align 4
  %add.3 = fadd fast float %3, %add.2
  %arrayidx.4 = getelementptr inbounds float, ptr %x, i64 5
  %4 = load float, ptr %arrayidx.4, align 4
  %add.4 = fadd fast float %4, %add.3
  %arrayidx.5 = getelementptr inbounds float, ptr %x, i64 6
  %5 = load float, ptr %arrayidx.5, align 4
  %add.5 = fadd fast float %5, %add.4
  %arrayidx.6 = getelementptr inbounds float, ptr %x, i64 7
  %6 = load float, ptr %arrayidx.6, align 4
  %add.6 = fadd fast float %6, %add.5
  %arrayidx.7 = getelementptr inbounds float, ptr %x, i64 8
  %7 = load float, ptr %arrayidx.7, align 4
  %add.7 = fadd fast float %7, %add.6
  %arrayidx.8 = getelementptr inbounds float, ptr %x, i64 9
  %8 = load float, ptr %arrayidx.8, align 4
  %add.8 = fadd fast float %8, %add.7
  %arrayidx.9 = getelementptr inbounds float, ptr %x, i64 10
  %9 = load float, ptr %arrayidx.9, align 4
  %add.9 = fadd fast float %9, %add.8
  %arrayidx.10 = getelementptr inbounds float, ptr %x, i64 11
  %10 = load float, ptr %arrayidx.10, align 4
  %add.10 = fadd fast float %10, %add.9
  %arrayidx.11 = getelementptr inbounds float, ptr %x, i64 12
  %11 = load float, ptr %arrayidx.11, align 4
  %add.11 = fadd fast float %11, %add.10
  %arrayidx.12 = getelementptr inbounds float, ptr %x, i64 13
  %12 = load float, ptr %arrayidx.12, align 4
  %add.12 = fadd fast float %12, %add.11
  %arrayidx.13 = getelementptr inbounds float, ptr %x, i64 14
  %13 = load float, ptr %arrayidx.13, align 4
  %add.13 = fadd fast float %13, %add.12
  %arrayidx.14 = getelementptr inbounds float, ptr %x, i64 15
  %14 = load float, ptr %arrayidx.14, align 4
  %add.14 = fadd fast float %14, %add.13
  %arrayidx.15 = getelementptr inbounds float, ptr %x, i64 16
  %15 = load float, ptr %arrayidx.15, align 4
  %add.15 = fadd fast float %15, %add.14
  %arrayidx.16 = getelementptr inbounds float, ptr %x, i64 17
  %16 = load float, ptr %arrayidx.16, align 4
  %add.16 = fadd fast float %16, %add.15
  %arrayidx.17 = getelementptr inbounds float, ptr %x, i64 18
  %17 = load float, ptr %arrayidx.17, align 4
  %add.17 = fadd fast float %17, %add.16
  %arrayidx.18 = getelementptr inbounds float, ptr %x, i64 19
  %18 = load float, ptr %arrayidx.18, align 4
  %add.18 = fadd fast float %18, %add.17
  %arrayidx.19 = getelementptr inbounds float, ptr %x, i64 20
  %19 = load float, ptr %arrayidx.19, align 4
  %add.19 = fadd fast float %19, %add.18
  %arrayidx.20 = getelementptr inbounds float, ptr %x, i64 21
  %20 = load float, ptr %arrayidx.20, align 4
  %add.20 = fadd fast float %20, %add.19
  %arrayidx.21 = getelementptr inbounds float, ptr %x, i64 22
  %21 = load float, ptr %arrayidx.21, align 4
  %add.21 = fadd fast float %21, %add.20
  %arrayidx.22 = getelementptr inbounds float, ptr %x, i64 23
  %22 = load float, ptr %arrayidx.22, align 4
  %add.22 = fadd fast float %22, %add.21
  %arrayidx.23 = getelementptr inbounds float, ptr %x, i64 24
  %23 = load float, ptr %arrayidx.23, align 4
  %add.23 = fadd fast float %23, %add.22
  %arrayidx.24 = getelementptr inbounds float, ptr %x, i64 25
  %24 = load float, ptr %arrayidx.24, align 4
  %add.24 = fadd fast float %24, %add.23
  %arrayidx.25 = getelementptr inbounds float, ptr %x, i64 26
  %25 = load float, ptr %arrayidx.25, align 4
  %add.25 = fadd fast float %25, %add.24
  %arrayidx.26 = getelementptr inbounds float, ptr %x, i64 27
  %26 = load float, ptr %arrayidx.26, align 4
  %add.26 = fadd fast float %26, %add.25
  %arrayidx.27 = getelementptr inbounds float, ptr %x, i64 28
  %27 = load float, ptr %arrayidx.27, align 4
  %add.27 = fadd fast float %27, %add.26
  %arrayidx.28 = getelementptr inbounds float, ptr %x, i64 29
  %28 = load float, ptr %arrayidx.28, align 4
  %add.28 = fadd fast float %28, %add.27
  %arrayidx.29 = getelementptr inbounds float, ptr %x, i64 30
  %29 = load float, ptr %arrayidx.29, align 4
  %add.29 = fadd fast float %29, %add.28
  ret float %add.29
}
