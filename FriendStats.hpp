#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <set>

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
  std::set<SourceLocation> ClassSrcLocs;
  std::set<SourceLocation> FuncSrcLocs;

public:
  virtual void run(const MatchFinder::MatchResult &Result) {
    if (const FriendDecl *FD =
            Result.Nodes.getNodeAs<clang::FriendDecl>("friend")) {
      // FD->dump();
      auto srcLoc = FD->getLocation();
      if (FD->getFriendType()) {
        auto it = ClassSrcLocs.find(srcLoc);
        if (it == std::end(ClassSrcLocs)) {
          //llvm::outs() << "Class/Struct\n";
          ++result.friendClassCount;
          ClassSrcLocs.insert(srcLoc);
        }
      } else {
        auto it = FuncSrcLocs.find(srcLoc);
        if (it == std::end(FuncSrcLocs)) {
          //llvm::outs() << "Function\n";
          ++result.friendFuncCount;
          FuncSrcLocs.insert(srcLoc);
        }
      }
      //FD->getLocation().dump(*Result.SourceManager);
      //llvm::outs() << "\n";
      //llvm::outs().flush();
    }
  }
  const MyResult &getResult() const { return result; }
};

