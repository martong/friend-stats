// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"

#include "../FriendStats.hpp"
#include "Fixture.hpp"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;

TEST_F(FriendStats, ClassCount) {
  Tool->mapVirtualFile(FileA, "class A { friend class B; }; class B {};");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  EXPECT_EQ(res.friendClassCount, 1);
}

TEST_F(FriendStats, ClassCount_ExtendedFriend) {
  Tool->mapVirtualFile(FileA, "class Y {}; class A { friend Y; }; ");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  EXPECT_EQ(res.friendClassCount, 1);
}

TEST_F(FriendStats, FuncCount) {
  Tool->mapVirtualFile(FileA,
                       "class A { friend void func(); }; void func(){};");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  EXPECT_EQ(res.friendFuncCount, 1);
}

TEST_F(FriendStats, NumberOfUsedPrivateOrProtectedVariablesInFriendFunc) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  friend void func(A &);
};
void func(A &a) {
  a.a = 1;
  a.b = 2;
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

TEST_F(FriendStats, LambdaInsideFriendFunction) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  friend void func(A &);
};
void func(A &a) {
  auto l = [](A &a) {
    a.a = 1;
    a.b = 2;
  };
  l(a);
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

// C++ standard does not allow local classes to be templates,
// or to have templates defined inside. (I am so lucky)
TEST_F(FriendStats, LocalClassInFriendFunction) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  friend void func(A &);
};
void func(A &a) {
  struct X {
    void foo(A &a) {
      a.a = 1;
      a.b = 2;
    }
  };
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

TEST_F(FriendStats, NumberOfUsedNestedPrivateOrProtectedVariablesInFriendFunc) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
// The below example is not valid C++, but it is here for completeness.
// The friend function of A cannot access A's nested class' (B) members.
/*
class A {
  class B {
    int x;
    int y;
    int z;
  };
  friend void func(B &);
};
void func(A::B &b) {
  b.x = 1;
  b.y = 2;
};
*/

class A {
  class B {
    int x;
    int y;
    int z;
	friend void func(B &b) {
	  b.x = 1;
	  b.y = 2;
	}
  };
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

TEST_F(FriendStats, NumberOfUsedPrivateOrProtectedVariablesInFriendOpEqual) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  friend bool operator==(A x, A y) {
    return x.a == y.a;
  }
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
  EXPECT_EQ(fr.usedPrivateVarsCount, 1);
}

TEST_F(FriendStats, BugAtExternalASTSource_h_575) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
template <class T>
class XXX {
  public:
class A {
  int a;
  int b;
  int c;
  friend bool operator==(A x, A y) {
    return x.a == y.a;
  }
};
};
void foo() { XXX<int>::A x; bool b = x == x; }
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
  EXPECT_EQ(fr.usedPrivateVarsCount, 1);
}

TEST_F(FriendStats, NumberOfPrivateOrProtectedVariablesInParent) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
public:
  int c;
  friend void func(A &);
};
void func(A &a) {
  a.a = 1;
  a.b = 2;
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.parentPrivateVarsCount, 2);
}

TEST_F(FriendStats, PrivOperatorUsedInFriendFunction) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  void operator+=(A& x) {}
  friend void foo(A& x, A& y) {
    x += y;
  }
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.parentPrivateMethodsCount, 1);
  EXPECT_EQ(fr.usedPrivateMethodsCount, 1);
}

TEST_F(FriendStats, NumberOfUsedPrivateOrProtectedMethodsInFriendFunc) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  void privFunc() {}
  void privFunc(int) {}
  friend void func(A &);
};
void func(A &a) {
  a.privFunc();
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.usedPrivateMethodsCount, 1);
  EXPECT_EQ(fr.parentPrivateMethodsCount, 2);
}

