//===--- PS4CPU.cpp - PS4CPU ToolChain Implementations ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PS4CPU.h"
#include "clang/Config/config.h"
#include "clang/Driver/CommonArgs.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Options.h"
#include "clang/Driver/SanitizerArgs.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include <cstdlib> // ::getenv

using namespace clang::driver;
using namespace clang;
using namespace llvm::opt;

// Helper to paste bits of an option together and return a saved string.
static const char *makeArgString(const ArgList &Args, const char *Prefix,
                                 const char *Base, const char *Suffix) {
  // Basically "Prefix + Base + Suffix" all converted to Twine then saved.
  return Args.MakeArgString(Twine(StringRef(Prefix), Base) + Suffix);
}

void tools::PScpu::addProfileRTArgs(const ToolChain &TC, const ArgList &Args,
                                    ArgStringList &CmdArgs) {
  assert(TC.getTriple().isPS());
  auto &PSTC = static_cast<const toolchains::PS4PS5Base &>(TC);

  if ((Args.hasFlag(options::OPT_fprofile_arcs, options::OPT_fno_profile_arcs,
                    false) ||
       Args.hasFlag(options::OPT_fprofile_generate,
                    options::OPT_fno_profile_generate, false) ||
       Args.hasFlag(options::OPT_fprofile_generate_EQ,
                    options::OPT_fno_profile_generate, false) ||
       Args.hasFlag(options::OPT_fprofile_instr_generate,
                    options::OPT_fno_profile_instr_generate, false) ||
       Args.hasFlag(options::OPT_fprofile_instr_generate_EQ,
                    options::OPT_fno_profile_instr_generate, false) ||
       Args.hasFlag(options::OPT_fcs_profile_generate,
                    options::OPT_fno_profile_generate, false) ||
       Args.hasFlag(options::OPT_fcs_profile_generate_EQ,
                    options::OPT_fno_profile_generate, false) ||
       Args.hasArg(options::OPT_fcreate_profile) ||
       Args.hasArg(options::OPT_coverage)))
    CmdArgs.push_back(makeArgString(
        Args, "--dependent-lib=", PSTC.getProfileRTLibName(), ""));
}

void tools::PScpu::Assembler::ConstructJob(Compilation &C, const JobAction &JA,
                                           const InputInfo &Output,
                                           const InputInfoList &Inputs,
                                           const ArgList &Args,
                                           const char *LinkingOutput) const {
  auto &TC = static_cast<const toolchains::PS4PS5Base &>(getToolChain());
  claimNoWarnArgs(Args);
  ArgStringList CmdArgs;

  Args.AddAllArgValues(CmdArgs, options::OPT_Wa_COMMA, options::OPT_Xassembler);

  CmdArgs.push_back("-o");
  CmdArgs.push_back(Output.getFilename());

  assert(Inputs.size() == 1 && "Unexpected number of inputs.");
  const InputInfo &Input = Inputs[0];
  assert(Input.isFilename() && "Invalid input.");
  CmdArgs.push_back(Input.getFilename());

  std::string AsName = TC.qualifyPSCmdName("as");
  const char *Exec = Args.MakeArgString(TC.GetProgramPath(AsName.c_str()));
  C.addCommand(std::make_unique<Command>(JA, *this,
                                         ResponseFileSupport::AtFileUTF8(),
                                         Exec, CmdArgs, Inputs, Output));
}

void tools::PScpu::addSanitizerArgs(const ToolChain &TC, const ArgList &Args,
                                    ArgStringList &CmdArgs) {
  assert(TC.getTriple().isPS());
  auto &PSTC = static_cast<const toolchains::PS4PS5Base &>(TC);
  PSTC.addSanitizerArgs(Args, CmdArgs, "--dependent-lib=lib", ".a");
}

