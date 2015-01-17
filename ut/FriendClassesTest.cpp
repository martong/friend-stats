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
  ASSERT_EQ(res.ClassResults.begin()->second.memberFuncResults.size(),
            std::size_t{1});
  auto fr = getFirstMemberFuncResult(res);
  EXPECT_EQ(fr.usedPrivateVarsCount, 2);
  EXPECT_EQ(fr.parentPrivateVarsCount, 3);
}

