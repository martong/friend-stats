#include <sstream> // to print results in percentage
// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "FriendStats.hpp"
#include "DataCrunching.hpp"
#include "DataIO.hpp"

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
    MoreHelp("\nThis tool collects statistics about friend declarations");

static cl::opt<bool> UseCompilationDbFiles(
    "db", cl::desc("Run the tool on all files of the compilation db"),
    cl::ValueOptional, cl::cat(MyToolCategory));

static cl::opt<bool> PrintZeroPrivInHost(
    "zh", cl::desc("Print friend function instances whose "
                   "befriending class has no private entities"),
    cl::ValueOptional, cl::cat(MyToolCategory));

static cl::opt<bool> PrintZeroPrivInFriend(
    "zf", cl::desc("Print friend function instances "
                   "who does not access any private entities."),
    cl::ValueOptional, cl::cat(MyToolCategory));

static cl::opt<bool> NoStatistics(
    "no_stats", cl::desc("Do not print out statistics about friend usage."),
    cl::ValueOptional, cl::cat(MyToolCategory));

static cl::opt<bool> PrintPossiblyIncorrectFriend(
    "if", cl::desc("Print friend function instances "
                   "which are possible not correct."),
    cl::ValueOptional, cl::cat(MyToolCategory));

static cl::opt<bool> PrintHostClassesWithZeroPrivate(
    "host_classes_with_zero_priv",
    cl::desc(
        "Print befriending classes which have only public members and types."),
    cl::ValueOptional, cl::cat(MyToolCategory));

static cl::opt<bool> PrintIncorrectFriendClasses(
    "incorrect_friend_classes",
    cl::desc("Print friend classes which don't use any private entities."),
    cl::ValueOptional, cl::cat(MyToolCategory));

static cl::opt<bool> PrintStrongCandidate(
    "sc", cl::desc("Print entries (for friend function decls) whose "
                   "are strong candidates of selective friend"),
    cl::ValueOptional, cl::cat(MyToolCategory));

static cl::opt<bool> PrintStrongCandidateBecauseOfMemberVars(
    "scmv", cl::desc("Print entries (for friend function decls) whose "
                     "are strong candidates of selective friend because of "
                     "their member variable usage"),
    cl::ValueOptional, cl::cat(MyToolCategory));

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
    if (PrintHostClassesWithZeroPrivate)
      printHostClassesWithZeroPrivate();
    if (!NoStatistics)
      conclusion();
  }

