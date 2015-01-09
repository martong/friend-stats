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

  FriendHandler Handler;
  MatchFinder Finder;
  Finder.addMatcher(FriendMatcher, &Handler);

  auto ret = Tool.run(newFrontendActionFactory(&Finder).get());
  llvm::outs() << "MyResult.Class: " << Handler.getResult().friendClassCount
               << "\n";
  llvm::outs() << "MyResult.Func: " << Handler.getResult().friendFuncCount
               << "\n";

  double sum = 0.0;
  int num = 0;
  for (const auto &v : Handler.getResult().FuncResults) {
    const auto &funcRes = v.second;
    int numerator = funcRes.usedPrivateVarsCount +
                    funcRes.usedPrivateMethodsCount +
                    funcRes.types.usedPrivateCount;
    int denominator = funcRes.parentPrivateVarsCount +
                      funcRes.parentPrivateMethodsCount +
                      funcRes.types.parentPrivateCount;
    if (denominator) {
      ++num;
      sum += numerator / denominator;
      llvm::outs() << "loc: " << funcRes.locationStr << "\n";
      llvm::outs() << "usedPrivateVarsCount: " << funcRes.usedPrivateVarsCount
                   << "\n";
      llvm::outs() << "parentPrivateVarsCount: "
                   << funcRes.parentPrivateVarsCount << "\n";
      llvm::outs() << "usedPrivateMethodsCount: "
                   << funcRes.usedPrivateMethodsCount << "\n";
      llvm::outs() << "parentPrivateMethodsCount: "
                   << funcRes.parentPrivateMethodsCount << "\n";
      llvm::outs() << "types.usedPrivateCount: "
                   << funcRes.types.usedPrivateCount << "\n";
      llvm::outs() << "types.parentPrivateCount: "
                   << funcRes.types.parentPrivateCount << "\n";
    }
  }
  llvm::outs() << "Number of uninterpreted friend function declarations: "
               << Handler.getResult().friendFuncCount - num << "\n";
  sum /= num;
  llvm::outs() << "Avarage usage of priv variables/methods: " << sum << "\n";
  return ret;
}