void toolchains::PS4CPU::addSanitizerArgs(const ArgList &Args,
                                          ArgStringList &CmdArgs,
                                          const char *Prefix,
                                          const char *Suffix) const {
  auto arg = [&](const char *Name) -> const char * {
    return makeArgString(Args, Prefix, Name, Suffix);
  };
  const SanitizerArgs &SanArgs = getSanitizerArgs(Args);
  if (SanArgs.needsUbsanRt())
    CmdArgs.push_back(arg("SceDbgUBSanitizer_stub_weak"));
  if (SanArgs.needsAsanRt())
    CmdArgs.push_back(arg("SceDbgAddressSanitizer_stub_weak"));
}

void toolchains::PS5CPU::addSanitizerArgs(const ArgList &Args,
                                          ArgStringList &CmdArgs,
                                          const char *Prefix,
                                          const char *Suffix) const {
  auto arg = [&](const char *Name) -> const char * {
    return makeArgString(Args, Prefix, Name, Suffix);
  };
  const SanitizerArgs &SanArgs = getSanitizerArgs(Args);
  if (SanArgs.needsUbsanRt())
    CmdArgs.push_back(arg("SceUBSanitizer_nosubmission_stub_weak"));
  if (SanArgs.needsAsanRt())
    CmdArgs.push_back(arg("SceAddressSanitizer_nosubmission_stub_weak"));
  if (SanArgs.needsTsanRt())
    CmdArgs.push_back(arg("SceThreadSanitizer_nosubmission_stub_weak"));
}

void tools::PS4cpu::Linker::ConstructJob(Compilation &C, const JobAction &JA,
                                         const InputInfo &Output,
                                         const InputInfoList &Inputs,
                                         const ArgList &Args,
                                         const char *LinkingOutput) const {
  auto &TC = static_cast<const toolchains::PS4PS5Base &>(getToolChain());
  const Driver &D = TC.getDriver();
  ArgStringList CmdArgs;

  // Silence warning for "clang -g foo.o -o foo"
  Args.ClaimAllArgs(options::OPT_g_Group);
  // and "clang -emit-llvm foo.o -o foo"
  Args.ClaimAllArgs(options::OPT_emit_llvm);
  // and for "clang -w foo.o -o foo". Other warning options are already
  // handled somewhere else.
  Args.ClaimAllArgs(options::OPT_w);

  CmdArgs.push_back(
      Args.MakeArgString("--sysroot=" + TC.getSDKLibraryRootDir()));

  if (Args.hasArg(options::OPT_pie))
    CmdArgs.push_back("-pie");

  if (Args.hasArg(options::OPT_static))
    CmdArgs.push_back("-static");
  if (Args.hasArg(options::OPT_rdynamic))
    CmdArgs.push_back("-export-dynamic");
  if (Args.hasArg(options::OPT_shared))
    CmdArgs.push_back("--shared");

  assert((Output.isFilename() || Output.isNothing()) && "Invalid output.");
  if (Output.isFilename()) {
    CmdArgs.push_back("-o");
    CmdArgs.push_back(Output.getFilename());
  }

  const bool UseJMC =
      Args.hasFlag(options::OPT_fjmc, options::OPT_fno_jmc, false);

  const char *LTOArgs = "";
  auto AddLTOFlag = [&](Twine Flag) {
    LTOArgs = Args.MakeArgString(Twine(LTOArgs) + " " + Flag);
  };

  // If the linker sees bitcode objects it will perform LTO. We can't tell
  // whether or not that will be the case at this point. So, unconditionally
  // pass LTO options to ensure proper codegen, metadata production, etc if
  // LTO indeed occurs.
  if (Args.hasFlag(options::OPT_funified_lto, options::OPT_fno_unified_lto,
                   true))
    CmdArgs.push_back(D.getLTOMode() == LTOK_Thin ? "--lto=thin"
                                                  : "--lto=full");
  if (UseJMC)
    AddLTOFlag("-enable-jmc-instrument");

  if (Arg *A = Args.getLastArg(options::OPT_fcrash_diagnostics_dir))
    AddLTOFlag(Twine("-crash-diagnostics-dir=") + A->getValue());

  if (StringRef Threads = getLTOParallelism(Args, D); !Threads.empty())
    AddLTOFlag(Twine("-threads=") + Threads);

  if (*LTOArgs)
    CmdArgs.push_back(
        Args.MakeArgString(Twine("-lto-debug-options=") + LTOArgs));

  // Sanitizer runtimes must be supplied before all other objects and libs.
  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs))
    TC.addSanitizerArgs(Args, CmdArgs, "-l", "");

  // Other drivers typically add library search paths (`-L`) here via
  // TC.AddFilePathLibArgs(). We don't do that on PS4 as the PS4 linker
  // searches those locations by default.
  Args.addAllArgs(CmdArgs, {options::OPT_L, options::OPT_T_Group,
                            options::OPT_s, options::OPT_t});

  if (Args.hasArg(options::OPT_Z_Xlinker__no_demangle))
    CmdArgs.push_back("--no-demangle");

  AddLinkerInputs(TC, Inputs, Args, CmdArgs, JA);

  if (Args.hasArg(options::OPT_pthread)) {
    CmdArgs.push_back("-lpthread");
  }

  if (UseJMC) {
    CmdArgs.push_back("--whole-archive");
    CmdArgs.push_back("-lSceDbgJmc");
    CmdArgs.push_back("--no-whole-archive");
  }

  if (Args.hasArg(options::OPT_fuse_ld_EQ)) {
    D.Diag(diag::err_drv_unsupported_opt_for_target)
        << "-fuse-ld" << TC.getTriple().str();
  }

  std::string LdName = TC.qualifyPSCmdName(TC.getLinkerBaseName());
  const char *Exec = Args.MakeArgString(TC.GetProgramPath(LdName.c_str()));

  C.addCommand(std::make_unique<Command>(JA, *this,
                                         ResponseFileSupport::AtFileUTF8(),
                                         Exec, CmdArgs, Inputs, Output));
}

