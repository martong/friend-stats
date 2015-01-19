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

  //ASSERT_EQ(res.ClassResults.begin()->second.size(), std::size_t{1});
  const Result::ClassResultsForOneFriendDecl &firstFriendDeclClassResults =
      (res.ClassResults.begin())->second;
  ASSERT_EQ(firstFriendDeclClassResults.size(), std::size_t{1});

  ASSERT_EQ(firstFriendDeclClassResults.at(0).memberFuncResults.size(),
            std::size_t{1});
  auto fr = getFirstMemberFuncResult(res);
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
  ASSERT_EQ(res.friendClassCount, 1);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{1});

  //ASSERT_EQ(res.ClassResults.begin()->second.size(), std::size_t{1});
  const Result::ClassResultsForOneFriendDecl &firstFriendDeclClassResults =
      (res.ClassResults.begin())->second;
  ASSERT_EQ(firstFriendDeclClassResults.size(), std::size_t{1});

  ASSERT_EQ(firstFriendDeclClassResults.at(0).memberFuncResults.size(),
            std::size_t{1});
  auto fr = getFirstMemberFuncResult(res);
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
  ASSERT_EQ(res.friendClassCount, 1);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{1});

  //ASSERT_EQ(res.ClassResults.begin()->second.size(), std::size_t{1});
  const Result::ClassResultsForOneFriendDecl &firstFriendDeclClassResults =
      (res.ClassResults.begin())->second;
  ASSERT_EQ(firstFriendDeclClassResults.size(), std::size_t{1});

  ASSERT_EQ(firstFriendDeclClassResults.at(0).memberFuncResults.size(),
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
  ASSERT_EQ(res.friendClassCount, 1);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{1});
  const Result::ClassResultsForOneFriendDecl &firstFriendDeclClassResults =
      (res.ClassResults.begin())->second;
  ASSERT_EQ(firstFriendDeclClassResults.size(), std::size_t{2});

  // specialization with char
  auto fr = getFirstMemberFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 1);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);

  // The 1st member function of the 2nd specialization of template class B
  // instantiation with int
  fr = firstFriendDeclClassResults.at(1).memberFuncResults.at(0).funcResult;
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
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
  ASSERT_EQ(res.friendClassCount, 1);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{1});
  const Result::ClassResultsForOneFriendDecl &firstFriendDeclClassResults =
      (res.ClassResults.begin())->second;
  ASSERT_EQ(firstFriendDeclClassResults.size(), std::size_t{1});
  ASSERT_EQ(firstFriendDeclClassResults.front().memberFuncResults.size(),
            std::size_t{1});

  auto fr = getFirstMemberFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
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
  ASSERT_EQ(res.friendClassCount, 1);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{1});
  const Result::ClassResultsForOneFriendDecl &firstFriendDeclClassResults =
      (res.ClassResults.begin())->second;
  ASSERT_EQ(firstFriendDeclClassResults.size(), std::size_t{1});
  ASSERT_EQ(firstFriendDeclClassResults.front().memberFuncResults.size(),
            std::size_t{2});

  auto fr = getFirstMemberFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);

  fr = firstFriendDeclClassResults.front().memberFuncResults.at(1).funcResult;
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
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
  ASSERT_EQ(res.friendClassCount, 1);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{1});

  const Result::ClassResultsForOneFriendDecl &firstFriendDeclClassResults =
      (res.ClassResults.begin())->second;
  ASSERT_EQ(firstFriendDeclClassResults.size(), std::size_t{2});
  ASSERT_EQ(firstFriendDeclClassResults.at(1).memberFuncResults.size(),
            std::size_t{1});

  auto fr =
      firstFriendDeclClassResults.at(1).memberFuncResults.at(0).funcResult;
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
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
  ASSERT_EQ(res.friendClassCount, 1);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{1});

  const Result::ClassResultsForOneFriendDecl &firstFriendDeclClassResults =
      (res.ClassResults.begin())->second;
  ASSERT_EQ(firstFriendDeclClassResults.size(), std::size_t{2});
  ASSERT_EQ(firstFriendDeclClassResults.at(1).memberFuncResults.size(),
            std::size_t{1});

  auto fr =
      firstFriendDeclClassResults.at(1).memberFuncResults.at(0).funcResult;
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
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
  ASSERT_EQ(res.friendClassCount, 1);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{1});

  const Result::ClassResultsForOneFriendDecl &firstFriendDeclClassResults =
      (res.ClassResults.begin())->second;
  ASSERT_EQ(firstFriendDeclClassResults.size(), std::size_t{2});
  EXPECT_EQ(firstFriendDeclClassResults.at(0).memberFuncResults.size(),
            std::size_t{0});
  EXPECT_EQ(firstFriendDeclClassResults.at(1).memberFuncResults.size(),
            std::size_t{1});

  auto fr =
      firstFriendDeclClassResults.at(1).memberFuncResults.at(0).funcResult;
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
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
  ASSERT_EQ(res.friendClassCount, 1);
  ASSERT_EQ(res.ClassResults.size(), std::size_t{1});

  const Result::ClassResultsForOneFriendDecl &firstFriendDeclClassResults =
      (res.ClassResults.begin())->second;
  ASSERT_EQ(firstFriendDeclClassResults.size(), std::size_t{3});
  EXPECT_EQ(firstFriendDeclClassResults.at(0).memberFuncResults.size(),
            std::size_t{0});
  EXPECT_EQ(firstFriendDeclClassResults.at(1).memberFuncResults.size(),
            std::size_t{0});
  EXPECT_EQ(firstFriendDeclClassResults.at(2).memberFuncResults.size(),
            std::size_t{1});

  auto fr =
      firstFriendDeclClassResults.at(2).memberFuncResults.at(0).funcResult;
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}


