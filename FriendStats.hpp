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

class MemberHandler : public MatchFinder::MatchCallback {
  // TODO should this be just RecordDecl ?
  const CXXRecordDecl *Class;
  std::set<const FieldDecl *> fields;
  Result::FuncResult funcResult;

public:
  MemberHandler(const CXXRecordDecl *Class) : Class(Class) {}
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

class FriendHandler : public MatchFinder::MatchCallback {
  Result result;

public:
  virtual void run(const MatchFinder::MatchResult &Result) {

    // auto tuPrinter = [&Result]() {
    // Result.Context->getTranslationUnitDecl()->dump();
    // return 0;
    //};
    // const static int x = tuPrinter();
    //(void)x;

    const CXXRecordDecl *RD =
        Result.Nodes.getNodeAs<clang::CXXRecordDecl>("class");
    if (!RD) {
      return;
    }
    // RD->dump();
    // This CXXRecordDecl is the child of a ClassTemplateDecl
    // i.e. this is not a template instantiation/specialization.
    if (RD->getDescribedClassTemplate()) {
      return;
    }
    const FriendDecl *FD = Result.Nodes.getNodeAs<clang::FriendDecl>("friend");
    if (!FD) {
      return;
    }
    // FD->dump();

    auto srcLoc = FD->getLocation();
    if (FD->getFriendType()) { // friend class
      handleFriendClass(RD, FD, srcLoc, Result);
    } else { // friend function
      handleFriendFunction(RD, FD, srcLoc, Result);
    }
    // FD->getLocation().dump(*Result.SourceManager);
    // llvm::outs() << "\n";
    // llvm::outs().flush();
  }
  const Result &getResult() const { return result; }

private:

  void handleFriendClass(const CXXRecordDecl *RD, const FriendDecl *FD,
                         const SourceLocation &srcLoc,
                         const MatchFinder::MatchResult &Result) {
    auto it = result.ClassResults.find(srcLoc);
    if (it != std::end(result.ClassResults)) {
      return;
    }
    // llvm::outs() << "Class/Struct\n";
    ++result.friendClassCount;
    result.ClassResults.insert({srcLoc, 0});
  }

  void handleFriendFunction(const CXXRecordDecl *RD, const FriendDecl *FD,
                            const SourceLocation &srcLoc,
                            const MatchFinder::MatchResult &Result) {
    auto it = result.FuncResults.find(srcLoc);
    if (it != std::end(result.FuncResults)) {
      return;
    }
    // llvm::outs() << "Function\n";
    ++result.friendFuncCount;
    Result::FuncResult funcRes;
    NamedDecl *ND = FD->getFriendDecl();
    if (!ND) {
      return;
    }
    auto handleFuncD = [&](FunctionDecl *FuncD) {
      Stmt *Body = FuncD->getBody();
      if (!Body) {
        return;
      }
      assert(RD);
      auto MemberExprMatcher = findAll(memberExpr().bind("member"));
      MatchFinder Finder;
      MemberHandler memberHandler{RD};
      Finder.addMatcher(MemberExprMatcher, &memberHandler);
      Finder.match(*Body, *Result.Context);
      funcRes = memberHandler.getResult();
      funcRes.locationStr = srcLoc.printToString(*Result.SourceManager);
    };
    if (FunctionDecl *FuncD = dyn_cast<FunctionDecl>(ND)) {
      handleFuncD(FuncD);
    } else if (FunctionTemplateDecl *FTD = dyn_cast<FunctionTemplateDecl>(ND)) {
      // int numOfFuncSpecs = std::distance(FTD->specializations().begin(),
      // FTD->specializations().end());
      // llvm::outs() << "FTD specs: " << numOfFuncSpecs << "\n";
      for (const auto &spec : FTD->specializations()) {
        handleFuncD(spec);
      }
      handleFuncD(FTD->getTemplatedDecl());
    }
    result.FuncResults.insert({srcLoc, funcRes});
  }
};