void tools::PS5cpu::Linker::ConstructJob(Compilation &C, const JobAction &JA,
                                         const InputInfo &Output,
                                         const InputInfoList &Inputs,
                                         const ArgList &Args,
                                         const char *LinkingOutput) const {
  auto &TC = static_cast<const toolchains::PS4PS5Base &>(getToolChain());
  const Driver &D = TC.getDriver();
  ArgStringList CmdArgs;

  const bool Relocatable = Args.hasArg(options::OPT_r);
  const bool Shared = Args.hasArg(options::OPT_shared);
  const bool Static = Args.hasArg(options::OPT_static);

  // Silence warning for "clang -g foo.o -o foo"
  Args.ClaimAllArgs(options::OPT_g_Group);
  // and "clang -emit-llvm foo.o -o foo"
  Args.ClaimAllArgs(options::OPT_emit_llvm);
  // and for "clang -w foo.o -o foo". Other warning options are already
  // handled somewhere else.
  Args.ClaimAllArgs(options::OPT_w);

  CmdArgs.push_back("-m");
  CmdArgs.push_back("elf_x86_64_fbsd");

  CmdArgs.push_back(
      Args.MakeArgString("--sysroot=" + TC.getSDKLibraryRootDir()));

  // Default to PIE for non-static executables.
  const bool PIE = Args.hasFlag(options::OPT_pie, options::OPT_no_pie,
                                !Relocatable && !Shared && !Static);
  if (PIE)
    CmdArgs.push_back("-pie");

  if (!Relocatable) {
    CmdArgs.push_back("--eh-frame-hdr");
    CmdArgs.push_back("--hash-style=sysv");

    // Add a build-id by default to allow the PlayStation symbol server to
    // index the symbols. `uuid` is the cheapest fool-proof method.
    // (The non-determinism and alternative methods are noted in the downstream
    // PlayStation docs).
    // Static executables are only used for a handful of specialized components,
    // where the extra section is not wanted.
    if (!Static)
      CmdArgs.push_back("--build-id=uuid");

    // All references are expected to be resolved at static link time for both
    // executables and dynamic libraries. This has been the default linking
    // behaviour for numerous PlayStation generations.
    CmdArgs.push_back("--unresolved-symbols=report-all");

    // Lazy binding of PLTs is not supported on PlayStation. They are placed in
    // the RelRo segment.
    CmdArgs.push_back("-z");
    CmdArgs.push_back("now");

    // Don't export linker-generated __start/stop... section bookends.
    CmdArgs.push_back("-z");
    CmdArgs.push_back("start-stop-visibility=hidden");

    // DT_DEBUG is not supported on PlayStation.
    CmdArgs.push_back("-z");
    CmdArgs.push_back("rodynamic");

    CmdArgs.push_back("-z");
    CmdArgs.push_back("common-page-size=0x4000");

    CmdArgs.push_back("-z");
    CmdArgs.push_back("max-page-size=0x4000");

    // Patch relocated regions of DWARF whose targets are eliminated at link
    // time with specific tombstones, such that they're recognisable by the
    // PlayStation debugger.
    CmdArgs.push_back("-z");
    CmdArgs.push_back("dead-reloc-in-nonalloc=.debug_*=0xffffffffffffffff");
    CmdArgs.push_back("-z");
    CmdArgs.push_back(
        "dead-reloc-in-nonalloc=.debug_ranges=0xfffffffffffffffe");
    CmdArgs.push_back("-z");
    CmdArgs.push_back("dead-reloc-in-nonalloc=.debug_loc=0xfffffffffffffffe");

    // The PlayStation loader expects linked objects to be laid out in a
    // particular way. This is achieved by linker scripts that are supplied
    // with the SDK. The scripts are inside <sdkroot>/target/lib, which is
    // added as a search path elsewhere.
    // "PRX" has long stood for "PlayStation Relocatable eXecutable".
    if (!Args.hasArgNoClaim(options::OPT_T)) {
      CmdArgs.push_back("--default-script");
      CmdArgs.push_back(Static   ? "static.script"
                        : Shared ? "prx.script"
                                 : "main.script");
    }
  }

  if (Static)
    CmdArgs.push_back("-static");
  if (Args.hasArg(options::OPT_rdynamic))
    CmdArgs.push_back("-export-dynamic");
  if (Shared)
    CmdArgs.push_back("--shared");

  // Provide a base address for non-PIE executables. This includes cases where
  // -static is supplied without -pie.
  if (!Relocatable && !Shared && !PIE)
    CmdArgs.push_back("--image-base=0x400000");

  assert((Output.isFilename() || Output.isNothing()) && "Invalid output.");
  if (Output.isFilename()) {
    CmdArgs.push_back("-o");
    CmdArgs.push_back(Output.getFilename());
  }

  const bool UseJMC =
      Args.hasFlag(options::OPT_fjmc, options::OPT_fno_jmc, false);

  auto AddLTOFlag = [&](Twine Flag) {
    CmdArgs.push_back(Args.MakeArgString(Twine("-plugin-opt=") + Flag));
  };

  // If the linker sees bitcode objects it will perform LTO. We can't tell
  // whether or not that will be the case at this point. So, unconditionally
  // pass LTO options to ensure proper codegen, metadata production, etc if
  // LTO indeed occurs.
  if (Args.hasFlag(options::OPT_funified_lto, options::OPT_fno_unified_lto,
                   true))
    CmdArgs.push_back(D.getLTOMode() == LTOK_Thin ? "--lto=thin"
                                                  : "--lto=full");

  AddLTOFlag("-emit-jump-table-sizes-section");

  if (UseJMC)
    AddLTOFlag("-enable-jmc-instrument");

  if (Args.hasFlag(options::OPT_fstack_size_section,
                   options::OPT_fno_stack_size_section, false))
    AddLTOFlag("-stack-size-section");

  if (Arg *A = Args.getLastArg(options::OPT_fcrash_diagnostics_dir))
    AddLTOFlag(Twine("-crash-diagnostics-dir=") + A->getValue());

  if (StringRef Jobs = getLTOParallelism(Args, D); !Jobs.empty())
    AddLTOFlag(Twine("jobs=") + Jobs);

  Args.AddAllArgs(CmdArgs, options::OPT_L);
  TC.AddFilePathLibArgs(Args, CmdArgs);
  Args.addAllArgs(CmdArgs,
                  {options::OPT_T_Group, options::OPT_s, options::OPT_t});

  if (Args.hasArg(options::OPT_Z_Xlinker__no_demangle))
    CmdArgs.push_back("--no-demangle");

  // Sanitizer runtimes must be supplied before all other objects and libs.
  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs))
    TC.addSanitizerArgs(Args, CmdArgs, "-l", "");

  const bool AddStartFiles =
      !Relocatable &&
      !Args.hasArg(options::OPT_nostartfiles, options::OPT_nostdlib);

  auto AddCRTObject = [&](StringRef Name) {
    // CRT objects can be found on user supplied library paths. This is
    // an entrenched expectation on PlayStation.
    CmdArgs.push_back(Args.MakeArgString("-l:" + Name));
  };

  if (AddStartFiles) {
    if (!Shared)
      AddCRTObject("crt1.o");
    AddCRTObject("crti.o");
    AddCRTObject(Shared   ? "crtbeginS.o"
                 : Static ? "crtbeginT.o"
                          : "crtbegin.o");
  }

  AddLinkerInputs(TC, Inputs, Args, CmdArgs, JA);

  if (!Relocatable &&
      !Args.hasArg(options::OPT_nodefaultlibs, options::OPT_nostdlib)) {

    if (UseJMC) {
      CmdArgs.push_back("--push-state");
      CmdArgs.push_back("--whole-archive");
      CmdArgs.push_back("-lSceJmc_nosubmission");
      CmdArgs.push_back("--pop-state");
    }

    if (Args.hasArg(options::OPT_pthread))
      CmdArgs.push_back("-lpthread");

    if (Static) {
      if (!Args.hasArg(options::OPT_nostdlibxx))
        CmdArgs.push_back("-lstdc++");
      if (!Args.hasArg(options::OPT_nolibc)) {
        CmdArgs.push_back("-lm");
        CmdArgs.push_back("-lc");
      }

      CmdArgs.push_back("-lcompiler_rt");
      CmdArgs.push_back("-lkernel");
    } else {
      // The C and C++ libraries are combined.
      if (!Args.hasArg(options::OPT_nolibc, options::OPT_nostdlibxx))
        CmdArgs.push_back("-lc_stub_weak");

      CmdArgs.push_back("-lkernel_stub_weak");
    }
  }
  if (AddStartFiles) {
    AddCRTObject(Shared ? "crtendS.o" : "crtend.o");
    AddCRTObject("crtn.o");
  }

  if (Args.hasArg(options::OPT_fuse_ld_EQ)) {
    D.Diag(diag::err_drv_unsupported_opt_for_target)
        << "-fuse-ld" << TC.getTriple().str();
  }

  std::string LdName = TC.qualifyPSCmdName(TC.getLinkerBaseName());
  const char *Exec = Args.MakeArgString(TC.GetProgramPath(LdName.c_str()));

  C.addCommand(std::make_unique<Command>(JA, *this,
                                         ResponseFileSupport::AtFileUTF8(),
                                         Exec, CmdArgs, Inputs, Output));
}