TEST_F(FriendStats, NumberOfUsedPrivateOrProtectedMethodsAndVarsInFriendFunc) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b = 0;
  void privFunc() {}
  void privFunc(int) {}
  friend void func(A &);
};
void func(A &a) {
  a.a = 1;
  a.privFunc();
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 1);
  EXPECT_EQ(fr.parentPrivateVarsCount, 2);
  EXPECT_EQ(fr.usedPrivateMethodsCount, 1);
  EXPECT_EQ(fr.parentPrivateMethodsCount, 2);
}

TEST_F(FriendStats, NumberOfUsedPrivateOrProtectedStaticVariablesInFriend) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  static int a;
  static int b;
  friend void foo() {
    A::a = 1;
  }
};
int A::a = 0;
int A::b = 0;
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.parentPrivateVarsCount, 2);
  EXPECT_EQ(fr.usedPrivateVarsCount, 1);
  EXPECT_EQ(fr.parentPrivateMethodsCount, 0);
  EXPECT_EQ(fr.usedPrivateMethodsCount, 0);
}

TEST_F(FriendStats, NumberOfUsedPrivateOrProtectedStaticMehtodsInFriend) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  static void a() {};
  static void b() {};
  friend void foo() {
    A::a();
  }
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.parentPrivateVarsCount, 0);
  EXPECT_EQ(fr.usedPrivateVarsCount, 0);
  EXPECT_EQ(fr.parentPrivateMethodsCount, 2);
  EXPECT_EQ(fr.usedPrivateMethodsCount, 1);
}

TEST_F(FriendStats,
       NumberOfUsedPrivateOrProtectedStaticTemplateMehtodsInFriend) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  template <typename T>
  static void a() {};
  friend void foo() {
    A::a<int>();
  }
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.parentPrivateVarsCount, 0);
  EXPECT_EQ(fr.usedPrivateVarsCount, 0);
  EXPECT_EQ(fr.parentPrivateMethodsCount, 1);
  EXPECT_EQ(fr.usedPrivateMethodsCount, 1);
}

TEST_F(FriendStats, PublicStaticFunctionsIsNotCounted) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  template <typename T>
  static void a() {};
public:
  static void b() {};
  friend void foo() {
    A::a<int>();
  }
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.parentPrivateVarsCount, 0);
  EXPECT_EQ(fr.usedPrivateVarsCount, 0);
  EXPECT_EQ(fr.parentPrivateMethodsCount, 1);
  EXPECT_EQ(fr.usedPrivateMethodsCount, 1);
}

// ================= Type Tests ============================================= //
struct FriendStatsForTypes : FriendStats {};

TEST_F(FriendStatsForTypes, Variable) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  using Int = int;
  friend void func();
};
void func() {
  A::Int i = 0;
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.types.usedPrivateCount, 1);
}

TEST_F(FriendStatsForTypes, DoNotCountSelf) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int Position;
public:
    typedef int                 difference_type;
    friend A& operator+=(A &X, difference_type D) {
      X.Position += D;
      return X;
    }
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.types.parentPrivateCount, 0);
  EXPECT_EQ(fr.types.usedPrivateCount, 0);
  EXPECT_EQ(fr.usedPrivateVarsCount, 1);
}

TEST_F(FriendStatsForTypes, NestedVariable) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  using Int = int;
  friend void func();
};
void func() {
  struct X {
    A::Int i = 0;
  };
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.types.usedPrivateCount, 1);
}

TEST_F(FriendStatsForTypes, TypeAlias) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  using Int = int;
  using Int2 = int;
  friend void func();
};
void func() {
  using X = A::Int;
  typedef A::Int2 Y;
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.types.usedPrivateCount, 2);
}

TEST_F(FriendStatsForTypes, NestedTypeAlias) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  using Int = int;
  using Int2 = int;
  friend void func();
};
void func() {
  struct S {
    using X = A::Int;
    typedef A::Int2 Y;
  };
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.types.usedPrivateCount, 2);
}

TEST_F(FriendStatsForTypes, FuncParameter) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  using Int = int;
  using Double = double;
  friend A::Double func(Int);
};
A::Double func(A::Int) {
    return 0.0;
}
)phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.types.usedPrivateCount, 2);
}