private:
  const Result &result;
  SelfDiagnostics diags;
  HostClassesWithZeroPrivate hostClassesWithZeroPriv;
  struct Func {
    Average average;
    PercentageDistribution percentageDist;
    NumberOfUsedPrivsDistribution usedPrivsDistribution;
    StrongCandidate strongCandidate;
    StrongCandidateBecauseMemberVars strongCandidateBecuaseMemberVars;
    ZeroPrivInHost zeroPrivInHost;
    ZeroPrivInFriend zeroPrivInFriend;
    MeyersCandidate meyersCandidate;
    PossiblyIncorrectFriend possiblyIncorrect;
  } func;
  struct Class {
    Average average;
    PercentageDistribution percentageDist;
    NumberOfUsedPrivsDistribution usedPrivsDistribution;
  } clazz;

  void traverseFriendFuncData() {
    for (const auto &v : result.FuncResults) {
      for (const auto funcResPair : v.second) {
        const auto &funcRes = funcResPair.second;
        if (diags(funcRes)) {
          func.average(funcRes);
          func.percentageDist(funcRes);
          func.usedPrivsDistribution(funcRes);

          // Order in the condition is important here, because
          // we want the stats even if we don't want to print out the entries.
          if (func.strongCandidate(funcRes) && PrintStrongCandidate) {
            print(funcResPair);
          }
          if (func.strongCandidateBecuaseMemberVars(funcRes) &&
              PrintStrongCandidateBecauseOfMemberVars) {
            print(funcResPair);
          }

          if (PrintZeroPrivInHost && func.zeroPrivInHost(funcRes)) {
            print(funcResPair);
          }
          if (PrintZeroPrivInFriend && func.zeroPrivInFriend(funcRes)) {
            print(funcResPair);
          }

          func.meyersCandidate(funcResPair);

          if (PrintPossiblyIncorrectFriend && func.possiblyIncorrect(funcResPair)) {
            print(funcResPair);
          }

          hostClassesWithZeroPriv(funcRes);

        } else {
          llvm::outs() << "WRONG MEASURE here:\n" << funcRes.friendDeclLocStr
                       << "\n";
          print(funcResPair);
          llvm::outs() << "SKIPPING ENTRY FROM STATISTICS\n\n";
        }
      }
    }
  }

  void traverseFriendClassData() {
    for (const auto &friendDecls : result.ClassResults) {
      for (const auto &classSpecs : friendDecls.second) {
        IncorrectFriendClass incorrectFriendClass;
        for (const auto &funcResPair : classSpecs.second.memberFuncResults) {
          const auto &funcRes = funcResPair.second;
          if (diags(funcRes)) {
            clazz.average(funcRes);
            clazz.percentageDist(funcRes);
            clazz.usedPrivsDistribution(funcRes);
            hostClassesWithZeroPriv(funcRes);
            incorrectFriendClass(funcRes);
          } else {
            llvm::outs() << "WRONG MEASURE here:\n" << funcRes.friendDeclLocStr
                         << "\n";
            print(funcResPair);
            llvm::outs() << "SKIPPING ENTRY FROM STATISTICS\n\n";
          }
        }
        if (PrintIncorrectFriendClasses && incorrectFriendClass.result) {
          printIncorrectFriendClass(classSpecs.second);
        }
      }
    }
  }

  void printHostClassesWithZeroPrivate() {
    for (const auto& cip : hostClassesWithZeroPriv.classes) {
      print(*cip);
    }
  }

  void printIncorrectFriendClass(const Result::ClassResult &classResult) {
    llvm::outs()
        << "============================================================"
           "================\n";
    llvm::outs() << "diagName: " << classResult.diagName << "\n";
    llvm::outs() << "defLoc: " << classResult.defLocStr << "\n";
    llvm::outs() << "friendDeclLoc: " << classResult.friendDeclLocStr << "\n";
    llvm::outs()
        << "============================================================"
           "================\n";
  }

  void conclusion() {
    llvm::outs() << "########## Friend FUNCTIONS ##########"
                 << "\n";
    llvm::outs() << "Number of available friend function definitions: "
                 << func.average.num << "\n";
    llvm::outs() << "Number of friend function declarations with zero priv "
                    "entity declared in host class: "
                 << func.average.numZeroDenom
                 << "\n";
    double sum = func.average.get();
    llvm::outs() << "Average usage of priv entities (vars, funcs, types) in "
                    "friend functions: "
                 << to_percentage(sum)
                 << "\n";
    llvm::outs()
        << "Friend functions private usage (in percentage) distribution:"
        << "\n";
    llvm::outs() << func.percentageDist.dist;
    llvm::outs() << "Friend functions private usage (by piece) distribution:"
                 << "\n";
    llvm::outs() << func.usedPrivsDistribution.dist;
    llvm::outs() << R"(Number of "friend for" strong candidates: )"
                 << func.strongCandidate.count << "\n";
    llvm::outs() << "From this, strong candidates because of member usage: "
                 << func.strongCandidateBecuaseMemberVars.count << "\n";
    llvm::outs() << "Number of Meyers candidates: "
                 << func.meyersCandidate.count << "\n";

    llvm::outs() << "\n";
    llvm::outs() << "########## Friend CLASSES ##########"
                 << "\n";
    llvm::outs()
        << "Number of available function definitions in friend classes: "
        << clazz.average.num << "\n";
    llvm::outs()
        << "Number of function declarations of friend classes with zero priv "
           "entity declared in host class: "
        << clazz.average.numZeroDenom
        << "\n";
    sum = clazz.average.get();
    llvm::outs() << "Average usage of priv entities (vars, funcs, types) in "
                    "friend classes: "
                 << to_percentage(sum)
                 << "\n";
    llvm::outs() << R"("Indirect friend")"
                    " functions private usage (in percentage) distribution:"
                 << "\n";
    llvm::outs() << clazz.percentageDist.dist;
    llvm::outs() << R"("Indirect friend")"
                    " functions private usage (by piece) distribution:"
                 << "\n";
    llvm::outs() << clazz.usedPrivsDistribution.dist;
  }
};

int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);

  auto files = OptionsParser.getSourcePathList();
  if (UseCompilationDbFiles.getNumOccurrences() > 0) {
    files = OptionsParser.getCompilations().getAllFiles();
  }

  ClangTool Tool(OptionsParser.getCompilations(), files);

  FriendHandler Handler;
  MatchFinder Finder;
  Finder.addMatcher(FriendMatcher, &Handler);

  ProgressIndicator progressIndicator{files.size()};
  auto ret =
      Tool.run(newFrontendActionFactory(&Finder, &progressIndicator).get());
  llvm::outs() << "\n";
  llvm::outs() << "Number of processed friend function declarations: "
               << Handler.getResult().friendFuncDeclCount << "\n";
  llvm::outs() << "Number of processed friend class declarations: "
               << Handler.getResult().friendClassDeclCount << "\n";
  llvm::outs() << "\n";

  DataTraversal traversal{Handler.getResult()};
  traversal();

  return ret;
}

