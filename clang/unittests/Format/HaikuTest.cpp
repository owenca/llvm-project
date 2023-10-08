//===- unittest/Format/HaikuTest.cpp --------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FormatTestBase.h"

#define DEBUG_TYPE "haiku-test"

namespace clang {
namespace format {
namespace test {
namespace {

class HaikuTest : public FormatTestBase {};

TEST_F(HaikuTest, BreakConstructorInitializers) {
  FormatStyle Style = getHaikuStyle();

  verifyFormat("Foo::Foo(int32 param)\n"
               "\t:\n"
               "\tfMember(param),\n"
               "\tfPointerMember(NULL)\n"
               "{\n"
               "}\n",
               Style);

  verifyFormat("class BreakCtorInitializers\n"
               "{\n"
               "private:\n"
               "\tint a, b;\n"
               "\n"
               "public:\n"
               "\tBreakCtorInitializers(int i)\n"
               "\t\t:\n"
               "\t\ta(i)\n"
               "\t{\n"
               "\t\tb = 0;\n"
               "\t}\n"
               "\tBreakCtorInitializers(int i, int j);\n"
               "};\n"
               "\n"
               "\n"
               "BreakCtorInitializers::BreakCtorInitializers(int i, int j)\n"
               "\t:\n"
               "\ta(i),\n"
               "\tb(j)\n"
               "{\n"
               "}\n",
               "class BreakCtorInitializers {\n"
               "  private: int a, b;\n"
               "\n"
               "  public:\n"
               "    BreakCtorInitializers(int i)\n"
               "      : a(i) { b = 0; }\n"
               "    BreakCtorInitializers(int i, int j);\n"
               "};\n"
               "\n"
               "BreakCtorInitializers::BreakCtorInitializers(int i, int j)\n"
               ": a(i), b(j) {}\n",
               Style);
}

} // namespace
} // namespace test
} // namespace format
} // namespace clang
