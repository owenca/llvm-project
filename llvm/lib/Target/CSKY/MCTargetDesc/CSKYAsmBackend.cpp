//===-- CSKYAsmBackend.cpp - CSKY Assembler Backend -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CSKYAsmBackend.h"
#include "MCTargetDesc/CSKYMCTargetDesc.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "csky-asmbackend"

using namespace llvm;

std::unique_ptr<MCObjectTargetWriter>
CSKYAsmBackend::createObjectTargetWriter() const {
  return createCSKYELFObjectWriter();
}

MCFixupKindInfo CSKYAsmBackend::getFixupKindInfo(MCFixupKind Kind) const {

  static llvm::DenseMap<unsigned, MCFixupKindInfo> Infos = {
      {CSKY::Fixups::fixup_csky_addr32, {"fixup_csky_addr32", 0, 32, 0}},
      {CSKY::Fixups::fixup_csky_addr_hi16, {"fixup_csky_addr_hi16", 0, 32, 0}},
      {CSKY::Fixups::fixup_csky_addr_lo16, {"fixup_csky_addr_lo16", 0, 32, 0}},
      {CSKY::Fixups::fixup_csky_pcrel_imm16_scale2,
       {"fixup_csky_pcrel_imm16_scale2", 0, 32, 0}},
      {CSKY::Fixups::fixup_csky_pcrel_uimm16_scale4,
       {"fixup_csky_pcrel_uimm16_scale4", 0, 32, 0}},
      {CSKY::Fixups::fixup_csky_pcrel_uimm8_scale4,
       {"fixup_csky_pcrel_uimm8_scale4", 0, 32, 0}},
      {CSKY::Fixups::fixup_csky_pcrel_imm26_scale2,
       {"fixup_csky_pcrel_imm26_scale2", 0, 32, 0}},
      {CSKY::Fixups::fixup_csky_pcrel_imm18_scale2,
       {"fixup_csky_pcrel_imm18_scale2", 0, 32, 0}},
      {CSKY::Fixups::fixup_csky_got32, {"fixup_csky_got32", 0, 32, 0}},
      {CSKY::Fixups::fixup_csky_got_imm18_scale4,
       {"fixup_csky_got_imm18_scale4", 0, 32, 0}},
      {CSKY::Fixups::fixup_csky_gotoff, {"fixup_csky_gotoff", 0, 32, 0}},
      {CSKY::Fixups::fixup_csky_gotpc, {"fixup_csky_gotpc", 0, 32, 0}},
      {CSKY::Fixups::fixup_csky_plt32, {"fixup_csky_plt32", 0, 32, 0}},
      {CSKY::Fixups::fixup_csky_plt_imm18_scale4,
       {"fixup_csky_plt_imm18_scale4", 0, 32, 0}},
      {CSKY::Fixups::fixup_csky_pcrel_imm10_scale2,
       {"fixup_csky_pcrel_imm10_scale2", 0, 16, 0}},
      {CSKY::Fixups::fixup_csky_pcrel_uimm7_scale4,
       {"fixup_csky_pcrel_uimm7_scale4", 0, 16, 0}},
      {CSKY::Fixups::fixup_csky_doffset_imm18,
       {"fixup_csky_doffset_imm18", 0, 18, 0}},
      {CSKY::Fixups::fixup_csky_doffset_imm18_scale2,
       {"fixup_csky_doffset_imm18_scale2", 0, 18, 0}},
      {CSKY::Fixups::fixup_csky_doffset_imm18_scale4,
       {"fixup_csky_doffset_imm18_scale4", 0, 18, 0}}};

  assert(Infos.size() == CSKY::NumTargetFixupKinds &&
         "Not all fixup kinds added to Infos array");

  if (mc::isRelocation(Kind))
    return {};
  if (Kind < FirstTargetFixupKind)
    return MCAsmBackend::getFixupKindInfo(Kind);
  return Infos[Kind];
}

