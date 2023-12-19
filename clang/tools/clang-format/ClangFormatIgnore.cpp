//===-- ClangFormatIgnore.cpp - Specify files clang-format should ignore --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements the functionality of matching a file path name to
/// patterns in a .clang-format-ignore file to see whether clang-format should
/// ignore it.
///
//===----------------------------------------------------------------------===//

#include "llvm/Support/VirtualFileSystem.h"
#include <fstream>

using namespace llvm;

namespace clang {
namespace format {

// Check whether `FilePath` matches `Pattern` based on POSIX Section 2.13.
static bool matchFilePath(const StringRef Pattern, const StringRef FilePath) {
  assert(!Pattern.empty());
  assert(!FilePath.empty());

  constexpr auto Separator = '/';
  const auto EOP = Pattern.size();  // End of `Pattern`.
  const auto End = FilePath.size(); // End of `FilePath`.
  unsigned I = 0;                   // Index to `Pattern`.

  // No match if `Pattern` ends with a non-meta character not equal to the last
  // character of `FilePath`.
  if (const auto C = Pattern.back(); !strchr("?*]", C) && C != FilePath.back())
    return false;

  for (const auto &F : FilePath) {
    if (I == EOP)
      return false;

    switch (Pattern[I]) {
    case '\\':
      if (++I == EOP || F != '\\')
        return false;
      ++I;
      break;
    case '?':
      if (F == Separator)
        return false;
      ++I;
      break;
    case '*': {
      unsigned J = &F - FilePath.data(); // Index of `F`.
      // Skip consecutive stars.
      do {
        if (++I == EOP)
          return FilePath.find(Separator, J + 1) == StringRef::npos;
      } while (Pattern[I] == '*');
      while (FilePath[J] != Separator) {
        if (matchFilePath(Pattern.substr(I), FilePath.substr(J)))
          return true;
        if (++J == End)
          return false;
      }
      break;
    }
    case '[':
      // Skip e.g. `[!]`.
      if (I + 3 < EOP || (I + 3 == EOP && Pattern[I + 1] != '!')) {
        // Skip unpaired `[`, brackets containing slashes, and `[]`.
        if (const auto J = Pattern.find_first_of("]/", I + 1);
            J != StringRef::npos && Pattern[J] == ']' && J > I + 1) {
          if (F == Separator)
            return false;
          ++I; // After the `[`.
          bool Negated = false;
          if (Pattern[I] == '!') {
            Negated = true;
            ++I; // After the `!`.
          }
          bool Match = false;
          do {
            if (I + 2 < J && Pattern[I + 1] == '-') {
              Match = Pattern[I] <= F && F <= Pattern[I + 2];
              I += 3; // After the range, e.g. `A-Z`.
            } else {
              Match = F == Pattern[I++];
            }
          } while (!Match && I < J);
          if (Negated ? Match : !Match)
            return false;
          I = J + 1; // After the `]`.
          break;
        }
      }
      [[fallthrough]]; // Match `[` literally.
    default:
      if (F != Pattern[I])
        return false;
      ++I;
    }
  }

  // Match trailing stars with null strings.
  while (I < EOP && Pattern[I] == '*')
    ++I;

  return I == EOP;
}

// Check whether `FilePath` is ignored according to the nearest
// .clang-format-ignore file based on the rules below:
// - A blank line is skipped.
// - Leading and trailing spaces of a line are trimmed.
// - A line starting with a hash (`#`) is a comment.
// - A non-comment line is a single pattern.
// - The slash (`/`) is used as the directory separator.
// - A pattern is relative to the directory of the .clang-format-ignore file (or
//   the root directory if the pattern starts with a slash).
// - A pattern is negated if it starts with a bang (`!`).
bool isIgnored(const StringRef FilePath) {
  if (!llvm::sys::fs::is_regular_file(FilePath))
    return false;

  using namespace llvm::sys::path;
  SmallString<128> Path, AbsPath{convert_to_slash(FilePath)};

  llvm::vfs::getRealFileSystem()->makeAbsolute(AbsPath);
  remove_dots(AbsPath, /*remove_dot_dot=*/true);

  StringRef IgnoreDir{AbsPath};
  do {
    IgnoreDir = parent_path(IgnoreDir);
    if (IgnoreDir.empty())
      return false;

    Path = IgnoreDir;
    append(Path, ".clang-format-ignore");
  } while (!llvm::sys::fs::is_regular_file(Path));

  std::ifstream IgnoreFile{Path.c_str()};
  if (!IgnoreFile.good())
    return false;

  bool HasMatch = false;
  for (std::string Pattern; std::getline(IgnoreFile, Pattern);) {
    Pattern = StringRef(Pattern).trim();
    if (Pattern.empty() || Pattern[0] == '#')
      continue;

    const bool IsNegated = Pattern[0] == '!';
    if (IsNegated)
      Pattern.erase(0, 1);

    if (Pattern.empty())
      continue;

    Pattern = StringRef(Pattern).ltrim();
    if (Pattern[0] != '/') {
      Path = IgnoreDir;
      append(Path, Pattern);
      remove_dots(Path, /*remove_dot_dot=*/true);
      Pattern = Path.str();
    }

    if (matchFilePath(Pattern.c_str(), AbsPath.c_str()) == !IsNegated) {
      HasMatch = true;
      break;
    }
  }

  IgnoreFile.close();
  return HasMatch;
}

} // namespace format
} // namespace clang
