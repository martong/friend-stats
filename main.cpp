// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "FriendStats.hpp"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...");

int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  FriendPrinter Printer;
  MatchFinder Finder;
  Finder.addMatcher(FriendMatcher, &Printer);

  auto ret = Tool.run(newFrontendActionFactory(&Finder).get());
  llvm::outs() << "MyResult.Class: " << Printer.getResult().friendClassCount
               << "\n";
  llvm::outs() << "MyResult.Func: " << Printer.getResult().friendFuncCount
               << "\n";

  double sum = 0.0;
  int num = 0;
  for (const auto &v : Printer.getResult().FuncResults) {
    const auto &funcRes = v.second;
    if (funcRes.parentPrivateVarsCount) {
      ++num;
      sum += funcRes.usedPrivateVarsCount / funcRes.parentPrivateVarsCount;
      llvm::outs() << "loc: " << funcRes.locationStr << "\n";
      llvm::outs() << "usedPrivateVarsCount: " << funcRes.usedPrivateVarsCount
                   << "\n";
      llvm::outs() << "parentPrivateVarsCount: "
                   << funcRes.parentPrivateVarsCount << "\n";
    }
  }
  sum /= num;
  llvm::outs() << "Avarage usage of priv variables: " << sum << "\n";
  return ret;
}

