#include <sstream> // to print results in percentage
// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "FriendStats.hpp"
#include "DataCrunching.hpp"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("friend-stats-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp
    MoreHelp("\nThis tool collects statistics about friend declarations.");

static cl::opt<bool> UseCompilationDbFiles(
    "db", cl::desc("Run the tool on all files of the compilation db."),
    cl::cat(MyToolCategory));

class ProgressIndicator : public SourceFileCallbacks {
  const std::size_t numFiles = 0;
  std::size_t processedFiles = 0;

public:
  ProgressIndicator(std::size_t numFiles) : numFiles(numFiles) {}
  virtual bool handleBeginSource(CompilerInstance &CI,
                                 StringRef Filename) override {
    ++processedFiles;
    llvm::outs() << Filename << " [" << processedFiles << "/" << numFiles << "]"
                 << "\n";
    llvm::outs().flush();
    return true;
  }
};

inline std::string to_percentage(double d) {
  std::stringstream ss;
  ss << std::fixed << d * 100 << " %";
  return ss.str();
}

class DataTraversal {
public:
  DataTraversal(const Result &result) : result(result) {}
  void operator()() {
    traverseFriendFuncData();
    traverseFriendClassData();
    conclusion();
  }

private:
  const Result &result;
  SelfDiagnostics diags;
  struct Func {
    Average average;
    PercentageDistribution percentageDist;
    NumberOfUsedPrivsDistribution usedPrivsDistribution;
  } func;
  struct Class {
    Average average;
    PercentageDistribution percentageDist;
    NumberOfUsedPrivsDistribution usedPrivsDistribution;
  } clazz;

  void traverseFriendFuncData() {
    for (const auto &v : result.FuncResults) {
      for (const auto vv : v.second) {
        const auto &funcRes = vv.second;
        diags(funcRes);
        func.average(funcRes);
        func.percentageDist(funcRes);
        func.usedPrivsDistribution(funcRes);
      }
    }
  }

  void traverseFriendClassData() {
    for (const auto &friendDecls : result.ClassResults) {
      for (const auto &classSpecs : friendDecls.second) {
        for (const auto &funcResPair : classSpecs.second.memberFuncResults) {
          const auto &funcRes = funcResPair.second;
          diags(funcRes);
          clazz.average(funcRes);
          clazz.percentageDist(funcRes);
          clazz.usedPrivsDistribution(funcRes);
        }
      }
    }
  }

  void conclusion() {
    llvm::outs() << "---- DataTraversal ----"
                 << "\n";

    llvm::outs() << "Number of available friend function definitions: "
                 << func.average.num << "\n";
    llvm::outs() << "Number of friend function declarations with zero priv "
                    "entity declared: " << func.average.numZeroDenom << "\n";
    double sum = func.average.get();
    llvm::outs() << "Average usage of priv entities (vars, funcs, types) in "
                    "friend functions: " << to_percentage(sum) << "\n";

    llvm::outs()
        << "Number of available function definitions in friend classes: "
        << clazz.average.num << "\n";
    llvm::outs()
        << "Number of function declarations of friend classes with zero priv "
           "entity declared: " << clazz.average.numZeroDenom << "\n";
    sum = clazz.average.get();
    llvm::outs() << "Average usage of priv entities (vars, funcs, types) in "
                    "friend classes: " << to_percentage(sum) << "\n";

    llvm::outs()
        << "Friend functions private usage (in percentage) distribution: "
        << "\n";
    llvm::outs() << func.percentageDist.dist;
    llvm::outs() << "Friend functions private usage (by piece) distribution: "
                 << "\n";
    llvm::outs() << func.usedPrivsDistribution.dist;
  }
};

int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);

  auto files = OptionsParser.getSourcePathList();
  if (UseCompilationDbFiles) {
    files = OptionsParser.getCompilations().getAllFiles();
  }

  ClangTool Tool(OptionsParser.getCompilations(), files);

  FriendHandler Handler;
  MatchFinder Finder;
  Finder.addMatcher(FriendMatcher, &Handler);

  ProgressIndicator progressIndicator{files.size()};
  auto ret =
      Tool.run(newFrontendActionFactory(&Finder, &progressIndicator).get());
  llvm::outs() << "Number of processed friend function declarations: "
               << Handler.getResult().friendFuncDeclCount << "\n";
  llvm::outs() << "Number of processed friend class declarations: "
               << Handler.getResult().friendClassDeclCount << "\n";

  DataTraversal traversal{Handler.getResult()};
  traversal();

  return ret;
}

