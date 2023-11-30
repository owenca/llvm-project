//===- unittest/Format/HaikuTest.cpp --------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FormatTestBase.h"

#define verifyHaiku(...) _verifyHaiku(__FILE__, __LINE__, __VA_ARGS__)
#define verifyFormatted(...) _verifyFormatted(__FILE__, __LINE__, __VA_ARGS__)

namespace clang {
namespace format {
namespace test {
namespace {

class HaikuTest : public FormatTestBase {
  FormatStyle Style;

public:
  HaikuTest() : Style(getHaikuStyle()) { Style.ColumnLimit = 60; }

protected:
  void _verifyHaiku(const char *File, int Line, StringRef Expected,
                    StringRef Code, bool MessUp = true) {
    testing::ScopedTrace t(File, Line, testing::Message() << Code.str());
    EXPECT_EQ(Expected.str(), format(Expected, Style))
        << "Expected code is not stable";
    EXPECT_EQ(Expected.str(), format(Code, Style));
    if (MessUp)
      EXPECT_EQ(Expected.str(), format(messUp(Code), Style));
  }

  void _verifyHaiku(const char *File, int Line, StringRef Code) {
    _verifyHaiku(File, Line, Code, Code);
  }

  void _verifyFormatted(const char *File, int Line, StringRef Code) {
    _verifyHaiku(File, Line, Code, Code, /*MessUp=*/false);
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

TEST_F(HaikuTest, ContinuationIndent) {
  verifyHaiku("for (int32 i = 0; ar->FindMessage(str, i, &msg) == B_OK;\n"
              "\ti++) {\n"
              "\tf(i);\n"
              "}\n");
}

TEST_F(HaikuTest, CommentsBelowCode) {
  verifyFormatted("return B_BENDIAN_TO_HOST_INT32(*(uint32*) &color);\n"
                  "\t// rgb_color is always in rgba format, no matter what\n"
                  "\t// endian; we always return the value in host endian.\n");
}

TEST_F(HaikuTest, SeparateFunctionDefinitions) {
  verifyHaiku("void\n"
              "f()\n"
              "{\n"
              "}\n"
              "\n"
              "\n"
              "void\n"
              "g()\n"
              "{\n"
              "} // g\n"
              "\n"
              "\n"
              "void\n"
              "h()\n"
              "{\n"
              "}\n"
              "// comment\n"
              "void\n"
              "foo()\n"
              "{\n"
              "}\n",
              "void f() {}\n"
              "void g() {\n"
              "}\t// g\n"
              "\n"
              "void\n"
              "h() {}\n"
              "// comment\n"
              "void foo()\n"
              "{\n"
              "}");
}

TEST_F(HaikuTest, NoSpaceBetweenNewAndLParen) {
  verifyHaiku("return new(std::nothrow) BView(data);\n");
}

TEST_F(HaikuTest, InsertAndRemoveBraces) {
  verifyHaiku("while (a) {\n"
              "\tif (b)\n"
              "\t\tc();\n"
              "}\n",
              "while (a)\n"
              "\tif (b)\n"
              "\t\tc();");

  verifyHaiku("while (a) {\n"
              "\tif (b)\n"
              "\t\tc();\n"
              "}\n",
              "while (a) {\n"
              "\tif (b) {\n"
              "\t\tc();"
              "\t}\n"
              "}");
}

TEST_F(HaikuTest, BinPackArguments) {
  verifyHaiku("BString f(reinterpret_cast<char*>(addressof(*o)),\n"
              "\tdistance(o, r));\n");

  verifyHaiku("BString f{reinterpret_cast<char*>(addressof(*o)),\n"
              "\tdistance(o, r)};\n");
}

} // namespace
} // namespace test
} // namespace format
} // namespace clang
