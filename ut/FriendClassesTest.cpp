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
  ASSERT_EQ(res.friendClassDeclCount, 1);
  ASSERT_EQ(res.ClassResults.size(), 1u);

  const auto &crs = getClassResultsFor1stFriendDecl(res);
  ASSERT_EQ(crs.size(), 1u);
  const auto &cr = get1stClassResult(crs);
  ASSERT_EQ(cr.memberFuncResults.size(), 1u);
  const Result::FuncResult &fr = get1stMemberFuncResult(cr);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

TEST_F(FriendClassesStats, Constructor) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  friend class B;
};
class B {
  B(A &a) {
    a.a = 1;
    a.b = 2;
  }
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassDeclCount, 1);
  ASSERT_EQ(res.ClassResults.size(), 1u);

  const auto &crs = getClassResultsFor1stFriendDecl(res);
  ASSERT_EQ(crs.size(), 1u);
  const auto &cr = get1stClassResult(crs);
  ASSERT_EQ(cr.memberFuncResults.size(), 1u);
  const Result::FuncResult &fr = get1stMemberFuncResult(cr);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

TEST_F(FriendClassesStats, DefaultConstructor) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  struct X{};
  friend class B;
};
class B {
  A::X x;
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassDeclCount, 1);
  ASSERT_EQ(res.ClassResults.size(), 1u);

  const auto &crs = getClassResultsFor1stFriendDecl(res);
  ASSERT_EQ(crs.size(), 1u);
  const auto &cr = get1stClassResult(crs);
  ASSERT_EQ(cr.memberFuncResults.size(), 1u);
  const Result::FuncResult &fr = get1stMemberFuncResult(cr);
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
  ASSERT_EQ(res.friendClassDeclCount, 1);
  ASSERT_EQ(res.ClassResults.size(), 1u);

  const auto &crs = getClassResultsFor1stFriendDecl(res);
  ASSERT_EQ(crs.size(), 1u);
  const auto &cr = get1stClassResult(crs);
  ASSERT_EQ(cr.memberFuncResults.size(), 2u);
  {
    const Result::FuncResult &fr = get1stMemberFuncResult(cr);
    EXPECT_EQ(fr.usedPrivateVarsCount, 2);
    EXPECT_EQ(fr.parentPrivateVarsCount, 3);
  }
  {
    const Result::FuncResult &fr = get2ndMemberFuncResult(cr);
    EXPECT_EQ(fr.usedPrivateVarsCount, 1);
    EXPECT_EQ(fr.parentPrivateVarsCount, 3);
  }
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
  ASSERT_EQ(res.friendClassDeclCount, 2);
  ASSERT_EQ(res.ClassResults.size(), 2u);

  {
    const auto &crs = getClassResultsFor1stFriendDecl(res);
    ASSERT_EQ(crs.size(), 1u);
    const auto &cr = get1stClassResult(crs);
    ASSERT_EQ(cr.memberFuncResults.size(), 1u);
    {
      const Result::FuncResult &fr = get1stMemberFuncResult(cr);
      EXPECT_EQ(fr.usedPrivateVarsCount, 2);
      EXPECT_EQ(fr.parentPrivateVarsCount, 3);
    }
  }
  {
    const auto &crs = getClassResultsFor2ndFriendDecl(res);
    ASSERT_EQ(crs.size(), 1u);
    const auto &cr = get1stClassResult(crs);
    ASSERT_EQ(cr.memberFuncResults.size(), 1u);
    {
      const Result::FuncResult &fr = get1stMemberFuncResult(cr);
      EXPECT_EQ(fr.usedPrivateVarsCount, 1);
      EXPECT_EQ(fr.parentPrivateVarsCount, 3);
    }
  }
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
  ASSERT_EQ(res.friendClassDeclCount, 1);
  ASSERT_EQ(res.ClassResults.size(), 1u);

  const auto &crs = get1stClassResult(getClassResultsFor1stFriendDecl(res));
  ASSERT_EQ(crs.memberFuncResults.size(), 1u);

  const auto& fr = get1stMemberFuncResult(crs);
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
  ASSERT_EQ(res.friendClassDeclCount, 1);
  ASSERT_EQ(res.ClassResults.size(), 1u);

  const auto &crs = get1stClassResult(getClassResultsFor1stFriendDecl(res));
  ASSERT_EQ(crs.memberFuncResults.size(), 1u);

  const auto& fr = get1stMemberFuncResult(crs);
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
  ASSERT_EQ(res.friendClassDeclCount, 2);
  ASSERT_EQ(res.ClassResults.size(), 2u);

  {
    const auto &crs = getClassResultsFor1stFriendDecl(res);
    ASSERT_EQ(crs.size(), 1u);
    const auto &cr = get1stClassResult(crs);
    ASSERT_EQ(cr.memberFuncResults.size(), 1u);
    {
      const Result::FuncResult &fr = get1stMemberFuncResult(cr);
      EXPECT_EQ(fr.usedPrivateVarsCount, 2);
      EXPECT_EQ(fr.parentPrivateVarsCount, 3);
    }
  }
  {
    const auto &crs = getClassResultsFor2ndFriendDecl(res);
    ASSERT_EQ(crs.size(), 1u);
    const auto &cr = get1stClassResult(crs);
    ASSERT_EQ(cr.memberFuncResults.size(), 1u);
    {
      const Result::FuncResult &fr = get1stMemberFuncResult(cr);
      EXPECT_EQ(fr.usedPrivateVarsCount, 1);
      EXPECT_EQ(fr.parentPrivateVarsCount, 3);
    }
  }
}

