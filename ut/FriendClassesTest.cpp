#include "../FriendStats.hpp"
#include "Fixture.hpp"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;

struct FriendClassesStats : FriendStats {};

TEST_F(FriendClassesStats, First) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  friend class B;
};
class B {
  void func(A &a) {
    a.a = 1;
    a.b = 2;
  }
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassCount, 1);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{1});
  ASSERT_EQ(res.ClassResults.begin()->second.size(), std::size_t{1});
  ASSERT_EQ(res.ClassResults.begin()->second.at(0).memberFuncResults.size(),
            std::size_t{1});
  auto fr = getFirstMemberFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

TEST_F(FriendClassesStats, SeveralMemberFunctions) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  friend class B;
};
class B {
  void func(A &a) {
    a.a = 1;
    a.b = 2;
  }
  void func2(A &a) {
    a.a = 1;
  }
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassCount, 1);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{1});
  ASSERT_EQ(res.ClassResults.begin()->second.size(), std::size_t{1});
  ASSERT_EQ(res.ClassResults.begin()->second.at(0).memberFuncResults.size(),
            std::size_t{2});
  auto fr = getFirstMemberFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
  fr =
      res.ClassResults.begin()->second.at(0).memberFuncResults.at(1).funcResult;
  EXPECT_EQ(fr.usedPrivateVarsCount, 1);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

TEST_F(FriendClassesStats, SeveralFriendClasses) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  friend class B;
  friend class C;
};
class B {
  void func(A &a) {
    a.a = 1;
    a.b = 2;
  }
};
class C {
  void func(A &a) {
    a.a = 1;
  }
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassCount, 2);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{2});

  ASSERT_EQ(res.ClassResults.begin()->second.at(0).memberFuncResults.size(),
            std::size_t{1});
  auto fr = getFirstMemberFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);

  // 2nd memberFuncResult
  const auto &memberFuncResults =
      (++res.ClassResults.begin())->second.at(0).memberFuncResults;
  ASSERT_EQ(memberFuncResults.size(), std::size_t{1});
  fr = memberFuncResults.at(0).funcResult;
  EXPECT_EQ(fr.usedPrivateVarsCount, 1);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

TEST_F(FriendClassesStats, TemplateHostClass) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
template <class T>
class A {
  int a = 0;
  int b;
  int c;
  friend class B;
};
class B {
  void func(A<int> &a) {
    a.a = 1;
    a.b = 2;
  }
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassCount, 1);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{1});
  ASSERT_EQ(res.ClassResults.begin()->second.at(0).memberFuncResults.size(),
            std::size_t{1});
  auto fr = getFirstMemberFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

TEST_F(FriendClassesStats, TemplateFriendClass) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  template <class T>
  friend class B;
};
template <class T>
class B {
  void func(A &a) {
    a.a = 1;
    a.b = 2;
  }
};
template class B<int>;
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassCount, 1);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{1});
  ASSERT_EQ(res.ClassResults.begin()->second.at(0).memberFuncResults.size(),
            std::size_t{1});
  auto fr = getFirstMemberFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

TEST_F(FriendClassesStats, TemplateFriendClasses) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  template <class T>
  friend class B;
  template <class T>
  friend class C;
};
template <class T>
class B {
  void func(A &a) {
    a.a = 1;
    a.b = 2;
  }
};
template <class T>
class C {
  void func(A &a) {
    a.a = 1;
  }
};
template class B<int>;
template class C<int>;
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassCount, 2);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{2});

  ASSERT_EQ(res.ClassResults.begin()->second.at(0).memberFuncResults.size(),
            std::size_t{1});
  auto fr = getFirstMemberFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);

  const Result::ClassResultsForOneFriendDecl &secondFriendDeclClassResults =
      (++res.ClassResults.begin())->second;
  ASSERT_EQ(secondFriendDeclClassResults.size(), std::size_t{1});
  // 2nd memberFuncResult
  const auto &memberFuncResults =
      secondFriendDeclClassResults.at(0).memberFuncResults;
  fr = memberFuncResults.at(0).funcResult;
  EXPECT_EQ(fr.usedPrivateVarsCount, 1);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

