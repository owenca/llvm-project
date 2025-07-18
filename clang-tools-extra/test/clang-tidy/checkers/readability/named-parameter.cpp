// RUN: %check_clang_tidy %s readability-named-parameter %t
// RUN: %check_clang_tidy -check-suffix=PLAIN-NAMES %s readability-named-parameter %t -- \
// RUN:   -config="{CheckOptions: [{key: readability-named-parameter.InsertPlainNamesInForwardDecls, value: true}]}"

void Method(char *) { /* */ }
// CHECK-MESSAGES: :[[@LINE-1]]:19: warning: all parameters should be named in a function
// CHECK-FIXES: void Method(char * /*unused*/) { /* */ }
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:19: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: void Method(char * /*unused*/) { /* */ }
void Method2(char *) {}
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: all parameters should be named in a function
// CHECK-FIXES: void Method2(char * /*unused*/) {}
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:20: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: void Method2(char * /*unused*/) {}
void Method3(char *, void *) {}
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: all parameters should be named in a function
// CHECK-FIXES: void Method3(char * /*unused*/, void * /*unused*/) {}
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:20: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: void Method3(char * /*unused*/, void * /*unused*/) {}
void Method4(char *, int /*unused*/) {}
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: all parameters should be named in a function
// CHECK-FIXES: void Method4(char * /*unused*/, int /*unused*/) {}
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:20: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: void Method4(char * /*unused*/, int /*unused*/) {}
void operator delete[](void *) throw() {}
// CHECK-MESSAGES: :[[@LINE-1]]:30: warning: all parameters should be named in a function
// CHECK-FIXES: void operator delete[](void * /*unused*/) throw() {}
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:30: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: void operator delete[](void * /*unused*/) throw() {}
int Method5(int) { return 0; }
// CHECK-MESSAGES: :[[@LINE-1]]:16: warning: all parameters should be named in a function
// CHECK-FIXES: int Method5(int /*unused*/) { return 0; }
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:16: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: int Method5(int /*unused*/) { return 0; }
void Method6(void (*)(void *)) {}
// CHECK-MESSAGES: :[[@LINE-1]]:21: warning: all parameters should be named in a function
// CHECK-FIXES: void Method6(void (* /*unused*/)(void *)) {}
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:21: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: void Method6(void (* /*unused*/)(void *)) {}
template <typename T> void Method7(T) {}
// CHECK-MESSAGES: :[[@LINE-1]]:37: warning: all parameters should be named in a function
// CHECK-FIXES: template <typename T> void Method7(T /*unused*/) {}
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:37: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: template <typename T> void Method7(T /*unused*/) {}

// Don't warn in macros.
#define M void MethodM(int) {}
M

void operator delete(void *x) throw() {}
void Method7(char * /*x*/) {}
void Method8(char *x) {}
typedef void (*TypeM)(int x);
void operator delete[](void *x) throw();
void operator delete[](void * /*x*/) throw();

struct X {
  void operator++(int) {}
  void operator--(int) {}

  X(X&) = delete;
  X &operator=(X&) = default;

  const int &i;
};

void (*Func1)(void *);
void Func2(void (*func)(void *)) {}
template <void Func(void *)> void Func3() {}

template <typename T>
struct Y {
  void foo(T) {}
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: all parameters should be named in a function
// CHECK-FIXES: void foo(T /*unused*/) {}
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:13: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: void foo(T /*unused*/) {}
};

Y<int> y;
Y<float> z;

struct Base {
  virtual void foo(bool notThisOne);
  virtual void foo(int argname);
};

struct Derived : public Base {
  void foo(int);
// CHECK-MESSAGES: :[[@LINE-1]]:15: warning: all parameters should be named in a function
// CHECK-FIXES: void foo(int /*argname*/);
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:15: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: void foo(int argname);
};

void FDef(int);
// CHECK-MESSAGES: :[[@LINE-1]]:14: warning: all parameters should be named in a function
// CHECK-FIXES: void FDef(int /*n*/);
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:14: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: void FDef(int n);
void FDef(int n) {}

void FDef2(int, int);
// CHECK-MESSAGES: :[[@LINE-1]]:15: warning: all parameters should be named in a function
// CHECK-FIXES: void FDef2(int /*n*/, int /*unused*/);
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:15: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: void FDef2(int n, int /*unused*/);
void FDef2(int n, int) {}
// CHECK-MESSAGES: :[[@LINE-1]]:22: warning: all parameters should be named in a function
// CHECK-FIXES: void FDef2(int n, int /*unused*/) {}
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:22: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: void FDef2(int n, int /*unused*/) {}

void FNoDef(int);

class Z {};
Z the_z;

Z &operator++(Z&) { return the_z; }
// CHECK-MESSAGES: :[[@LINE-1]]:17: warning: all parameters should be named in a function
// CHECK-FIXES: Z &operator++(Z& /*unused*/) { return the_z; }
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:17: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: Z &operator++(Z& /*unused*/) { return the_z; }

Z &operator++(Z&, int) { return the_z; }
// CHECK-MESSAGES: :[[@LINE-1]]:17: warning: all parameters should be named in a function
// CHECK-FIXES: Z &operator++(Z& /*unused*/, int) { return the_z; }
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:17: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: Z &operator++(Z& /*unused*/, int) { return the_z; }

Z &operator--(Z&) { return the_z; }
// CHECK-MESSAGES: :[[@LINE-1]]:17: warning: all parameters should be named in a function
// CHECK-FIXES: Z &operator--(Z& /*unused*/) { return the_z; }
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:17: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: Z &operator--(Z& /*unused*/) { return the_z; }

Z &operator--(Z&, int) { return the_z; }
// CHECK-MESSAGES: :[[@LINE-1]]:17: warning: all parameters should be named in a function
// CHECK-FIXES: Z &operator--(Z& /*unused*/, int) { return the_z; }
// CHECK-MESSAGES-PLAIN-NAMES: :[[@LINE-3]]:17: warning: all parameters should be named in a function
// CHECK-FIXES-PLAIN-NAMES: Z &operator--(Z& /*unused*/, int) { return the_z; }

namespace testing {
namespace internal {
class IgnoredValue {
 public:
  template <typename T>
  IgnoredValue(const T& /* ignored */) {}
};
}
typedef internal::IgnoredValue Unused;
}

using ::testing::Unused;

void MockFunction(Unused, int q, Unused) {
  ++q;
  ++q;
  ++q;
}

namespace std {
typedef decltype(nullptr) nullptr_t;
}

void f(std::nullptr_t) {}

typedef void (F)(int);
F f;
void f(int x) {}

namespace issue_63056
{
  struct S {
    S(const S&);
    S(S&&);

    S& operator=(const S&);
    S& operator=(S&&);
  };

  S::S(const S&) = default;
  S::S(S&&) = default;

  S& S::operator=(const S&) = default;
  S& S::operator=(S&&) = default;
} // namespace issue_63056