static uint64_t adjustFixupValue(const MCFixup &Fixup, uint64_t Value,
                                 MCContext &Ctx) {
  switch (Fixup.getKind()) {
  default:
    llvm_unreachable("Unknown fixup kind!");
  case CSKY::fixup_csky_got32:
  case CSKY::fixup_csky_got_imm18_scale4:
  case CSKY::fixup_csky_gotoff:
  case CSKY::fixup_csky_gotpc:
  case CSKY::fixup_csky_plt32:
  case CSKY::fixup_csky_plt_imm18_scale4:
    llvm_unreachable("Relocation should be unconditionally forced\n");
  case FK_Data_1:
  case FK_Data_2:
  case FK_Data_4:
  case FK_Data_8:
    return Value;
  case CSKY::fixup_csky_addr32:
    return Value & 0xffffffff;
  case CSKY::fixup_csky_pcrel_imm16_scale2:
    if (!isIntN(17, Value))
      Ctx.reportError(Fixup.getLoc(), "out of range pc-relative fixup value.");
    if (Value & 0x1)
      Ctx.reportError(Fixup.getLoc(), "fixup value must be 2-byte aligned.");

    return (Value >> 1) & 0xffff;
  case CSKY::fixup_csky_pcrel_uimm16_scale4:
    if (!isUIntN(18, Value))
      Ctx.reportError(Fixup.getLoc(), "out of range pc-relative fixup value.");
    if (Value & 0x3)
      Ctx.reportError(Fixup.getLoc(), "fixup value must be 4-byte aligned.");

    return (Value >> 2) & 0xffff;
  case CSKY::fixup_csky_pcrel_imm26_scale2:
    if (!isIntN(27, Value))
      Ctx.reportError(Fixup.getLoc(), "out of range pc-relative fixup value.");
    if (Value & 0x1)
      Ctx.reportError(Fixup.getLoc(), "fixup value must be 2-byte aligned.");

    return (Value >> 1) & 0x3ffffff;
  case CSKY::fixup_csky_pcrel_imm18_scale2:
    if (!isIntN(19, Value))
      Ctx.reportError(Fixup.getLoc(), "out of range pc-relative fixup value.");
    if (Value & 0x1)
      Ctx.reportError(Fixup.getLoc(), "fixup value must be 2-byte aligned.");

    return (Value >> 1) & 0x3ffff;
  case CSKY::fixup_csky_pcrel_uimm8_scale4: {
    if (!isUIntN(10, Value))
      Ctx.reportError(Fixup.getLoc(), "out of range pc-relative fixup value.");
    if (Value & 0x3)
      Ctx.reportError(Fixup.getLoc(), "fixup value must be 4-byte aligned.");

    unsigned IMM4L = (Value >> 2) & 0xf;
    unsigned IMM4H = (Value >> 6) & 0xf;

    Value = (IMM4H << 21) | (IMM4L << 4);
    return Value;
  }
  case CSKY::fixup_csky_pcrel_imm10_scale2:
    if (!isIntN(11, Value))
      Ctx.reportError(Fixup.getLoc(), "out of range pc-relative fixup value.");
    if (Value & 0x1)
      Ctx.reportError(Fixup.getLoc(), "fixup value must be 2-byte aligned.");

    return (Value >> 1) & 0x3ff;
  case CSKY::fixup_csky_pcrel_uimm7_scale4:
    if ((Value >> 2) > 0xfe)
      Ctx.reportError(Fixup.getLoc(), "out of range pc-relative fixup value.");
    if (Value & 0x3)
      Ctx.reportError(Fixup.getLoc(), "fixup value must be 4-byte aligned.");

    if ((Value >> 2) <= 0x7f) {
      unsigned IMM5L = (Value >> 2) & 0x1f;
      unsigned IMM2H = (Value >> 7) & 0x3;

      Value = (1 << 12) | (IMM2H << 8) | IMM5L;
    } else {
      unsigned IMM5L = (~Value >> 2) & 0x1f;
      unsigned IMM2H = (~Value >> 7) & 0x3;

      Value = (IMM2H << 8) | IMM5L;
    }

    return Value;
  }
}

