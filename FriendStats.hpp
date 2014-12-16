#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <map>

// TODO remove
using namespace clang;
using namespace clang::ast_matchers;

struct Result {
  int friendClassCount = 0;
  int friendFuncCount = 0;
  std::map<SourceLocation, int> ClassResults;
  struct FuncResult {
    // The number of used variables in this (friend) function
    int usedPrivateVarsCount = 0;
    // The number of priv/protected variables
    // in this (friend) function's referred class.
    int parentPrivateVarsCount = 0;
  };
  std::map<SourceLocation, FuncResult> FuncResults;
};

int numberOfPrivOrProtMembers(const RecordDecl* RD) {
  int res = 0;
  for (const auto& x : RD->fields()) { ++res; (void)x; }
  return res;
}

class MemberPrinter : public MatchFinder::MatchCallback {
  // TODO should this be just RecordDecl ?
  const CXXRecordDecl *Class;
  Result::FuncResult funcResult;

public:
  MemberPrinter(const CXXRecordDecl *Class) : Class(Class) {}
  virtual void run(const MatchFinder::MatchResult &Result) {
    if (const MemberExpr *ME = Result.Nodes.getNodeAs<MemberExpr>("member")) {
      // llvm::outs() << "ME: \n";
      if (const FieldDecl *FD =
              dyn_cast_or_null<const FieldDecl>(ME->getMemberDecl())) {
        const RecordDecl *Parent = FD->getParent();
        // TODO count this only once:
        funcResult.parentPrivateVarsCount = numberOfPrivOrProtMembers(Parent);
        bool privateOrProtected =
            FD->getAccess() == AS_private || FD->getAccess() == AS_protected;
        if (Parent == Class && privateOrProtected) {
          ++funcResult.usedPrivateVarsCount;
          // llvm::outs() << "MATCH\n";
        }
      }
      // ME->dump();
    }
  }
  const Result::FuncResult &getResult() const { return funcResult; }
};

auto FriendMatcher = recordDecl(has(friendDecl().bind("friend"))).bind("class");
auto xxxMatcher = fieldDecl(isPrivate());

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
                funcRes = Printer.getResult();
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