TEST_F(FriendClassesStats, TemplateFriendClassSpecializations) {
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
template <>
class B<char> {
  void foo(A &a) {
    a.a = 1;
  }
};
template class B<int>;
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassDeclCount, 1);
  ASSERT_EQ(res.ClassResults.size(), 1u);

  const auto &crs = getClassResultsFor1stFriendDecl(res);
  ASSERT_EQ(crs.size(), 2u);
  {
    // specialization with char
    const auto &cr = get1stClassResult(crs);
    ASSERT_EQ(cr.memberFuncResults.size(), 1u);
    {
      const Result::FuncResult &fr = get1stMemberFuncResult(cr);
      EXPECT_EQ(fr.usedPrivateVarsCount, 1);
      EXPECT_EQ(fr.parentPrivateVarsCount, 3);
    }
  }
  {
    // specialization with int
    const auto &cr = get2ndClassResult(crs);
    ASSERT_EQ(cr.memberFuncResults.size(), 1u);
    {
      const Result::FuncResult &fr = get1stMemberFuncResult(cr);
      EXPECT_EQ(fr.usedPrivateVarsCount, 2);
      EXPECT_EQ(fr.parentPrivateVarsCount, 3);
    }
  }
}

TEST_F(FriendClassesStats, TemplateFriendClassWithTemplateFunction) {
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
  template <class U>
  void func(A &a) {
    a.a = 1;
    a.b = 2;
  }
};
// explicit instantiation of class B
template class B<int>;
// explicit instantiation of func<char> of B<int>
template void B<int>::func<char>(A &a);
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassDeclCount, 1);
  ASSERT_EQ(res.ClassResults.size(), 1u);

  const auto &crs = getClassResultsFor1stFriendDecl(res);
  ASSERT_EQ(crs.size(), 1u);
  const auto &cr = get1stClassResult(crs);
  ASSERT_EQ(cr.memberFuncResults.size(), 1u);
  {
    const Result::FuncResult &fr = get1stMemberFuncResult(cr);
    EXPECT_EQ(fr.usedPrivateVarsCount, 2);
    EXPECT_EQ(fr.parentPrivateVarsCount, 3);
  }
}

TEST_F(FriendClassesStats, TemplateFriendClassWithTemplateFunctionSpecs) {
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
  template <class U>
  void func(A &a) {
    a.a = 1;
    a.b = 2;
  }
};
// explicit instantiation of class B
template class B<int>;
// explicit instantiations of func
template void B<int>::func<char>(A &a);
template void B<int>::func<float>(A &a);
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassDeclCount, 1);
  ASSERT_EQ(res.ClassResults.size(), 1u);

  const auto &crs = getClassResultsFor1stFriendDecl(res);
  ASSERT_EQ(crs.size(), 1u);
  const auto &cr = get1stClassResult(crs);
  ASSERT_EQ(cr.memberFuncResults.size(), 2u);
  {
    const Result::FuncResult &fr = get1stMemberFuncResult(cr);
    EXPECT_EQ(fr.usedPrivateVarsCount, 2);
    EXPECT_EQ(fr.parentPrivateVarsCount, 3);
  }
  {
    const Result::FuncResult &fr = get2ndMemberFuncResult(cr);
    EXPECT_EQ(fr.usedPrivateVarsCount, 2);
    EXPECT_EQ(fr.parentPrivateVarsCount, 3);
  }
}

