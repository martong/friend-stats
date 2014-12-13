#include <gtest/gtest.h>

// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/Tooling.h"

#include "../FriendStats.hpp"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;

TEST(First, first) {
  std::string error;
  std::vector<std::string> sources{"test1.cpp"};
  auto compDb = CompilationDatabase::autoDetectFromDirectory("./test", error);
  if (!error.empty()) {
    llvm::outs() << "ERROR: ";
    llvm::outs() << error << "\n";
  }
  ClangTool Tool(*compDb, sources);

  FriendPrinter Printer;
  MatchFinder Finder;
  Finder.addMatcher(FriendMatcher, &Printer);

  Tool.run(newFrontendActionFactory(&Finder).get());
  llvm::outs() << "MyResult.Class: " << Printer.getResult().friendClassCount
               << "\n";
  llvm::outs() << "MyResult.Func: " << Printer.getResult().friendFuncCount
               << "\n";
}

