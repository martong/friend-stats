#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <map>
#include <set>

// TODO remove
using namespace clang;
using namespace clang::ast_matchers;

struct Result {
  int friendClassCount = 0;
  int friendFuncCount = 0;
  std::map<SourceLocation, int> ClassResults;
  struct FuncResult {
    std::string locationStr;
    // The number of used variables in this (friend) function
    int usedPrivateVarsCount = 0;
    // The number of priv/protected variables
    // in this (friend) function's referred class.
    int parentPrivateVarsCount = 0;
    // TODO what about static members ?
  };
  std::map<SourceLocation, FuncResult> FuncResults;
};

int numberOfPrivOrProtMembers(const RecordDecl *RD) {
  int res = 0;
  for (const auto &x : RD->fields()) {
    if (x->getAccess() == AS_private || x->getAccess() == AS_protected) {
      ++res;
    }
  }
  return res;
}

class MemberPrinter : public MatchFinder::MatchCallback {
  // TODO should this be just RecordDecl ?
  const CXXRecordDecl *Class;
  std::set<const FieldDecl *> fields;
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
          fields.insert(FD);
          // TODO this could be calculated once in getResult
          funcResult.usedPrivateVarsCount = fields.size();
          //++funcResult.usedPrivateVarsCount;
          // llvm::outs() << "MATCH\n";
        }
      }
      // ME->dump();
    }
  }
  const Result::FuncResult &getResult() const { return funcResult; }
};

auto FriendMatcher = recordDecl(has(friendDecl().bind("friend"))).bind("class");

class FriendPrinter : public MatchFinder::MatchCallback {
  Result result;

public:
  virtual void run(const MatchFinder::MatchResult &Result) {
    if (const CXXRecordDecl *RD =
            Result.Nodes.getNodeAs<clang::CXXRecordDecl>("class")) {
      // This CXXRecordDecl is not the child of a ClassTemplateDecl
      // i.e. this is a template instantiation/specialization.
      if (!RD->getDescribedClassTemplate()) {
        if (const FriendDecl *FD =
                Result.Nodes.getNodeAs<clang::FriendDecl>("friend")) {
          // FD->dump();
          auto srcLoc = FD->getLocation();
          if (FD->getFriendType()) { // friend class
            auto it = result.ClassResults.find(srcLoc);
            if (it == std::end(result.ClassResults)) {
              // llvm::outs() << "Class/Struct\n";
              ++result.friendClassCount;
              result.ClassResults.insert({srcLoc, 0});
            }
          } else { // friend function
            auto it = result.FuncResults.find(srcLoc);
            if (it == std::end(result.FuncResults)) {
              // llvm::outs() << "Function\n";
              ++result.friendFuncCount;
              Result::FuncResult funcRes;
              if (NamedDecl *ND = FD->getFriendDecl()) {
                auto handleFuncD = [&](FunctionDecl *FuncD) {
                  if (Stmt *Body = FuncD->getBody()) {
                    // Body->dump();
                    const CXXRecordDecl *RD =
                        Result.Nodes.getNodeAs<clang::CXXRecordDecl>("class");
                    assert(RD);
                    auto MemberExprMatcher =
                        findAll(memberExpr().bind("member"));
                    MatchFinder Finder;
                    MemberPrinter Printer{RD};
                    Finder.addMatcher(MemberExprMatcher, &Printer);
                    Finder.match(*Body, *Result.Context);
                    funcRes = Printer.getResult();
                    funcRes.locationStr =
                        srcLoc.printToString(*Result.SourceManager);
                  }
                };
                if (FunctionDecl *FuncD = dyn_cast<FunctionDecl>(ND)) {
                  handleFuncD(FuncD);
                } else if (FunctionTemplateDecl *FTD =
                               dyn_cast<FunctionTemplateDecl>(ND)) {
                  handleFuncD(FTD->getTemplatedDecl());
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
    }
  }
  const Result &getResult() const { return result; }
};

