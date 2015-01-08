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

    struct Types {
      // The number of used types in this (friend) function
      int usedPrivateCount = 0;
      // The number of priv/protected types
      // in this (friend) function's referred class.
      int parentPrivateCount = 0;
    } types;
  };
  std::map<FullSourceLoc, FuncResult> FuncResults;
};

template <typename T> bool privOrProt(const T *x) {
  return x->getAccess() == AS_private || x->getAccess() == AS_protected;
}

int numberOfPrivOrProtFields(const RecordDecl *RD) {
  int res = 0;
  for (const auto &x : RD->fields()) {
    if (privOrProt(x)) {
      ++res;
    }
  }
  return res;
}

int numberOfPrivOrProtMethods(const CXXRecordDecl *RD) {
  int res = 0;
  for (const auto &x : RD->methods()) {
    if (privOrProt(x)) {
      ++res;
    }
  }
  return res;
}

class PrivTypeCounter : public RecursiveASTVisitor<PrivTypeCounter> {
  int result = 0;

public:
  int getResult() const { return result; }
  bool VisitRecordDecl(const RecordDecl *RD) {
    llvm::outs() << "PrivTypeCounter RD"
                 << "\n";
    if (privOrProt(RD)) {
      ++result;
    }
    return true;
  }
  bool VisitTypedefNameDecl(const TypedefNameDecl *TD) {
    llvm::outs() << "PrivTypeCounter TND"
                 << "\n";
    if (privOrProt(TD)) {
      ++result;
    }
    return true;
  }
};

class TypeHandlerVisitor : public RecursiveASTVisitor<TypeHandlerVisitor> {
  const CXXRecordDecl *Class;
  Result::FuncResult::Types typesResult;
  std::set<const Type *> countedTypes;

  // TODO Verify and make it nicer
  TypedefNameDecl *GetTypeAliasDecl(QualType QT) {
    const Type *T = QT.getTypePtr();
    if (const TypedefType *TT = dyn_cast<TypedefType>(T)) {
      return TT->getDecl();
    }
    while (true) {
      QualType SingleStepDesugar =
          T->getLocallyUnqualifiedSingleStepDesugaredType();
      const Type *T2 = SingleStepDesugar.getTypePtr();
      if (SingleStepDesugar == QualType(T, 0))
        break;
      // T2->dump();
      if (const TypedefType *TT = dyn_cast<TypedefType>(T2)) {
        return TT->getDecl();
      }
      T = T2;
    }
    return nullptr;
  }

  void HandleType(QualType QT) {
    TypedefNameDecl *TND = GetTypeAliasDecl(QT);
    if (!TND) {
      return;
    }

    llvm::outs() << "Decl: " << TND << "\n";
    llvm::outs() << "DeclContext: " << TND->getDeclContext() << "\n";
    CXXRecordDecl *DeclContextRD =
        dyn_cast<CXXRecordDecl>(TND->getDeclContext());
    llvm::outs() << "DeclContext RD: " << DeclContextRD << "\n";
    llvm::outs() << "Class: " << Class << "\n";

    // The actual CXXRecordDecl where our friend function is declared is a
    // class template specialization.
    if (const ClassTemplateSpecializationDecl *CTSD =
            dyn_cast<ClassTemplateSpecializationDecl>(Class)) {

      auto Union = CTSD->getSpecializedTemplateOrPartial();

      // Handle the primary class template
      if (ClassTemplateDecl *CTD = Union.dyn_cast<ClassTemplateDecl *>()) {
        // The type originally defined in the (CXXRecordDecl of the)
        // class template.
        if (DeclContextRD == CTD->getTemplatedDecl()) {
          countedTypes.insert(QT.getTypePtr());
        }
      // Handle the partial specialization
      } else if (ClassTemplatePartialSpecializationDecl *CTPSD = Union.dyn_cast<
                     ClassTemplatePartialSpecializationDecl *>()) {
        if (DeclContextRD == CTPSD) {
          countedTypes.insert(QT.getTypePtr());
        }
      }

    // Regular class (not template instantiation)
    } else if (DeclContextRD == Class) {
      countedTypes.insert(QT.getTypePtr());
    }
  }

public:
  TypeHandlerVisitor(const CXXRecordDecl *Class) : Class(Class) {}

  std::size_t getResult() const { return countedTypes.size(); }

  bool VisitValueDecl(ValueDecl *D) {
    llvm::outs() << "VisitValueDecl: "
                 << "\n";
    QualType QT = D->getType();
    QT->dump();

    const Type *T = QT.getTypePtr();
    if (const FunctionProtoType *FP = T->getAs<FunctionProtoType>()) {
      // if (T->isFunctionProtoType()) {
      // llvm::outs() << "FunctionProtoType: "
      //<< "\n";
      QualType RetType = FP->getReturnType();
      HandleType(RetType);
      // for (const auto &x : FP->getParamTypes()) {
      // llvm::outs() << "Param Type   ";
      // x->dump();
      // HandleType(x);
      //}
      return true;
    }

    HandleType(QT);
    // The return value indicates whether we want the visitation to proceed.
    // Return false to stop the traversal of the AST.
    return true;
  }

  bool VisitTypedefNameDecl(TypedefNameDecl *TD) {

    TD->getUnderlyingType()->dump();

    QualType QT = TD->getUnderlyingType();
    HandleType(QT);

    return true;
  }
};

class MemberHandler : public MatchFinder::MatchCallback {
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

    // This CXXRecordDecl is the child of a ClassTemplateDecl
    // i.e. this is not a template instantiation/specialization.
    // We want to collect statistics only on instantiations/specializations.
    // We are not interested in not used templates.
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

    PrivTypeCounter Visitor;
    Visitor.TraverseCXXRecordDecl(const_cast<CXXRecordDecl *>(RD));

    auto srcLoc = FullSourceLoc{FD->getLocation(), *Result.SourceManager};
    if (FD->getFriendType()) { // friend class
      handleFriendClass(RD, FD, srcLoc, Result);
    } else { // friend function
      handleFriendFunction(RD, FD, srcLoc, Result);
      result.FuncResults.at(srcLoc).types.parentPrivateCount =
          Visitor.getResult();
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
      const FunctionDecl *FuncDefinition =
          nullptr; // This will be set to the decl with the body
      Stmt *Body = FuncD->getBody(FuncDefinition);
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

      TypeHandlerVisitor Visitor{RD};
      // Traverse the function header and body
      // or just the header if body is not existent.
      FuncDefinition
          ? Visitor.TraverseFunctionDecl(
                const_cast<FunctionDecl *>(FuncDefinition))
          : Visitor.TraverseFunctionDecl(FuncD);

      // This is location dependent
      // TODO funcRes.members = ...
      funcRes = memberHandler.getResult();
      funcRes.locationStr = srcLoc.printToString(*Result.SourceManager);
      funcRes.types.usedPrivateCount = Visitor.getResult();
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