toolchains::PS4PS5Base::PS4PS5Base(const Driver &D, const llvm::Triple &Triple,
                                   const ArgList &Args, StringRef Platform,
                                   const char *EnvVar)
    : Generic_ELF(D, Triple, Args) {
  // Determine the baseline SDK directory from the environment, else
  // the driver's location, which should be <SDK_DIR>/host_tools/bin.
  SmallString<128> SDKRootDir;
  SmallString<80> Whence;
  if (const char *EnvValue = getenv(EnvVar)) {
    SDKRootDir = EnvValue;
    Whence = {"environment variable '", EnvVar, "'"};
  } else {
    SDKRootDir = D.Dir + "/../../";
    Whence = "compiler's location";
  }

  // Allow --sysroot= to override the root directory for header and library
  // search, and -isysroot to override header search. If both are specified,
  // -isysroot overrides --sysroot for header search.
  auto OverrideRoot = [&](const options::ID &Opt, std::string &Root,
                          StringRef Default) {
    if (const Arg *A = Args.getLastArg(Opt)) {
      Root = A->getValue();
      if (!llvm::sys::fs::exists(Root))
        D.Diag(clang::diag::warn_missing_sysroot) << Root;
      return true;
    }
    Root = Default.str();
    return false;
  };

  bool CustomSysroot =
      OverrideRoot(options::OPT__sysroot_EQ, SDKLibraryRootDir, SDKRootDir);
  bool CustomISysroot =
      OverrideRoot(options::OPT_isysroot, SDKHeaderRootDir, SDKLibraryRootDir);

  // Emit warnings if parts of the SDK are missing, unless the user has taken
  // control of header or library search. If we're not linking, don't check
  // for missing libraries.
  auto CheckSDKPartExists = [&](StringRef Dir, StringRef Desc) {
    if (llvm::sys::fs::exists(Dir))
      return true;
    D.Diag(clang::diag::warn_drv_unable_to_find_directory_expected)
        << (Twine(Platform) + " " + Desc).str() << Dir << Whence;
    return false;
  };

  bool Linking = !Args.hasArg(options::OPT_E, options::OPT_c, options::OPT_S,
                              options::OPT_emit_ast);
  if (Linking) {
    SmallString<128> Dir(SDKLibraryRootDir);
    llvm::sys::path::append(Dir, "target/lib");
    if (CheckSDKPartExists(Dir, "system libraries"))
      getFilePaths().push_back(std::string(Dir));
  }
  if (!CustomSysroot && !CustomISysroot &&
      !Args.hasArg(options::OPT_nostdinc, options::OPT_nostdlibinc)) {
    SmallString<128> Dir(SDKHeaderRootDir);
    llvm::sys::path::append(Dir, "target/include");
    CheckSDKPartExists(Dir, "system headers");
  }

  getFilePaths().push_back(".");
}

