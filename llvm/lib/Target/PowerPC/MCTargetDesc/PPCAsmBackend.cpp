//===-- PPCAsmBackend.cpp - PPC Assembler Backend -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/PPCFixupKinds.h"
#include "MCTargetDesc/PPCMCAsmInfo.h"
#include "MCTargetDesc/PPCMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/BinaryFormat/MachO.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCMachObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/MC/MCSymbolXCOFF.h"
#include "llvm/MC/MCValue.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"
using namespace llvm;

static uint64_t adjustFixupValue(unsigned Kind, uint64_t Value) {
  switch (Kind) {
  default:
    llvm_unreachable("Unknown fixup kind!");
  case FK_Data_1:
  case FK_Data_2:
  case FK_Data_4:
  case FK_Data_8:
  case PPC::fixup_ppc_nofixup:
    return Value;
  case PPC::fixup_ppc_brcond14:
  case PPC::fixup_ppc_brcond14abs:
    return Value & 0xfffc;
  case PPC::fixup_ppc_br24:
  case PPC::fixup_ppc_br24abs:
  case PPC::fixup_ppc_br24_notoc:
    return Value & 0x3fffffc;
  case PPC::fixup_ppc_half16:
    return Value & 0xffff;
  case PPC::fixup_ppc_half16ds:
  case PPC::fixup_ppc_half16dq:
    return Value & 0xfffc;
  case PPC::fixup_ppc_pcrel34:
  case PPC::fixup_ppc_imm34:
    return Value & 0x3ffffffff;
  }
}

static unsigned getFixupKindNumBytes(unsigned Kind) {
  switch (Kind) {
  default:
    llvm_unreachable("Unknown fixup kind!");
  case FK_Data_1:
    return 1;
  case FK_Data_2:
  case PPC::fixup_ppc_half16:
  case PPC::fixup_ppc_half16ds:
  case PPC::fixup_ppc_half16dq:
    return 2;
  case FK_Data_4:
  case PPC::fixup_ppc_brcond14:
  case PPC::fixup_ppc_brcond14abs:
  case PPC::fixup_ppc_br24:
  case PPC::fixup_ppc_br24abs:
  case PPC::fixup_ppc_br24_notoc:
    return 4;
  case PPC::fixup_ppc_pcrel34:
  case PPC::fixup_ppc_imm34:
  case FK_Data_8:
    return 8;
  case PPC::fixup_ppc_nofixup:
    return 0;
  }
}

namespace {

class PPCAsmBackend : public MCAsmBackend {
protected:
  Triple TT;
public:
  PPCAsmBackend(const Target &T, const Triple &TT)
      : MCAsmBackend(TT.isLittleEndian() ? llvm::endianness::little
                                         : llvm::endianness::big),
        TT(TT) {}

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override;

  void applyFixup(const MCFragment &, const MCFixup &Fixup,
                  const MCValue &Target, uint8_t *Data, uint64_t Value,
                  bool IsResolved) override;

