//===--- ClangFormatIgnore.h ------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file declares isIgnored() that checks whether a file should be ignored
/// by clang-format.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_CLANGFORMAT_CLANGFORMATIGNORE_H
#define LLVM_CLANG_TOOLS_CLANGFORMAT_CLANGFORMATIGNORE_H

#include "llvm/ADT/StringRef.h"

namespace clang {
namespace format {

bool isIgnored(const llvm::StringRef FilePath);

} // end namespace format
} // end namespace clang

#endif