TEST_F(FriendStatsForTypes, FuncParameterNoDuplicates) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  using Int = int;
  friend A::Int func(Int);
};
A::Int func(A::Int) { return 0; }
)phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.types.usedPrivateCount, 1);
}

TEST_F(FriendStatsForTypes, NestedFunctionParameter) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  using Int = int;
  friend void func();
};
void func() {
  struct X {
	void foo(A::Int);
  };
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.types.usedPrivateCount, 1);
}

TEST_F(FriendStatsForTypes, ParentPrivateCount) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {

  // two private types defined
  using Int = int;
  struct Nested1 {};

  friend void func();

public:

  struct Nested2 {};
  using Int2 = int;
};

void func() {
  A::Int i = 0;
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.types.usedPrivateCount, 1);
  EXPECT_EQ(fr.types.parentPrivateCount, 2);
}

TEST_F(FriendStatsForTypes, ParentPrivateCountComplex) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {

  // two private types defined
  using Int = int;
  struct Nested1 {};

  friend void f();

public:

  struct Nested2 {};
  using Int2 = int;
};

void f() {
  A::Int i = 0;
};

class B {

  // 3 private types defined
  using Int = int;
  struct Nested1 {};
  using Double = double;

  friend void g();

public:

  struct Nested2 {};
  using Int2 = int;
};

void g() {
  B::Int i = 0;
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 2);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{2});
  auto p = res.FuncResults.begin()->second.begin();
  EXPECT_EQ(p->second.types.usedPrivateCount, 1);
  EXPECT_EQ(p->second.types.parentPrivateCount, 2);
  p = (++res.FuncResults.begin())->second.begin();
  EXPECT_EQ(p->second.types.usedPrivateCount, 1);
  EXPECT_EQ(p->second.types.parentPrivateCount, 3);
}

// TEST_F(FriendStatsForTypes, NumberOfUsedPrivateOrProtectedTypesInFriendFunc)
// {
// Tool->mapVirtualFile(FileA,
// R"phi(
// class A {
// using Int = int;
// friend void func(Int);
//};
// void func(A::Int) {
// struct X {
// A::Int i;
// void foo(A::Int);
//};
// using MyInt = A::Int;
// A::Int i = 0;
//};
//)phi");
// Tool->run(newFrontendActionFactory(&Finder).get());
// auto res = Handler.getResult();
// ASSERT_EQ(res.friendFuncCount, 1);
// ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
//}

// ===================== Friend function uses template members ================

TEST_F(FriendStats, PrivTemplateMemberFunctionUsedInFriendFunction) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  template <typename T>
  void foo(T x) {}
  friend void friend_foo(A& x) {
    x.foo(1);
  }
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.parentPrivateMethodsCount, 1);
  EXPECT_EQ(fr.usedPrivateMethodsCount, 1);
}

TEST_F(FriendStats, PrivTemplateMemberOperatorUsedInFriendFunction) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  template <typename T>
  void operator+=(T& x) {}
  friend void foo(A& x, A& y) {
    x += y;
  }
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.parentPrivateMethodsCount, 1);
  EXPECT_EQ(fr.usedPrivateMethodsCount, 1);
}

TEST_F(FriendStats, PrivTemplateMemberVariableUsedInFriendFunction) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  template<class T>
  static constexpr T pi = T(3.1415926535897932385);  // variable template
  friend void friend_foo() {
    int a = A::pi<int>;
  }
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.parentPrivateVarsCount, 1);
  EXPECT_EQ(fr.usedPrivateVarsCount, 1);
}

// ================= Template  Tests ======================================== //
struct TemplateFriendStats : FriendStats {};

TEST_F(TemplateFriendStats, FuncCount) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  template <typename T>
  friend void f(T) {}
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  EXPECT_EQ(res.friendFuncCount, 1);
}

TEST_F(TemplateFriendStats,
       NumberOfUsedPrivateOrProtectedVariablesInFriendFunc) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A;
