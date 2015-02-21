#include <sstream> // to print results in percentage
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

void print(const Result::FuncResult &funcRes) {
  llvm::outs() << "friendDeclLoc: " << funcRes.friendDeclLocStr << "\n";
  llvm::outs() << "defLoc: " << funcRes.defLocStr << "\n";
  llvm::outs() << "diagName: " << funcRes.diagName << "\n";
  llvm::outs() << "usedPrivateVarsCount: " << funcRes.usedPrivateVarsCount
               << "\n";
  llvm::outs() << "parentPrivateVarsCount: " << funcRes.parentPrivateVarsCount
               << "\n";
  llvm::outs() << "usedPrivateMethodsCount: " << funcRes.usedPrivateMethodsCount
               << "\n";
  llvm::outs() << "parentPrivateMethodsCount: "
               << funcRes.parentPrivateMethodsCount << "\n";
  llvm::outs() << "types.usedPrivateCount: " << funcRes.types.usedPrivateCount
               << "\n";
  llvm::outs() << "types.parentPrivateCount: "
               << funcRes.types.parentPrivateCount << "\n";
}

struct Average {
  double sum = 0.0;
  int num = 0;
  int numZeroDenom = 0;
  void operator()(const Result::FuncResult &funcRes) {
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
      sum += static_cast<double>(numerator) / denominator;
    } else {
      // llvm::outs() << "ZERO PRIV" << "\n";
      //<< "\n";
      ++numZeroDenom;
    }
    // print(funcRes);
  }
  double get() const { return sum / num; }
};

void selfDiagnostic(const Result &result) {
  for (const auto &v : result.FuncResults) {
    for (const auto vv : v.second) {
      const auto &funcRes = vv.second;
      if (funcRes.usedPrivateVarsCount > funcRes.parentPrivateVarsCount ||
          funcRes.usedPrivateMethodsCount > funcRes.parentPrivateMethodsCount ||
          funcRes.types.usedPrivateCount > funcRes.types.parentPrivateCount) {
        llvm::errs() << "WRONG MEASURE here: \n" << funcRes.friendDeclLocStr
                     << "\n";
        print(funcRes);
      }
    }
  }
  for (const auto &friendDecls : result.ClassResults) {
    for (const auto &classSpecs : friendDecls.second) {
      for (const auto &funcResPair : classSpecs.second.memberFuncResults) {
        const auto &funcRes = funcResPair.second;
        if (funcRes.usedPrivateVarsCount > funcRes.parentPrivateVarsCount ||
            funcRes.usedPrivateMethodsCount >
                funcRes.parentPrivateMethodsCount ||
            funcRes.types.usedPrivateCount > funcRes.types.parentPrivateCount) {
          llvm::errs() << "WRONG MEASURE here: \n" << funcRes.friendDeclLocStr
                       << "\n";
          print(funcRes);
        }
      }
    }
  }
}

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
  struct Func {
    Average average;
  } func;
  struct Class {
    Average average;
  } clazz;

  void traverseFriendFuncData() {
    for (const auto &v : result.FuncResults) {
      for (const auto vv : v.second) {
        const auto &funcRes = vv.second;
        func.average(funcRes);
      }
    }
  }

  void traverseFriendClassData() {
    for (const auto &friendDecls : result.ClassResults) {
      for (const auto &classSpecs : friendDecls.second) {
        for (const auto &funcResPair : classSpecs.second.memberFuncResults) {
          const auto &funcRes = funcResPair.second;
          clazz.average(funcRes);
        }
      }
    }
  }

  void conclusion() {
    llvm::outs() << "---- DataTraversal ----"
                 << "\n";

    llvm::outs() << "Number of friend function declarations with zero priv "
                    "entity declared: " << func.average.numZeroDenom << "\n";
    double sum = func.average.get();
    llvm::outs() << "Average usage of priv entities (vars, funcs, types) in "
                    "friend functions: " << to_percentage(sum) << "\n";

    llvm::outs()
        << "Number of function declarations of friend classes with zero priv "
           "entity declared: " << clazz.average.numZeroDenom << "\n";
    sum = clazz.average.get();
    llvm::outs() << "Average usage of priv entities (vars, funcs, types) in "
                    "friend classes: " << to_percentage(sum) << "\n";
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
  llvm::outs() << "ClassDecls count: "
               << Handler.getResult().friendClassDeclCount << "\n";
  llvm::outs() << "FuncDecls count: " << Handler.getResult().friendFuncDeclCount
               << "\n";

  DataTraversal traversal{Handler.getResult()};
  traversal();

  const bool doSelfDiagnostics = false;
  if (doSelfDiagnostics) {
    selfDiagnostic(Handler.getResult());
  }
  return ret;
}