bool CSKYAsmBackend::fixupNeedsRelaxationAdvanced(const MCFragment &,
                                                  const MCFixup &Fixup,
                                                  const MCValue &,
                                                  uint64_t Value,
                                                  bool Resolved) const {
  // Return true if the symbol is unresolved.
  if (!Resolved)
    return true;

  int64_t Offset = int64_t(Value);
  switch (Fixup.getKind()) {
  default:
    return false;
  case CSKY::fixup_csky_pcrel_imm10_scale2:
    return !isShiftedInt<10, 1>(Offset);
  case CSKY::fixup_csky_pcrel_imm16_scale2:
    return !isShiftedInt<16, 1>(Offset);
  case CSKY::fixup_csky_pcrel_imm26_scale2:
    return !isShiftedInt<26, 1>(Offset);
  case CSKY::fixup_csky_pcrel_uimm7_scale4:
    return ((Value >> 2) > 0xfe) || (Value & 0x3);
  }
}

std::optional<bool> CSKYAsmBackend::evaluateFixup(const MCFragment &F,
                                                  MCFixup &Fixup, MCValue &,
                                                  uint64_t &Value) {
  // For a few PC-relative fixups, offsets need to be aligned down. We
  // compensate here because the default handler's `Value` decrement doesn't
  // account for this alignment.
  switch (Fixup.getKind()) {
  case CSKY::fixup_csky_pcrel_uimm16_scale4:
  case CSKY::fixup_csky_pcrel_uimm8_scale4:
  case CSKY::fixup_csky_pcrel_uimm7_scale4:
    Value = (Asm->getFragmentOffset(F) + Fixup.getOffset()) % 4;
  }
  return {};
}

void CSKYAsmBackend::applyFixup(const MCFragment &F, const MCFixup &Fixup,
                                const MCValue &Target, uint8_t *Data,
                                uint64_t Value, bool IsResolved) {
  if (IsResolved && shouldForceRelocation(Fixup, Target))
    IsResolved = false;
  maybeAddReloc(F, Fixup, Target, Value, IsResolved);

  MCFixupKind Kind = Fixup.getKind();
  if (mc::isRelocation(Kind))
    return;
  MCContext &Ctx = getContext();
  MCFixupKindInfo Info = getFixupKindInfo(Kind);
  if (!Value)
    return; // Doesn't change encoding.
  // Apply any target-specific value adjustments.
  Value = adjustFixupValue(Fixup, Value, Ctx);

  // Shift the value into position.
  Value <<= Info.TargetOffset;

  unsigned NumBytes = alignTo(Info.TargetSize + Info.TargetOffset, 8) / 8;

  assert(Fixup.getOffset() + NumBytes <= F.getSize() &&
         "Invalid fixup offset!");

  // For each byte of the fragment that the fixup touches, mask in the
  // bits from the fixup value.
  bool IsLittleEndian = (Endian == llvm::endianness::little);
  bool IsInstFixup = (Kind >= FirstTargetFixupKind);

  if (IsLittleEndian && IsInstFixup && (NumBytes == 4)) {
    Data[0] |= uint8_t((Value >> 16) & 0xff);
    Data[1] |= uint8_t((Value >> 24) & 0xff);
    Data[2] |= uint8_t(Value & 0xff);
    Data[3] |= uint8_t((Value >> 8) & 0xff);
  } else {
    for (unsigned I = 0; I != NumBytes; I++) {
      unsigned Idx = IsLittleEndian ? I : (NumBytes - 1 - I);
      Data[Idx] |= uint8_t((Value >> (I * 8)) & 0xff);
    }
  }
}

