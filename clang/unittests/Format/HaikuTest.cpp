//===- unittest/Format/HaikuTest.cpp --------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FormatTestBase.h"

#define verifyHaiku(...) _verifyHaikuFormat(__FILE__, __LINE__, __VA_ARGS__)

namespace clang {
namespace format {
namespace test {
namespace {

class HaikuTest : public FormatTestBase {
  const FormatStyle Style{getHaikuStyle()};

protected:
  void _verifyHaikuFormat(const char *File, int Line, StringRef Expected,
                          StringRef Code,
                          const std::vector<tooling::Range> &Ranges = {}) {
    testing::ScopedTrace t(File, Line, testing::Message() << Code.str());
    EXPECT_EQ(Expected.str(),
              format(Expected, Style, SC_ExpectComplete, Ranges))
        << "Expected code is not stable";
    EXPECT_EQ(Expected.str(), format(Code, Style, SC_ExpectComplete, Ranges));
    EXPECT_EQ(Expected.str(),
              format(messUp(Code), Style, SC_ExpectComplete, Ranges));
  }

  void _verifyHaikuFormat(const char *File, int Line, StringRef Code) {
    _verifyHaikuFormat(File, Line, Code, Code);
  }
};

TEST_F(HaikuTest, BreakConstructorInitializers) {
  verifyHaiku("Foo::Foo(int32 param)\n"
              "\t:\n"
              "\tfMember(param),\n"
              "\tfPointerMember(NULL)\n"
              "{\n"
              "}\n");

  verifyHaiku("class BreakCtorInitializers\n"
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
              ": a(i), b(j) {}\n");
}

} // namespace
} // namespace test
} // namespace format
} // namespace clang
