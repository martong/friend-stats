#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <map>

// TODO remove
using namespace clang;
using namespace clang::ast_matchers;

class MemberPrinter : public MatchFinder::MatchCallback {
  // TODO should this be just RecordDecl ?
  const CXXRecordDecl *Class;

public:
  MemberPrinter(const CXXRecordDecl *Class) : Class(Class) {}
  virtual void run(const MatchFinder::MatchResult &Result) {
    if (const MemberExpr *ME = Result.Nodes.getNodeAs<MemberExpr>("member")) {
      llvm::outs() << "ME: \n";
      if (const FieldDecl *FD =
              dyn_cast_or_null<const FieldDecl>(ME->getMemberDecl())) {
        const RecordDecl* Parent = FD->getParent();
        if (Parent == Class) {
          llvm::outs() << "MATCH\n";
        }
      }
      ME->dump();
    }
  }
};

struct Result {
  int friendClassCount = 0;
  int friendFuncCount = 0;
  std::map<SourceLocation, int> ClassResults;
  struct FuncResult {
    int usedPrivateVarsCount = 0;
  };
  std::map<SourceLocation, FuncResult> FuncResults;
};

auto FriendMatcher = recordDecl(has(friendDecl().bind("friend"))).bind("class");

class FriendPrinter : public MatchFinder::MatchCallback {
  Result result;

public:
  virtual void run(const MatchFinder::MatchResult &Result) {
    if (const CXXRecordDecl *RD =
            Result.Nodes.getNodeAs<clang::CXXRecordDecl>("class")) {
      RD->dump();
    }

    if (const FriendDecl *FD =
            Result.Nodes.getNodeAs<clang::FriendDecl>("friend")) {
      // FD->dump();
      auto srcLoc = FD->getLocation();
      if (FD->getFriendType()) {
        auto it = result.ClassResults.find(srcLoc);
        if (it == std::end(result.ClassResults)) {
          // llvm::outs() << "Class/Struct\n";
          ++result.friendClassCount;
          result.ClassResults.insert({srcLoc, 0});
        }
      } else {
        auto it = result.FuncResults.find(srcLoc);
        if (it == std::end(result.FuncResults)) {
          // llvm::outs() << "Function\n";
          ++result.friendFuncCount;
          Result::FuncResult funcRes;
          if (NamedDecl *ND = FD->getFriendDecl()) {
            if (FunctionDecl *FuncD = dyn_cast<FunctionDecl>(ND)) {
              if (Stmt *Body = FuncD->getBody()) {
                Body->dump();
                // TODO how to call a new MatchFinder on the Body
                // auto MemberExprMatcher = memberExpr().bind("member");
                // auto MemberExprMatcher = binaryOperator().bind("member");
                // auto MemberExprMatcher = compoundStmt().bind("member");
                // auto MemberExprMatcher = decl(forEachDescendant(stmt()));
                const CXXRecordDecl *RD =
                    Result.Nodes.getNodeAs<clang::CXXRecordDecl>("class");
                assert(RD);
                auto MemberExprMatcher = findAll(memberExpr().bind("member"));
                MatchFinder Finder;
                MemberPrinter Printer{RD};
                Finder.addMatcher(MemberExprMatcher, &Printer);
                Finder.match(*Body, *Result.Context);
                funcRes.usedPrivateVarsCount = 1;
              }
            }
          }
          result.FuncResults.insert({srcLoc, funcRes});
        }
      }
      // FD->getLocation().dump(*Result.SourceManager);
      // llvm::outs() << "\n";
      // llvm::outs().flush();
    }
  }
  const Result &getResult() const { return result; }
};

