// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;

struct MyResult {
  int friendClassCount = 0;
  int friendFuncCount = 0;
};

auto FriendMatcher = friendDecl().bind("friend");
class FriendPrinter : public MatchFinder::MatchCallback {
  MyResult result;

public:
  virtual void run(const MatchFinder::MatchResult &Result) {
    if (const FriendDecl *FD =
            Result.Nodes.getNodeAs<clang::FriendDecl>("friend")) {
      // FD->dump();
      if (FD->getFriendType()) {
        llvm::outs() << "Class/Struct\n";
        ++result.friendClassCount;
      } else {
        llvm::outs() << "Function\n";
        ++result.friendFuncCount;
      }
      FD->getLocation().dump(*Result.SourceManager);
      llvm::outs() << "\n";
      llvm::outs().flush();
    }
  }
  const MyResult &getResult() const { return result; }
};

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
  return ret;
}

// int main() {
// std::string error;
// std::vector<std::string> sources{"test1.cpp"};
// auto compDb = CompilationDatabase::autoDetectFromDirectory("./test", error);
// if (!error.empty()) {
// llvm::outs() << "ERROR: ";
// llvm::outs() << error << "\n";
//}
// ClangTool Tool(*compDb, sources);

// FriendPrinter Printer;
// MatchFinder Finder;
// Finder.addMatcher(FriendMatcher, &Printer);

// auto ret = Tool.run(newFrontendActionFactory(&Finder).get());
// llvm::outs() << "MyResult.Class: " << Printer.getResult().friendClassCount
//<< "\n";
// llvm::outs() << "MyResult.Func: " << Printer.getResult().friendFuncCount
//<< "\n";
// return ret;
//}
