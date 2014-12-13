#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

// TODO remove
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