  bool shouldForceRelocation(const MCFixup &Fixup, const MCValue &Target) {
    // If there is a @ specifier, unless it is optimized out (e.g. constant @l),
    // force a relocation.
    if (Target.getSpecifier())
      return true;
    MCFixupKind Kind = Fixup.getKind();
    switch ((unsigned)Kind) {
    default:
      return false;
    case PPC::fixup_ppc_br24:
    case PPC::fixup_ppc_br24abs:
    case PPC::fixup_ppc_br24_notoc:
      // If the target symbol has a local entry point we must not attempt
      // to resolve the fixup directly.  Emit a relocation and leave
      // resolution of the final target address to the linker.
      if (const auto *A = Target.getAddSym()) {
        if (const auto *S = dyn_cast<MCSymbolELF>(A)) {
          // The "other" values are stored in the last 6 bits of the second
          // byte. The traditional defines for STO values assume the full byte
          // and thus the shift to pack it.
          unsigned Other = S->getOther() << 2;
          if ((Other & ELF::STO_PPC64_LOCAL_MASK) != 0)
            return true;
        } else if (const auto *S = dyn_cast<MCSymbolXCOFF>(A)) {
          return !Target.isAbsolute() && S->isExternal() &&
                 S->getStorageClass() == XCOFF::C_WEAKEXT;
        }
      }
      return false;
    }
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override {
    uint64_t NumNops = Count / 4;
    for (uint64_t i = 0; i != NumNops; ++i)
      support::endian::write<uint32_t>(OS, 0x60000000, Endian);

    OS.write_zeros(Count % 4);

    return true;
  }
};
} // end anonymous namespace

MCFixupKindInfo PPCAsmBackend::getFixupKindInfo(MCFixupKind Kind) const {
  // clang-format off
  const static MCFixupKindInfo InfosBE[PPC::NumTargetFixupKinds] = {
      // name                    offset  bits  flags
      {"fixup_ppc_br24", 6, 24, 0},
      {"fixup_ppc_br24_notoc", 6, 24, 0},
      {"fixup_ppc_brcond14", 16, 14, 0},
      {"fixup_ppc_br24abs", 6, 24, 0},
      {"fixup_ppc_brcond14abs", 16, 14, 0},
      {"fixup_ppc_half16", 0, 16, 0},
      {"fixup_ppc_half16ds", 0, 14, 0},
      {"fixup_ppc_pcrel34", 0, 34, 0},
      {"fixup_ppc_imm34", 0, 34, 0},
      {"fixup_ppc_nofixup", 0, 0, 0}};
  const static MCFixupKindInfo InfosLE[PPC::NumTargetFixupKinds] = {
      // name                    offset  bits  flags
      {"fixup_ppc_br24", 2, 24, 0},
      {"fixup_ppc_br24_notoc", 2, 24, 0},
      {"fixup_ppc_brcond14", 2, 14, 0},
      {"fixup_ppc_br24abs", 2, 24, 0},
      {"fixup_ppc_brcond14abs", 2, 14, 0},
      {"fixup_ppc_half16", 0, 16, 0},
      {"fixup_ppc_half16ds", 2, 14, 0},
      {"fixup_ppc_pcrel34", 0, 34, 0},
      {"fixup_ppc_imm34", 0, 34, 0},
      {"fixup_ppc_nofixup", 0, 0, 0}};
  // clang-format on

  // Fixup kinds from .reloc directive are like R_PPC_NONE/R_PPC64_NONE. They
  // do not require any extra processing.
  if (mc::isRelocation(Kind))
    return {};

  if (Kind < FirstTargetFixupKind)
    return MCAsmBackend::getFixupKindInfo(Kind);

  assert(Kind - FirstTargetFixupKind < PPC::NumTargetFixupKinds &&
         "Invalid kind!");
  return (Endian == llvm::endianness::little
              ? InfosLE
              : InfosBE)[Kind - FirstTargetFixupKind];
}

void PPCAsmBackend::applyFixup(const MCFragment &F, const MCFixup &Fixup,
                               const MCValue &TargetVal, uint8_t *Data,
                               uint64_t Value, bool IsResolved) {
  // In PPC64 ELFv1, .quad .TOC.@tocbase in the .opd section is expected to
  // reference the null symbol.
  auto Target = TargetVal;
  if (Target.getSpecifier() == PPC::S_TOCBASE)
    Target.setAddSym(nullptr);
  if (IsResolved && shouldForceRelocation(Fixup, Target))
    IsResolved = false;
  if (!IsResolved)
    Asm->getWriter().recordRelocation(F, Fixup, Target, Value);

  MCFixupKind Kind = Fixup.getKind();
  if (mc::isRelocation(Kind))
    return;
  Value = adjustFixupValue(Kind, Value);
  if (!Value)
    return; // Doesn't change encoding.

  unsigned NumBytes = getFixupKindNumBytes(Kind);

  // For each byte of the fragment that the fixup touches, mask in the bits
  // from the fixup value. The Value has been "split up" into the appropriate
  // bitfields above.
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned Idx = Endian == llvm::endianness::little ? i : (NumBytes - 1 - i);
    Data[i] |= uint8_t((Value >> (Idx * 8)) & 0xff);
  }
}

// FIXME: This should be in a separate file.
namespace {

class ELFPPCAsmBackend : public PPCAsmBackend {
public:
  ELFPPCAsmBackend(const Target &T, const Triple &TT) : PPCAsmBackend(T, TT) {}

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    uint8_t OSABI = MCELFObjectTargetWriter::getOSABI(TT.getOS());
    bool Is64 = TT.isPPC64();
    return createPPCELFObjectWriter(Is64, OSABI);
  }

  std::optional<MCFixupKind> getFixupKind(StringRef Name) const override;
};

class XCOFFPPCAsmBackend : public PPCAsmBackend {
public:
  XCOFFPPCAsmBackend(const Target &T, const Triple &TT)
      : PPCAsmBackend(T, TT) {}

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createPPCXCOFFObjectWriter(TT.isArch64Bit());
  }
};

} // end anonymous namespace

std::optional<MCFixupKind>
ELFPPCAsmBackend::getFixupKind(StringRef Name) const {
  if (TT.isOSBinFormatELF()) {
    unsigned Type;
    if (TT.isPPC64()) {
      Type = llvm::StringSwitch<unsigned>(Name)
#define ELF_RELOC(X, Y) .Case(#X, Y)
#include "llvm/BinaryFormat/ELFRelocs/PowerPC64.def"
#undef ELF_RELOC
                 .Case("BFD_RELOC_NONE", ELF::R_PPC64_NONE)
                 .Case("BFD_RELOC_16", ELF::R_PPC64_ADDR16)
                 .Case("BFD_RELOC_32", ELF::R_PPC64_ADDR32)
                 .Case("BFD_RELOC_64", ELF::R_PPC64_ADDR64)
                 .Default(-1u);
    } else {
      Type = llvm::StringSwitch<unsigned>(Name)
#define ELF_RELOC(X, Y) .Case(#X, Y)
#include "llvm/BinaryFormat/ELFRelocs/PowerPC.def"
#undef ELF_RELOC
                 .Case("BFD_RELOC_NONE", ELF::R_PPC_NONE)
                 .Case("BFD_RELOC_16", ELF::R_PPC_ADDR16)
                 .Case("BFD_RELOC_32", ELF::R_PPC_ADDR32)
                 .Default(-1u);
    }
    if (Type != -1u)
      return static_cast<MCFixupKind>(FirstLiteralRelocationKind + Type);
  }
  return std::nullopt;
}

MCAsmBackend *llvm::createPPCAsmBackend(const Target &T,
                                        const MCSubtargetInfo &STI,
                                        const MCRegisterInfo &MRI,
                                        const MCTargetOptions &Options) {
  const Triple &TT = STI.getTargetTriple();
  if (TT.isOSBinFormatXCOFF())
    return new XCOFFPPCAsmBackend(T, TT);

  return new ELFPPCAsmBackend(T, TT);
}