bool CSKYAsmBackend::mayNeedRelaxation(unsigned Opcode, ArrayRef<MCOperand>,
                                       const MCSubtargetInfo &STI) const {
  switch (Opcode) {
  default:
    return false;
  case CSKY::JBR32:
  case CSKY::JBT32:
  case CSKY::JBF32:
  case CSKY::JBSR32:
    if (!STI.hasFeature(CSKY::Has2E3))
      return false;
    return true;
  case CSKY::JBR16:
  case CSKY::JBT16:
  case CSKY::JBF16:
  case CSKY::LRW16:
  case CSKY::BR16:
    return true;
  }
}

bool CSKYAsmBackend::shouldForceRelocation(const MCFixup &Fixup,
                                           const MCValue &Target /*STI*/) {
  if (Target.getSpecifier())
    return true;
  switch (Fixup.getKind()) {
  default:
    break;
  case CSKY::fixup_csky_doffset_imm18:
  case CSKY::fixup_csky_doffset_imm18_scale2:
  case CSKY::fixup_csky_doffset_imm18_scale4:
    return true;
  }

  return false;
}

bool CSKYAsmBackend::fixupNeedsRelaxation(const MCFixup &Fixup,
                                          uint64_t Value) const {
  return false;
}

void CSKYAsmBackend::relaxInstruction(MCInst &Inst,
                                      const MCSubtargetInfo &STI) const {
  MCInst Res;

  switch (Inst.getOpcode()) {
  default:
    LLVM_DEBUG(Inst.dump());
    llvm_unreachable("Opcode not expected!");
  case CSKY::LRW16:
    Res.setOpcode(CSKY::LRW32);
    Res.addOperand(Inst.getOperand(0));
    Res.addOperand(Inst.getOperand(1));
    break;
  case CSKY::BR16:
    Res.setOpcode(CSKY::BR32);
    Res.addOperand(Inst.getOperand(0));
    break;
  case CSKY::JBSR32:
    Res.setOpcode(CSKY::JSRI32);
    Res.addOperand(Inst.getOperand(1));
    break;
  case CSKY::JBR32:
    Res.setOpcode(CSKY::JMPI32);
    Res.addOperand(Inst.getOperand(1));
    break;
  case CSKY::JBT32:
  case CSKY::JBF32:
    Res.setOpcode(Inst.getOpcode() == CSKY::JBT32 ? CSKY::JBT_E : CSKY::JBF_E);
    Res.addOperand(Inst.getOperand(0));
    Res.addOperand(Inst.getOperand(1));
    Res.addOperand(Inst.getOperand(2));
    break;
  case CSKY::JBR16:
    Res.setOpcode(CSKY::JBR32);
    Res.addOperand(Inst.getOperand(0));
    Res.addOperand(Inst.getOperand(1));
    break;
  case CSKY::JBT16:
  case CSKY::JBF16:
    // ck801
    unsigned opcode;
    if (STI.hasFeature(CSKY::HasE2))
      opcode = Inst.getOpcode() == CSKY::JBT16 ? CSKY::JBT32 : CSKY::JBF32;
    else
      opcode = Inst.getOpcode() == CSKY::JBT16 ? CSKY::JBT_E : CSKY::JBF_E;

    Res.setOpcode(opcode);
    Res.addOperand(Inst.getOperand(0));
    Res.addOperand(Inst.getOperand(1));
    Res.addOperand(Inst.getOperand(2));
    break;
  }
  Inst = std::move(Res);
}

bool CSKYAsmBackend::writeNopData(raw_ostream &OS, uint64_t Count,
                                  const MCSubtargetInfo *STI) const {
  OS.write_zeros(Count);
  return true;
}

MCAsmBackend *llvm::createCSKYAsmBackend(const Target &T,
                                         const MCSubtargetInfo &STI,
                                         const MCRegisterInfo &MRI,
                                         const MCTargetOptions &Options) {
  return new CSKYAsmBackend(STI, Options);
}