void toolchains::PS4PS5Base::AddClangSystemIncludeArgs(
    const ArgList &DriverArgs, ArgStringList &CC1Args) const {
  const Driver &D = getDriver();

  if (DriverArgs.hasArg(options::OPT_nostdinc))
    return;

  if (!DriverArgs.hasArg(options::OPT_nobuiltininc)) {
    SmallString<128> Dir(D.ResourceDir);
    llvm::sys::path::append(Dir, "include");
    addSystemInclude(DriverArgs, CC1Args, Dir.str());
  }

  if (DriverArgs.hasArg(options::OPT_nostdlibinc))
    return;

  addExternCSystemInclude(DriverArgs, CC1Args,
                          SDKHeaderRootDir + "/target/include");
  addExternCSystemInclude(DriverArgs, CC1Args,
                          SDKHeaderRootDir + "/target/include_common");
}

Tool *toolchains::PS4CPU::buildAssembler() const {
  return new tools::PScpu::Assembler(*this);
}

Tool *toolchains::PS4CPU::buildLinker() const {
  return new tools::PS4cpu::Linker(*this);
}

Tool *toolchains::PS5CPU::buildAssembler() const {
  // PS5 does not support an external assembler.
  getDriver().Diag(clang::diag::err_no_external_assembler);
  return nullptr;
}