TEST_F(FriendClassesStats, NestedClassOfFriendClass) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  friend class B;
};
class B {
  class C {
    void func(A &a) {
      a.a = 1;
      a.b = 2;
    }
  };
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassDeclCount, 1);
  ASSERT_EQ(res.ClassResults.size(), 1u);

  const auto &crs = getClassResultsFor1stFriendDecl(res);
  ASSERT_EQ(crs.size(), 2u);
  {
    const auto &cr = get1stClassResult(crs);
    EXPECT_EQ(cr.memberFuncResults.size(), 0u);
  }
  {
    const auto &cr = get2ndClassResult(crs);
    ASSERT_EQ(cr.memberFuncResults.size(), 1u);
    {
      const Result::FuncResult &fr = get1stMemberFuncResult(cr);
      EXPECT_EQ(fr.usedPrivateVarsCount, 2);
      EXPECT_EQ(fr.parentPrivateVarsCount, 3);
    }
  }
}

TEST_F(FriendClassesStats, NestedClassTemplateOfFriendClass) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  friend class B;
};
class B {
  template <typename T>
  class C {
    void func(A &a) {
      a.a = 1;
      a.b = 2;
    }
  };
};
template class B::C<int>;
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassDeclCount, 1);
  ASSERT_EQ(res.ClassResults.size(), 1u);

  const auto &crs = getClassResultsFor1stFriendDecl(res);
  ASSERT_EQ(crs.size(), 2u);
  {
    const auto &cr = get1stClassResult(crs);
    EXPECT_EQ(cr.memberFuncResults.size(), 0u);
  }
  {
    const auto &cr = get2ndClassResult(crs);
    ASSERT_EQ(cr.memberFuncResults.size(), 1u);
    {
      const Result::FuncResult &fr = get1stMemberFuncResult(cr);
      EXPECT_EQ(fr.usedPrivateVarsCount, 2);
      EXPECT_EQ(fr.parentPrivateVarsCount, 3);
    }
  }
}

TEST_F(FriendClassesStats, NestedClassTemplateOfFriendClassTemplate) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  template <typename T>
  friend class B;
};
template <typename T>
class B {
  template <typename U>
  class C {
    void func(A &a) {
      a.a = 1;
      a.b = 2;
    }
  };
};
template class B<int>;
template class B<int>::C<int>;
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassDeclCount, 1);
  ASSERT_EQ(res.ClassResults.size(), 1u);

  const auto &crs = getClassResultsFor1stFriendDecl(res);
  ASSERT_EQ(crs.size(), 2u);
  {
    const auto &cr = get1stClassResult(crs);
    EXPECT_EQ(cr.memberFuncResults.size(), 0u);
  }
  {
    const auto &cr = get2ndClassResult(crs);
    ASSERT_EQ(cr.memberFuncResults.size(), 1u);
    {
      const Result::FuncResult &fr = get1stMemberFuncResult(cr);
      EXPECT_EQ(fr.usedPrivateVarsCount, 2);
      EXPECT_EQ(fr.parentPrivateVarsCount, 3);
    }
  }
}

TEST_F(FriendClassesStats, DeeplyNestedClass) {
  Tool->mapVirtualFile(FileA,
                       R"phi(
class A {
  int a = 0;
  int b;
  int c;
  friend class B;
};
class B {
  class C {
    class D {
      void func(A &a) {
        a.a = 1;
        a.b = 2;
      }
    };
  };
};
    )phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Handler.getResult();
  ASSERT_EQ(res.friendClassDeclCount, 1);
  ASSERT_EQ(res.ClassResults.size(), 1u);

  const auto &crs = getClassResultsFor1stFriendDecl(res);
  ASSERT_EQ(crs.size(), 3u);
  {
    const auto &cr = get1stClassResult(crs);
    EXPECT_EQ(cr.memberFuncResults.size(), 0u);
  }
  {
    const auto &cr = get2ndClassResult(crs);
    EXPECT_EQ(cr.memberFuncResults.size(), 0u);
  }
  {
    const auto &cr = get3rdClassResult(crs);
    ASSERT_EQ(cr.memberFuncResults.size(), 1u);
    {
      const Result::FuncResult &fr = get1stMemberFuncResult(cr);
      EXPECT_EQ(fr.usedPrivateVarsCount, 2);
      EXPECT_EQ(fr.parentPrivateVarsCount, 3);
    }
  }
}

