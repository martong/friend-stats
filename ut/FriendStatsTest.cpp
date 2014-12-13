#include <gtest/gtest.h>

// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/Tooling.h"

#include "../FriendStats.hpp"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;

struct FriendStats : ::testing::Test {
  SmallString<128> CurrentDir;
  SmallString<128> FileA;
  std::unique_ptr<tooling::FixedCompilationDatabase> Compilations;
  std::unique_ptr<tooling::ClangTool> Tool;
  std::vector<std::string> Sources;
  FriendPrinter Printer;
  MatchFinder Finder;
  FriendStats() {
    // The directory used is not important since the path gets mapped to a
    // virtual
    // file anyway. What is important is that we have an absolute path with
    // which
    // to use with mapVirtualFile().
    std::error_code EC = llvm::sys::fs::current_path(CurrentDir);
    assert(!EC);
    (void)EC;

    FileA = CurrentDir;
    llvm::sys::path::append(FileA, "a.cc");

    Compilations.reset(new tooling::FixedCompilationDatabase(CurrentDir.str(),
                                                             {"-std=c++11"}));
    Sources.push_back(FileA.str());
    Tool.reset(new tooling::ClangTool(*Compilations, Sources));

    Finder.addMatcher(FriendMatcher, &Printer);
  }
};

TEST_F(FriendStats, ClassCount) {
  Tool->mapVirtualFile(FileA, "class A { friend class B; }; class B {};");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Printer.getResult();
  EXPECT_EQ(res.friendClassCount, 1);
}

TEST_F(FriendStats, ClassCount_ExtendedFriend) {
  Tool->mapVirtualFile(FileA, "class Y {}; class A { friend Y; }; ");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Printer.getResult();
  EXPECT_EQ(res.friendClassCount, 1);
}

TEST_F(FriendStats, FuncCount) {
  Tool->mapVirtualFile(FileA,
                       "class A { friend void func(); }; void func(){};");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Printer.getResult();
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
  auto res = Printer.getResult();
  EXPECT_EQ(res.friendFuncCount, 1);
  EXPECT_EQ(res.FuncResults.size(), std::size_t{1});
  auto p = *res.FuncResults.begin();
  EXPECT_EQ(p.second.usedPrivateVarsCount, 1);
}

struct FriendStatsHeader : ::testing::Test {
  SmallString<128> CurrentDir;
  SmallString<128> HeaderA;
  SmallString<128> FileA;
  SmallString<128> FileB;
  std::unique_ptr<tooling::FixedCompilationDatabase> Compilations;
  std::unique_ptr<tooling::ClangTool> Tool;
  std::vector<std::string> Sources;
  FriendPrinter Printer;
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

    Finder.addMatcher(FriendMatcher, &Printer);
  }
};

TEST_F(FriendStatsHeader, NoDuplicateCountOnClasses) {
  Tool->mapVirtualFile(HeaderA, "class A { friend class B; }; class B {};");
  Tool->mapVirtualFile(FileA, R"phi(#include "a.h")phi");
  Tool->mapVirtualFile(FileB, R"phi(#include "a.h")phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Printer.getResult();
  EXPECT_EQ(res.friendClassCount, 1);
}

TEST_F(FriendStatsHeader, NoDuplicateCountOnFunctions) {
  Tool->mapVirtualFile(HeaderA,
                       "class A { friend void func(); }; void func(){};");
  Tool->mapVirtualFile(FileA, R"phi(#include "a.h")phi");
  Tool->mapVirtualFile(FileB, R"phi(#include "a.h")phi");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Printer.getResult();
  EXPECT_EQ(res.friendFuncCount, 1);
}

