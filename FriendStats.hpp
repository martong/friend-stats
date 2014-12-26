#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/TypeVisitor.h"
#include <map>
#include <set>

// TODO remove
using namespace clang;
using namespace clang::ast_matchers;

struct Result {
  int friendClassCount = 0;
  int friendFuncCount = 0;
  std::map<FullSourceLoc, int> ClassResults;
  struct FuncResult {
    std::string locationStr;
    // The number of used variables in this (friend) function
    int usedPrivateVarsCount = 0;
    // The number of priv/protected variables
    // in this (friend) function's referred class.
    int parentPrivateVarsCount = 0;
    // The number of used methods in this (friend) function
    int usedPrivateMethodsCount = 0;
    // The number of priv/protected methods
    // in this (friend) function's referred class.
    int parentPrivateMethodsCount = 0;
    // TODO what about static members ?
  };
  std::map<FullSourceLoc, FuncResult> FuncResults;
};

int numberOfPrivOrProtFields(const RecordDecl *RD) {
  int res = 0;
  for (const auto &x : RD->fields()) {
    if (x->getAccess() == AS_private || x->getAccess() == AS_protected) {
      ++res;
    }
  }
  return res;
}

int numberOfPrivOrProtMethods(const CXXRecordDecl *RD) {
  int res = 0;
  for (const auto &x : RD->methods()) {
    if (x->getAccess() == AS_private || x->getAccess() == AS_protected) {
      ++res;
    }
  }
  return res;
}

class TypeHandler : public MatchFinder::MatchCallback {
  const CXXRecordDecl *Class;

public:
  TypeHandler(const CXXRecordDecl *Class) : Class(Class) {}
  virtual void run(const MatchFinder::MatchResult &Result) {
    llvm::outs() << "RUN: "
                 << "\n";
    // if (const QualType *QT = Result.Nodes.getNodeAs<QualType>("type")) {
    // llvm::outs() << "QT: " << QT << "\n";
    //}
    // if (const ValueDecl *VD = Result.Nodes.getNodeAs<ValueDecl>("valueDecl"))
    // {
    // llvm::outs() << "VD: " << VD << "\n";
    //}
    if (const VarDecl *VD = Result.Nodes.getNodeAs<VarDecl>("decl")) {
      llvm::outs() << "VD: " << VD << "\n";
    }
  }
};

class TypeHandlerVisitor : public RecursiveASTVisitor<TypeHandlerVisitor> {
public:
  bool VisitValueDecl(ValueDecl *D) {
    // For debugging, dumping the AST nodes will show which nodes are already
    // being visited.
    // D->dump();
    // D->getType()->dump();

    // The return value indicates whether we want the visitation to proceed.
    // Return false to stop the traversal of the AST.
    return true;
  }
  bool VisitTypedefNameDecl(TypedefNameDecl *TD) {

    //TD->dump();

    TD->getUnderlyingType()->dump();

    //for (const auto x : TD->redecls()) {
      //llvm::outs() << "Redecl: ";
      //x->dump();
      //llvm::outs() << "\n";
    //}

    QualType QT = TD->getUnderlyingType();
    const Type *T = QT.getTypePtr(); 
    llvm::outs() << "Type1: " << T << "\n"; // ElaboratedType
    llvm::outs() << "TypeClassName: " << T->getTypeClassName() << "\n"; // ElaboratedType
    //QualType SingleStepDesugar =
        //T->getLocallyUnqualifiedSingleStepDesugaredType();
    //while (SingleStepDesugar != QualType(T, 0)) {
    while (true) {

      //if (!SingleStepDesugar.getTypePtr()) break;
      QualType SingleStepDesugar = T->getLocallyUnqualifiedSingleStepDesugaredType();
      const Type *T2 = SingleStepDesugar.getTypePtr();
      if (SingleStepDesugar == QualType(T, 0)) break;
      llvm::outs() << "Type2: " << T2 << "\n";
      llvm::outs() << "TypeClassName: " << T2->getTypeClassName() << "\n"; // ElaboratedType
      T2->dump();
      if (const TypedefType* TT = dyn_cast<TypedefType>(T2)) {
        //VisitTypedefNameDecl(TT->getDecl());
        llvm::outs() << "Decl: " << TT->getDecl() << "\n";
      }

      T = T2;
    }

    //QualType QT = TD->getUnderlyingType();
    //const Type *T = QT.getTypePtr(); 
    //T->dump();
    //struct TVisitor : TypeVisitor<TVisitor> {
      //void VisitTypedefType(const TypedefType *T) {
        //llvm::outs() << "TypedefType: ";
        //T->dump();
        //llvm::outs() << "\n";
      //}
      //void VisitElaboratedType(const ElaboratedType *T) {
        //llvm::outs() << "ElaboratedType: " << T;
        //T->dump();
        //llvm::outs() << "\n";
        //llvm::outs() << "ET: namedType: " << T->getNamedType().getTypePtr();
      //}
    //} tvisitor;
    //tvisitor.Visit(T);


    // TD->getUnderlyingType()->elabo
    // ElaboratedType* ET; ET->typedef
    // llvm::outs() << "TD under: " << TD->getUnderlyingType()->alias << "\n";
    return true;
  }
};

