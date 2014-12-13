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

    Compilations.reset(new tooling::FixedCompilationDatabase(
        CurrentDir.str(), std::vector<std::string>()));
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

TEST_F(FriendStats, FuncCount) {
  Tool->mapVirtualFile(FileA,
                       "class A { friend void func(); }; void func(){};");
  Tool->run(newFrontendActionFactory(&Finder).get());
  auto res = Printer.getResult();
  EXPECT_EQ(res.friendFuncCount, 1);
}
