#include <gtest/gtest.h>

// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/Tooling.h"

#include "../FriendStats.hpp"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;

TEST(FriendStats, ClassCount) {
  // The directory used is not important since the path gets mapped to a virtual
  // file anyway. What is important is that we have an absolute path with which
  // to use with mapVirtualFile().
  SmallString<128> CurrentDir;
  std::error_code EC = llvm::sys::fs::current_path(CurrentDir);
  assert(!EC);
  (void)EC;

  SmallString<128> FileA = CurrentDir;
  llvm::sys::path::append(FileA, "a.cc");

  tooling::FixedCompilationDatabase Compilations(CurrentDir.str(),
                                                 std::vector<std::string>());
  std::vector<std::string> Sources;
  Sources.push_back(FileA.str());
  tooling::ClangTool Tool(Compilations, Sources);

  Tool.mapVirtualFile(FileA, "class A { friend class B; }; class B {};");

  FriendPrinter Printer;
  MatchFinder Finder;
  Finder.addMatcher(FriendMatcher, &Printer);

  Tool.run(newFrontendActionFactory(&Finder).get());
  llvm::outs() << "MyResult.Class: " << Printer.getResult().friendClassCount
               << "\n";
  llvm::outs() << "MyResult.Func: " << Printer.getResult().friendFuncCount
               << "\n";
  auto res = Printer.getResult();
  EXPECT_EQ(res.friendClassCount, 1);
  EXPECT_EQ(res.friendFuncCount, 0);
}