class MemberHandler : public MatchFinder::MatchCallback {
  // TODO should this be just RecordDecl ?
  const CXXRecordDecl *Class;
  std::set<const FieldDecl *> fields;
  std::set<const CXXMethodDecl *> methods;
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
        funcResult.parentPrivateVarsCount = numberOfPrivOrProtFields(Parent);
        bool privateOrProtected =
            FD->getAccess() == AS_private || FD->getAccess() == AS_protected;
        if (Parent == Class && privateOrProtected) {
          fields.insert(FD);
          // TODO this could be calculated once in getResult
          funcResult.usedPrivateVarsCount = fields.size();
        }
      } else if (const CXXMethodDecl *MD =
                     dyn_cast_or_null<const CXXMethodDecl>(
                         ME->getMemberDecl())) {
        const CXXRecordDecl *Parent = MD->getParent();
        // TODO count this only once:
        funcResult.parentPrivateMethodsCount =
            numberOfPrivOrProtMethods(Parent);
        bool privateOrProtected =
            MD->getAccess() == AS_private || MD->getAccess() == AS_protected;
        if (Parent == Class && privateOrProtected) {
          methods.insert(MD);
          // TODO this could be calculated once in getResult
          funcResult.usedPrivateMethodsCount = methods.size();
        }
      }
      // ME->dump();
    }
  }
  const Result::FuncResult &getResult() const { return funcResult; }
};

auto FriendMatcher =
    friendDecl(hasParent(recordDecl().bind("class"))).bind("friend");

class FriendHandler : public MatchFinder::MatchCallback {
  Result result;

public:
  virtual void run(const MatchFinder::MatchResult &Result) {

    auto tuPrinter = [&Result]() {
      Result.Context->getTranslationUnitDecl()->dump();
      return 0;
    };
    const static int x = tuPrinter();
    (void)x;

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
    // llvm::outs() << "FD: " << FD << "\n";
    // llvm::outs() << "RD: " << RD << "\n";

    auto srcLoc = FullSourceLoc{FD->getLocation(), *Result.SourceManager};
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
                         const FullSourceLoc &srcLoc,
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
                            const FullSourceLoc &srcLoc,
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

      {
        // ValueDecl* VD;
        // VD->getType();
        // TypedefNameDecl* TD;
        // TD->getUnderlyingType();
        // auto Matcher = findAll(qualType().bind("type"));
        // auto Matcher = findAll(decl().bind("decl"));
        // MatchFinder Finder;
        // TypeHandler typeHandler{RD};
        // Finder.addMatcher(Matcher, &typeHandler);
        // Finder.match(*Body, *Result.Context);
      }

      TypeHandlerVisitor Visitor;
      Visitor.TraverseStmt(Body);

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

