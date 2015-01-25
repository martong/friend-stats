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
  llvm::outs() << "ClassDecls count: " << Handler.getResult().friendClassDeclCount
               << "\n";
  llvm::outs() << "FuncDecls count: "
               << Handler.getResult().friendFuncDeclCount << "\n";

  double sum = 0.0;
  int num = 0;
  int numZeroDenom = 0;
  for (const auto &v : Handler.getResult().FuncResults) {
    // llvm::outs() << v.first.printToString(v.first.getManager()) << "\n";
    for (const auto vv : v.second) {
      const auto &funcRes = vv.second;
      int numerator = funcRes.usedPrivateVarsCount +
                      funcRes.usedPrivateMethodsCount +
                      funcRes.types.usedPrivateCount;
      int denominator = funcRes.parentPrivateVarsCount +
                        funcRes.parentPrivateMethodsCount +
                        funcRes.types.parentPrivateCount;
      ++num;
      // If denominator is zero that means there are no priv or protected
      // entitties
      // in the class, only publicly availble entities are there.
      // If a friend function accesses only public entites that means, it should
      // not be friend at all, therefore we add nothing (zero) to sum in such
      // cases.
      if (denominator) {
        sum += numerator / denominator;
      } else {
        llvm::outs() << "ZERO PRIV"
                     << "\n";
        ++numZeroDenom;
      }
      llvm::outs() << "friendDeclLoc: " << funcRes.friendDeclLocStr << "\n";
      llvm::outs() << "defLoc: " << funcRes.defLocStr << "\n";
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
  llvm::outs() << "Number of friend function declarations with zero priv "
                  "entity declared: " << numZeroDenom << "\n";

  sum /= num;

  llvm::outs() << "Avarage usage of priv entities (vars, funcs, types): " << sum
               << "\n";

  // Self Diagnostics:
  for (const auto &v : Handler.getResult().FuncResults) {
    for (const auto vv : v.second) {
      const auto &funcRes = vv.second;
      if (funcRes.usedPrivateVarsCount > funcRes.parentPrivateVarsCount ||
          funcRes.usedPrivateMethodsCount > funcRes.parentPrivateMethodsCount ||
          funcRes.types.usedPrivateCount > funcRes.types.parentPrivateCount) {
        llvm::errs() << "WRONG MEASURE here: \n" << funcRes.friendDeclLocStr
                     << "\n";
      }
    }
  }

  //for (const auto &friendDecls : Handler.getResult().ClassResults) {
    //for (const auto &classSpecs : friendDecls.second) {
      //for (const Result::ClassResult::MemberFuncResult &memFuncRes :
           //classSpecs.memberFuncResults) {
        //const auto &funcRes = memFuncRes.funcResult;
        //llvm::outs() << "friendDeclLoc: " << funcRes.friendDeclLocStr << "\n";
        //llvm::outs() << "defLoc: " << funcRes.defLocStr << "\n";
        //llvm::outs() << "usedPrivateVarsCount: " << funcRes.usedPrivateVarsCount
                     //<< "\n";
        //llvm::outs() << "parentPrivateVarsCount: "
                     //<< funcRes.parentPrivateVarsCount << "\n";
        //llvm::outs() << "usedPrivateMethodsCount: "
                     //<< funcRes.usedPrivateMethodsCount << "\n";
        //llvm::outs() << "parentPrivateMethodsCount: "
                     //<< funcRes.parentPrivateMethodsCount << "\n";
        //llvm::outs() << "types.usedPrivateCount: "
                     //<< funcRes.types.usedPrivateCount << "\n";
        //llvm::outs() << "types.parentPrivateCount: "
                     //<< funcRes.types.parentPrivateCount << "\n";
      //}
    //}
  //}

  return ret;
}

