; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py UTC_ARGS: --version 5
; RUN: llc -mtriple=hexagon < %s | FileCheck %s
; RUN: llc -mtriple=hexagon-unknown-linux-gnu < %s | FileCheck %s
; RUN: llc -mtriple=hexagon-unknown-linux-musl < %s | FileCheck %s

define half @exp10_f16(half %x) #0 {
; CHECK-LABEL: exp10_f16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    {
; CHECK-NEXT:     call __extendhfsf2
; CHECK-NEXT:     allocframe(r29,#0):raw
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     call exp10f
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     call __truncsfhf2
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     r31:30 = dealloc_return(r30):raw
; CHECK-NEXT:    }
  %r = call half @llvm.exp10.f16(half %x)
  ret half %r
}

define <2 x half> @exp10_v2f16(<2 x half> %x) #0 {
; CHECK-LABEL: exp10_v2f16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    {
; CHECK-NEXT:     call __extendhfsf2
; CHECK-NEXT:     r16 = r1
; CHECK-NEXT:     memd(r29+#-16) = r17:16
; CHECK-NEXT:     allocframe(#8)
; CHECK-NEXT:    } // 8-byte Folded Spill
; CHECK-NEXT:    {
; CHECK-NEXT:     call exp10f
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     call __truncsfhf2
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     call __extendhfsf2
; CHECK-NEXT:     r17 = r0
; CHECK-NEXT:     r0 = r16
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     call exp10f
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     call __truncsfhf2
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     r1 = zxth(r0)
; CHECK-NEXT:     r2 = zxth(r17)
; CHECK-NEXT:     r17:16 = memd(r29+#0)
; CHECK-NEXT:    } // 8-byte Folded Reload
; CHECK-NEXT:    {
; CHECK-NEXT:     r0 = r2
; CHECK-NEXT:     dealloc_return
; CHECK-NEXT:    }
  %r = call <2 x half> @llvm.exp10.v2f16(<2 x half> %x)
  ret <2 x half> %r
}

define float @exp10_f32(float %x) #0 {
; CHECK-LABEL: exp10_f32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    {
; CHECK-NEXT:     call exp10f
; CHECK-NEXT:     allocframe(r29,#0):raw
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     r31:30 = dealloc_return(r30):raw
; CHECK-NEXT:    }
  %r = call float @llvm.exp10.f32(float %x)
  ret float %r
}

define <2 x float> @exp10_v2f32(<2 x float> %x) #0 {
; CHECK-LABEL: exp10_v2f32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    {
; CHECK-NEXT:     call exp10f
; CHECK-NEXT:     r16 = r1
; CHECK-NEXT:     memd(r29+#-16) = r17:16
; CHECK-NEXT:     allocframe(#8)
; CHECK-NEXT:    } // 8-byte Folded Spill
; CHECK-NEXT:    {
; CHECK-NEXT:     call exp10f
; CHECK-NEXT:     r17 = r0
; CHECK-NEXT:     r0 = r16
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     r1:0 = combine(r0,r17)
; CHECK-NEXT:     r17:16 = memd(r29+#0)
; CHECK-NEXT:     dealloc_return
; CHECK-NEXT:    } // 8-byte Folded Reload
  %r = call <2 x float> @llvm.exp10.v2f32(<2 x float> %x)
  ret <2 x float> %r
}

define double @exp10_f64(double %x) #0 {
; CHECK-LABEL: exp10_f64:
; CHECK:       // %bb.0:
; CHECK-NEXT:    {
; CHECK-NEXT:     call exp10
; CHECK-NEXT:     allocframe(r29,#0):raw
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     r31:30 = dealloc_return(r30):raw
; CHECK-NEXT:    }
  %r = call double @llvm.exp10.f64(double %x)
  ret double %r
}

define <2 x double> @exp10_v2f64(<2 x double> %x) #0 {
; CHECK-LABEL: exp10_v2f64:
; CHECK:       // %bb.0:
; CHECK-NEXT:    {
; CHECK-NEXT:     r16 = r0
; CHECK-NEXT:     r1:0 = combine(r3,r2)
; CHECK-NEXT:     memd(r29+#-16) = r17:16
; CHECK-NEXT:     allocframe(#24)
; CHECK-NEXT:    } // 8-byte Folded Spill
; CHECK-NEXT:    {
; CHECK-NEXT:     call exp10
; CHECK-NEXT:     r19:18 = combine(r5,r4)
; CHECK-NEXT:     memd(r29+#8) = r19:18
; CHECK-NEXT:     memd(r29+#0) = r21:20
; CHECK-NEXT:    } // 8-byte Folded Spill
; CHECK-NEXT:    {
; CHECK-NEXT:     call exp10
; CHECK-NEXT:     r21:20 = combine(r1,r0)
; CHECK-NEXT:     r1:0 = combine(r19,r18)
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     memd(r16+#8) = r1:0
; CHECK-NEXT:     memd(r16+#0) = r21:20
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     r17:16 = memd(r29+#16)
; CHECK-NEXT:     r19:18 = memd(r29+#8)
; CHECK-NEXT:    } // 8-byte Folded Reload
; CHECK-NEXT:    {
; CHECK-NEXT:     r21:20 = memd(r29+#0)
; CHECK-NEXT:     dealloc_return
; CHECK-NEXT:    } // 8-byte Folded Reload
  %r = call <2 x double> @llvm.exp10.v2f64(<2 x double> %x)
  ret <2 x double> %r
}

define fp128 @exp10_f128(fp128 %x) #0 {
; CHECK-LABEL: exp10_f128:
; CHECK:       // %bb.0:
; CHECK-NEXT:    {
; CHECK-NEXT:     r16 = r0
; CHECK-NEXT:     memd(r29+#-16) = r17:16
; CHECK-NEXT:     allocframe(#24)
; CHECK-NEXT:    } // 8-byte Folded Spill
; CHECK-NEXT:    {
; CHECK-NEXT:     call exp10l
; CHECK-NEXT:     r0 = add(r29,#0)
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     r1:0 = memd(r29+#0)
; CHECK-NEXT:     r3:2 = memd(r29+#8)
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     memd(r16+#8) = r3:2
; CHECK-NEXT:     memd(r16+#0) = r1:0
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     r17:16 = memd(r29+#16)
; CHECK-NEXT:     dealloc_return
; CHECK-NEXT:    } // 8-byte Folded Reload
  %r = call fp128 @llvm.exp10.f128(fp128 %x)
  ret fp128 %r
}

define <2 x fp128> @exp10_v2f128(<2 x fp128> %x) #0 {
; CHECK-LABEL: exp10_v2f128:
; CHECK:       // %bb.0:
; CHECK-NEXT:    {
; CHECK-NEXT:     r16 = r0
; CHECK-NEXT:     memd(r29+#-16) = r17:16
; CHECK-NEXT:     allocframe(#56)
; CHECK-NEXT:    } // 8-byte Folded Spill
; CHECK-NEXT:    {
; CHECK-NEXT:     r0 = add(r29,#16)
; CHECK-NEXT:     memd(r29+#40) = r19:18
; CHECK-NEXT:     memd(r29+#32) = r21:20
; CHECK-NEXT:    } // 8-byte Folded Spill
; CHECK-NEXT:    {
; CHECK-NEXT:     call exp10l
; CHECK-NEXT:     r19:18 = memd(r29+#64)
; CHECK-NEXT:     r21:20 = memd(r29+#72)
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     call exp10l
; CHECK-NEXT:     r0 = add(r29,#0)
; CHECK-NEXT:     r3:2 = combine(r19,r18)
; CHECK-NEXT:     r5:4 = combine(r21,r20)
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     r1:0 = memd(r29+#16)
; CHECK-NEXT:     r3:2 = memd(r29+#24)
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     r5:4 = memd(r29+#0)
; CHECK-NEXT:     r7:6 = memd(r29+#8)
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     memd(r16+#24) = r7:6
; CHECK-NEXT:     memd(r16+#16) = r5:4
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     memd(r16+#8) = r3:2
; CHECK-NEXT:     memd(r16+#0) = r1:0
; CHECK-NEXT:    }
; CHECK-NEXT:    {
; CHECK-NEXT:     r17:16 = memd(r29+#48)
; CHECK-NEXT:     r19:18 = memd(r29+#40)
; CHECK-NEXT:    } // 8-byte Folded Reload
; CHECK-NEXT:    {
; CHECK-NEXT:     r21:20 = memd(r29+#32)
; CHECK-NEXT:     dealloc_return
; CHECK-NEXT:    } // 8-byte Folded Reload
  %r = call <2 x fp128> @llvm.exp10.v2f128(<2 x fp128> %x)
  ret <2 x fp128> %r
}

attributes #0 = { nounwind }