template <typename T> void func(T, A a);
class A {
  int a = 0;
  int b;
  int c;
  template <typename T>
  friend void func(T, A a) {
    a.a = 1;
    a.b = 2;
  }
};
template void func<int>(int, A);
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

TEST_F(TemplateFriendStats,
       NumberOfUsedPrivateOrProtectedVariablesInFriendFuncOutOfLineDef) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  template <typename T>
  friend void func(T, A& a);
};
template <typename T>
void func(T, A& a) {
  a.a = 1;
  a.b = 2;
}
template void func<int>(int, A&);
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

TEST_F(TemplateFriendStats,
       NumberOfUsedPrivateOrProtectedVariablesInFriendFuncSpecialization) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  template <typename T>
  friend void func(T, A& a);
};
template <typename T>
void func(T, A& a) {
  a.a = 1;
  a.b = 2;
}

// Full(explicit) specialization
// Remember: There is no such thing as partial function template specialization!
// template <typename T> void func<T*>(T*, A&) would be a new base template.
// http://www.gotw.ca/publications/mill17.htm
template <> void func<double>(double, A& a) { a.a = 1; }

// Instantiations
template void func<int>(int, A&);
//template void func<char>(char, A&);
// The below lines are not redundant, because
// the same FunctionTemplateSpecializationInfo* refers to both the explicit
// specialization and the explicit/implicit instantiations.
template void func<double>(double, A&);
void fooo() { A a; func<double>(1.0,a); }
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  EXPECT_EQ(res.FuncResults.begin()->second.size(), std::size_t{2});
  // Go over the instantiations
  auto p = res.FuncResults.begin()->second.begin();
  // TODO query by double/int
  EXPECT_EQ(p->second.usedPrivateVarsCount, 1);
  EXPECT_EQ(p->second.parentPrivateVarsCount, 3);
  ++p;
  // int
  EXPECT_EQ(p->second.usedPrivateVarsCount, 2);
  EXPECT_EQ(p->second.parentPrivateVarsCount, 3);
}

TEST_F(TemplateFriendStats, ClassTemplate) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
template <typename T> class A {
  int a = 0;
  int b;
  int c;
  friend void func(A &a) {
    a.a = 1;
    a.b = 2;
  }
};
void f() { A<int> aint; func(aint); }
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

TEST_F(TemplateFriendStats, ClassTemplateFuncTemplate) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
template <typename T> class A {
  int a = 0;
  int b;
  int c;
  template <typename U>
  friend void func(U, A &a) {
    a.a = 1;
    a.b = 2;
  }
};
void f() { A<int> aint; func(1, aint); }
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

TEST_F(TemplateFriendStats, ClassTemplateFuncTemplateComposite) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
template <typename T> class A {
  int a = 0;
  int b;
  int c;
  friend void func2(A &a) {
    a.a = 1;
  }
  template <typename U>
  friend void func(U, A &a) {
    a.a = 1;
    a.b = 2;
  }
};
void f() { A<int> aint; func(1, aint); func2(aint); }
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 2);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{2});
  auto it = res.FuncResults.begin()->second.begin();
  {
    auto p = *it;
    EXPECT_EQ(p.second.usedPrivateVarsCount, 1);
    EXPECT_EQ(p.second.parentPrivateVarsCount, 3);
  }
  it = (++res.FuncResults.begin())->second.begin();
  {
    auto p = *it;
    EXPECT_EQ(p.second.usedPrivateVarsCount, 2);
    EXPECT_EQ(p.second.parentPrivateVarsCount, 3);
  }
}

TEST_F(TemplateFriendStats, ClassTemplateFriendFunctionUsingPrivTypes) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
template <typename T> class A {
  using X = int;
  using Y = int;
  using Z = int;
  friend void func2(A&) {
    X x; ++x;
  }
};
void f() { A<int> aint; func2(aint); }
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.types.usedPrivateCount, 1);
  EXPECT_EQ(fr.types.parentPrivateCount, 3);
}