Tool *toolchains::PS5CPU::buildLinker() const {
  return new tools::PS5cpu::Linker(*this);
}

SanitizerMask toolchains::PS4PS5Base::getSupportedSanitizers() const {
  SanitizerMask Res = ToolChain::getSupportedSanitizers();
  Res |= SanitizerKind::Address;
  Res |= SanitizerKind::PointerCompare;
  Res |= SanitizerKind::PointerSubtract;
  Res |= SanitizerKind::Vptr;
  return Res;
}

SanitizerMask toolchains::PS5CPU::getSupportedSanitizers() const {
  SanitizerMask Res = PS4PS5Base::getSupportedSanitizers();
  Res |= SanitizerKind::Thread;
  return Res;
}

void toolchains::PS4PS5Base::addClangTargetOptions(
    const ArgList &DriverArgs, ArgStringList &CC1Args,
    Action::OffloadKind DeviceOffloadingKind) const {
  // PS4/PS5 do not use init arrays.
  if (DriverArgs.hasArg(options::OPT_fuse_init_array)) {
    Arg *A = DriverArgs.getLastArg(options::OPT_fuse_init_array);
    getDriver().Diag(clang::diag::err_drv_unsupported_opt_for_target)
        << A->getAsString(DriverArgs) << getTriple().str();
  }

  CC1Args.push_back("-fno-use-init-array");

  // Default to `hidden` visibility for PS5.
  if (getTriple().isPS5() &&
      !DriverArgs.hasArg(options::OPT_fvisibility_EQ,
                         options::OPT_fvisibility_ms_compat))
    CC1Args.push_back("-fvisibility=hidden");

  // Default to -fvisibility-global-new-delete=source for PS5.
  if (getTriple().isPS5() &&
      !DriverArgs.hasArg(options::OPT_fvisibility_global_new_delete_EQ,
                         options::OPT_fvisibility_global_new_delete_hidden))
    CC1Args.push_back("-fvisibility-global-new-delete=source");

  const Arg *A =
      DriverArgs.getLastArg(options::OPT_fvisibility_from_dllstorageclass,
                            options::OPT_fno_visibility_from_dllstorageclass);
  if (!A ||
      A->getOption().matches(options::OPT_fvisibility_from_dllstorageclass)) {
    CC1Args.push_back("-fvisibility-from-dllstorageclass");

    if (DriverArgs.hasArg(options::OPT_fvisibility_dllexport_EQ))
      DriverArgs.AddLastArg(CC1Args, options::OPT_fvisibility_dllexport_EQ);
    else
      CC1Args.push_back("-fvisibility-dllexport=protected");

    // For PS4 we override the visibilty of globals definitions without
    // dllimport or  dllexport annotations.
    if (DriverArgs.hasArg(options::OPT_fvisibility_nodllstorageclass_EQ))
      DriverArgs.AddLastArg(CC1Args,
                            options::OPT_fvisibility_nodllstorageclass_EQ);
    else if (getTriple().isPS4())
      CC1Args.push_back("-fvisibility-nodllstorageclass=hidden");
    else
      CC1Args.push_back("-fvisibility-nodllstorageclass=keep");

    if (DriverArgs.hasArg(options::OPT_fvisibility_externs_dllimport_EQ))
      DriverArgs.AddLastArg(CC1Args,
                            options::OPT_fvisibility_externs_dllimport_EQ);
    else
      CC1Args.push_back("-fvisibility-externs-dllimport=default");

    // For PS4 we override the visibilty of external globals without
    // dllimport or  dllexport annotations.
    if (DriverArgs.hasArg(
            options::OPT_fvisibility_externs_nodllstorageclass_EQ))
      DriverArgs.AddLastArg(
          CC1Args, options::OPT_fvisibility_externs_nodllstorageclass_EQ);
    else if (getTriple().isPS4())
      CC1Args.push_back("-fvisibility-externs-nodllstorageclass=default");
    else
      CC1Args.push_back("-fvisibility-externs-nodllstorageclass=keep");
  }

  // Enable jump table sizes section for PS5.
  if (getTriple().isPS5()) {
    CC1Args.push_back("-mllvm");
    CC1Args.push_back("-emit-jump-table-sizes-section");
  }
}

// PS4 toolchain.
toolchains::PS4CPU::PS4CPU(const Driver &D, const llvm::Triple &Triple,
                           const llvm::opt::ArgList &Args)
    : PS4PS5Base(D, Triple, Args, "PS4", "SCE_ORBIS_SDK_DIR") {}

// PS5 toolchain.
toolchains::PS5CPU::PS5CPU(const Driver &D, const llvm::Triple &Triple,
                           const llvm::opt::ArgList &Args)
    : PS4PS5Base(D, Triple, Args, "PS5", "SCE_PROSPERO_SDK_DIR") {}
