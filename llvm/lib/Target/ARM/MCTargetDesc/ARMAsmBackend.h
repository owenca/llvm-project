//===-- ARMAsmBackend.h - ARM Assembler Backend -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARM_ARMASMBACKEND_H
#define LLVM_LIB_TARGET_ARM_ARMASMBACKEND_H

#include "MCTargetDesc/ARMFixupKinds.h"
#include "MCTargetDesc/ARMMCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

namespace llvm {

class ARMAsmBackend : public MCAsmBackend {
public:
  ARMAsmBackend(const Target &T, llvm::endianness Endian)
      : MCAsmBackend(Endian) {}

  bool hasNOP(const MCSubtargetInfo *STI) const {
    return STI->hasFeature(ARM::HasV6T2Ops);
  }

  std::optional<MCFixupKind> getFixupKind(StringRef Name) const override;

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override;

  bool shouldForceRelocation(const MCFixup &Fixup, const MCValue &Target);

  unsigned adjustFixupValue(const MCAssembler &Asm, const MCFixup &Fixup,
                            const MCValue &Target, uint64_t Value,
                            bool IsResolved, MCContext &Ctx,
                            const MCSubtargetInfo *STI) const;

  std::optional<bool> evaluateFixup(const MCFragment &, MCFixup &, MCValue &,
                                    uint64_t &) override;
  void applyFixup(const MCFragment &, const MCFixup &, const MCValue &Target,
                  uint8_t *Data, uint64_t Value, bool IsResolved) override;

  unsigned getRelaxedOpcode(unsigned Op, const MCSubtargetInfo &STI) const;

  bool mayNeedRelaxation(unsigned Opcode, ArrayRef<MCOperand> Operands,
                         const MCSubtargetInfo &STI) const override;

  const char *reasonForFixupRelaxation(const MCFixup &Fixup,
                                       uint64_t Value) const;

  bool fixupNeedsRelaxationAdvanced(const MCFragment &, const MCFixup &,
                                    const MCValue &, uint64_t,
                                    bool) const override;

  void relaxInstruction(MCInst &Inst,
                        const MCSubtargetInfo &STI) const override;

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override;

  unsigned getPointerSize() const { return 4; }
};
} // end namespace llvm

#endif