TEST_F(TemplateFriendStats,
       ClassTemplatePartialSpecializationFriendFunctionUsingPrivTypes) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
template <typename T> class A {};
template <typename T>
class A<T*> {
  using X = int;
  using Y = int;
  using Z = int;
  friend void func2(A&) {
    X x; ++x;
  }
};
void f() { A<int*> aint; func2(aint); }
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.types.usedPrivateCount, 1);
  EXPECT_EQ(fr.types.parentPrivateCount, 3);
}

TEST_F(TemplateFriendStats, ClassTemplateFriendFunctionTemplateUsingPrivTypes) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
template <typename T> class A {
  using X = int;
  using Y = int;
  using Z = int;
  template <typename U>
  friend Y func(U, A&) {
    X x; ++x;
    Y y; ++y;
    return y;
  }
};
void f() { A<int> aint; func(1, aint); }
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.types.usedPrivateCount, 2);
  EXPECT_EQ(fr.types.parentPrivateCount, 3);
}

TEST_F(TemplateFriendStats, ClassTemplateFuncTemplateOutOfLineDef) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
template <typename T> class A;

template <typename T>
void func(A<T> &a);

template <typename T> class A {
  int a = 0;
  int b;
  int c;

  // refers to a full specialization for this particular T
  friend void func <T> (A &a);
};

template <typename T>
void func(A<T>& a) {
  a.a = 1;
}

template void func(A<int>& a);
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendFuncCount, 1);
  ASSERT_EQ(res.FuncResults.size(), std::size_t{1});
  auto fr = getFirstFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 1);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

// ================= Duplicate Tests ======================================== //

struct FriendStatsHeader : ::testing::Test {
  SmallString<128> CurrentDir;
  SmallString<128> HeaderA;
  SmallString<128> FileA;
  SmallString<128> FileB;
  std::unique_ptr<tooling::FixedCompilationDatabase> Compilations;
  std::unique_ptr<tooling::ClangTool> Tool;
  std::vector<std::string> Sources;
  FriendHandler Handler;
  MatchFinder Finder;
  FriendStatsHeader() {
    // The directory used is not important since the path gets mapped to a
    // virtual
    // file anyway. What is important is that we have an absolute path with
    // which
    // to use with mapVirtualFile().
    std::error_code EC = llvm::sys::fs::current_path(CurrentDir);
    assert(!EC);
    (void)EC;

    HeaderA = CurrentDir;
    llvm::sys::path::append(HeaderA, "a.h");
    FileA = CurrentDir;
    llvm::sys::path::append(FileA, "a.cc");
    FileB = CurrentDir;
    llvm::sys::path::append(FileB, "b.cc");

    Compilations.reset(new tooling::FixedCompilationDatabase(
        CurrentDir.str(), std::vector<std::string>()));
    // Sources.push_back(HeaderA.str());
    Sources.push_back(FileA.str());
    Sources.push_back(FileB.str());
    Tool.reset(new tooling::ClangTool(*Compilations, Sources));

    Finder.addMatcher(FriendMatcher, &Handler);
  }
};

TEST_F(FriendStatsHeader, NoDuplicateCountOnClasses) {
  Tool->mapVirtualFile(HeaderA, "class A { friend class B; }; class B {};");
  Tool->mapVirtualFile(FileA, R"phi(#include "a.h")phi");
  Tool->mapVirtualFile(FileB, R"phi(#include "a.h")phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  EXPECT_EQ(res.friendClassCount, 1);
}

TEST_F(FriendStatsHeader, NoDuplicateCountOnFunctions) {
  Tool->mapVirtualFile(HeaderA,
                       "class A { friend void func(); }; void func(){};");
  Tool->mapVirtualFile(FileA, R"phi(#include "a.h")phi");
  Tool->mapVirtualFile(FileB, R"phi(#include "a.h")phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  EXPECT_EQ(res.friendFuncCount, 1);
}


