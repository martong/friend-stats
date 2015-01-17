#pragma once

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/TypeVisitor.h"
#include <map>
#include <set>

// TODO this should be a command line parameter or something similar, but
// definitely should not be in vcs.
const bool debug = false;

inline llvm::raw_ostream &debug_stream() {
  return debug ? llvm::outs() : llvm::nulls();
}

using namespace clang;
using namespace clang::ast_matchers;

// Holds the number of private or protected variables, methods, types
// in a class.
struct ClassCounts {
  int privateVarsCount = 0;
  int privateMethodsCount = 0;
  int privateTypesCount = 0;
};

struct Result {
  int friendClassCount = 0;
  int friendFuncCount = 0;
  std::map<FullSourceLoc, int> ClassResults;
  struct FuncResult {

    std::string locationStr;
    SourceLocation friendDeclLoc; // The location of the friend declaration
    SourceLocation defLoc; // The location of the definition of the friend

    // Below the static variables and static methods are counted in as well.
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

    struct Types {
      // The number of used types in this (friend) function
      int usedPrivateCount = 0;
      // The number of priv/protected types
      // in this (friend) function's referred class.
      int parentPrivateCount = 0;
    } types;
  };

  // Each friend funciton declaration might have it's connected function
  // definition.
  // We collect statistics only on friend functions and friend function
  // templates with body (i.e if they have the definition provided).
  // Each friend function template could have different specializations with
  // their own definition.
  std::map<FullSourceLoc, std::map<const FunctionDecl *, FuncResult>>
      FuncResults;
};

template <typename T> bool privOrProt(const T *x) {
  return x->getAccess() == AS_private || x->getAccess() == AS_protected;
}

inline int numberOfPrivOrProtFields(const RecordDecl *RD) {
  int res = 0;
  for (const auto &x : RD->fields()) {
    if (privOrProt(x)) {
      ++res;
    }
  }

  // To get the static variables in the class we go over all the VarDecls.
  // Unfortunately this is not exposed in RecordDecl as it is done with
  // fields().
  using var_decl_it = DeclContext::specific_decl_iterator<VarDecl>;
  using var_decl_range = llvm::iterator_range<var_decl_it>;
  auto var_decl_begin = var_decl_it{RD->decls_begin()};
  auto var_decl_end = var_decl_it{RD->decls_end()};
  auto var_decls = var_decl_range{var_decl_begin, var_decl_end};
  for (const VarDecl *VD : var_decls) {
    (void)VD;
    ++res;
  }

  return res;
}

inline int numberOfPrivOrProtMethods(const CXXRecordDecl *RD) {
  int res = 0;

  for (const auto &x : RD->methods()) {
    if (privOrProt(x)) {
      ++res;
    }
  }

  // Go over all the member function templates (methods templates).
  // Unfortunately this is not exposed in CXXRecordDecl as it is done with
  // non-template methods.
  using func_templ_it =
      DeclContext::specific_decl_iterator<FunctionTemplateDecl>;
  using func_templ_range = llvm::iterator_range<func_templ_it>;
  auto func_templ_begin = func_templ_it{RD->decls_begin()};
  auto func_templ_end = func_templ_it{RD->decls_end()};
  auto func_templates = func_templ_range{func_templ_begin, func_templ_end};
  for (const FunctionTemplateDecl *FTD : func_templates) {
    for (const auto *Spec : FTD->specializations()) {
      (void)Spec;
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
    if (privOrProt(RD)) {
      ++result;
    }
    return true;
  }
  bool VisitTypedefNameDecl(const TypedefNameDecl *TD) {
    if (privOrProt(TD)) {
      ++result;
    }
    return true;
  }
};

// It handles static methods and operator call expressions as well.
class CallExprVisitor : public RecursiveASTVisitor<CallExprVisitor> {
  const CXXRecordDecl *Class;
  std::set<Decl *> countedDecls;

public:
  std::size_t getResult() const { return countedDecls.size(); }
  CallExprVisitor(const CXXRecordDecl *Class) : Class(Class) {}
  bool VisitDeclRefExpr(DeclRefExpr *DRef) {
    if (CXXMethodDecl *MD = dyn_cast<CXXMethodDecl>(DRef->getDecl())) {
      if (Class == MD->getDeclContext() && privOrProt(MD)) {
        countedDecls.insert(MD);
      }
    }
    return true;
  }
};

class StaticVarsVisitor : public RecursiveASTVisitor<StaticVarsVisitor> {
  const CXXRecordDecl *Class;
  std::set<Decl *> countedDecls;

public:
  std::size_t getResult() const { return countedDecls.size(); }
  StaticVarsVisitor(const CXXRecordDecl *Class) : Class(Class) {}
  bool VisitDeclRefExpr(DeclRefExpr *DRef) {
    if (VarDecl *D = dyn_cast<VarDecl>(DRef->getDecl())) {
      if (Class == D->getDeclContext() && privOrProt(D)) {
        countedDecls.insert(D);
      }
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

    CXXRecordDecl *DeclContextRD =
        dyn_cast<CXXRecordDecl>(TND->getDeclContext());

    auto insert = [&](QualType QT) {
      if (privOrProt(TND))
        countedTypes.insert(QT.getTypePtr());
    };

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
          insert(QT);
        }
        // Handle the partial specialization
      } else if (ClassTemplatePartialSpecializationDecl *CTPSD = Union.dyn_cast<
                     ClassTemplatePartialSpecializationDecl *>()) {
        if (DeclContextRD == CTPSD) {
          insert(QT);
        }
      }

      // Regular class (not template instantiation)
    } else if (DeclContextRD == Class) {
      insert(QT);
    }
  }

public:
  TypeHandlerVisitor(const CXXRecordDecl *Class) : Class(Class) {}

  std::size_t getResult() const { return countedTypes.size(); }

  bool VisitValueDecl(ValueDecl *D) {
    QualType QT = D->getType();
    if (debug)
      QT->dump();

    const Type *T = QT.getTypePtr();
    if (const FunctionProtoType *FP = T->getAs<FunctionProtoType>()) {
      QualType RetType = FP->getReturnType();
      HandleType(RetType);
      return true;
    }

    HandleType(QT);
    // The return value indicates whether we want the visitation to proceed.
    // Return false to stop the traversal of the AST.
    return true;
  }

  bool VisitTypedefNameDecl(TypedefNameDecl *TD) {

    if (debug)
      TD->getUnderlyingType()->dump();

    QualType QT = TD->getUnderlyingType();
    HandleType(QT);

    return true;
  }
};

class MemberHandlerVisitor : public RecursiveASTVisitor<MemberHandlerVisitor> {
  const CXXRecordDecl *Class;
  std::set<const FieldDecl *> fields;
  std::set<const CXXMethodDecl *> methods;

public:
  MemberHandlerVisitor(const CXXRecordDecl *Class) : Class(Class) {}

  bool VisitMemberExpr(MemberExpr *ME) {
    if (const FieldDecl *FD =
            dyn_cast_or_null<const FieldDecl>(ME->getMemberDecl())) {
      const RecordDecl *Parent = FD->getParent();
      if (Parent == Class && privOrProt(FD)) {
        fields.insert(FD);
      }
    } else if (const CXXMethodDecl *MD =
                   dyn_cast_or_null<const CXXMethodDecl>(ME->getMemberDecl())) {
      const CXXRecordDecl *Parent = MD->getParent();
      if (Parent == Class && privOrProt(MD)) {
        methods.insert(MD);
      }
    }
    return true;
  }

  const Result::FuncResult getResult() const {
    Result::FuncResult funcResult;
    funcResult.usedPrivateVarsCount = fields.size();
    funcResult.usedPrivateMethodsCount = methods.size();
    return funcResult;
  }
};

inline ClassCounts getClassCounts(const CXXRecordDecl *RD) {
  ClassCounts classCounts;

  // TODO This could be done with decls_begin, since CXXRecordDecl is a
  // DeclContext. That might be more efficient, since that way we would
  // not traverse the full tree of RD.
  PrivTypeCounter privTypeCounter;
  privTypeCounter.TraverseCXXRecordDecl(const_cast<CXXRecordDecl *>(RD));
  classCounts.privateTypesCount = privTypeCounter.getResult();

  classCounts.privateVarsCount = numberOfPrivOrProtFields(RD);
  classCounts.privateMethodsCount = numberOfPrivOrProtMethods(RD);

  return classCounts;
}

auto const FriendMatcher =
    friendDecl(hasParent(recordDecl().bind("class"))).bind("friend");

class FriendHandler : public MatchFinder::MatchCallback {
  Result result;

public:
  virtual void run(const MatchFinder::MatchResult &Result) {

    auto tuPrinter = [&Result]() {
      if (debug)
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

    // We have to investigate all the parent CXXRecordDecls up in the tree
    // and ensure that they are not template declarations.
    // Again, we want to collect stats only on instantiation/specializations.
    const DeclContext *iRD = dyn_cast<DeclContext>(RD);
    while (iRD->getParent()) {
      if (auto *CRD = dyn_cast<CXXRecordDecl>(iRD->getParent())) {
        if (CRD->getDescribedClassTemplate()) {
          return;
        }
      }
      iRD = iRD->getParent();
    }
    debug_stream() << "CXXRecordDecl with friend: " << RD << "\n";

    const FriendDecl *FD = Result.Nodes.getNodeAs<clang::FriendDecl>("friend");
    if (!FD) {
      return;
    }

    ClassCounts classCounts = getClassCounts(RD);

    auto srcLoc = FullSourceLoc{FD->getLocation(), *Result.SourceManager};

    if (FD->getFriendType()) { // friend class
      handleFriendClass(RD, FD, srcLoc, Result);
    } else { // friend function
      handleFriendFunction(RD, FD, srcLoc, classCounts, Result);
    }
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
    ++result.friendClassCount;
    result.ClassResults.insert({srcLoc, 0});
  }

  void handleFriendFunction(const CXXRecordDecl *RD, const FriendDecl *FD,
                            const FullSourceLoc &srcLoc,
                            const ClassCounts &classCounts,
                            const MatchFinder::MatchResult &Result) {
    auto it = result.FuncResults.find(srcLoc);
    if (it != std::end(result.FuncResults)) {
      return;
    }
    ++result.friendFuncCount;
    Result::FuncResult funcRes;
    NamedDecl *ND = FD->getFriendDecl();
    if (!ND) {
      return;
    }

    // Here FuncD is the declaration of the function, which may or may not
    // has a body.
    auto handleFuncD = [&](FunctionDecl *FuncD) {
      // Get the Body and the function declaration which contains it,
      // i.e. that \cDecl is the function definition itself.
      const FunctionDecl *FuncDefinition =
          nullptr; // This will be set to the decl with the body
      Stmt *Body = FuncD->getBody(FuncDefinition);
      if (!Body) {
        return;
      }
      assert(RD);

      // We are counting stats only for functions which have definitions
      // provided.
      if (!FuncDefinition) {
        return;
      }

      // TODO implementd these visitors in one visitor,
      // so we would traverse the tree only once!
      MemberHandlerVisitor memberHandlerVisitor{RD};
      memberHandlerVisitor.TraverseFunctionDecl(
          const_cast<FunctionDecl *>(FuncDefinition));
      StaticVarsVisitor staticVarsVisitor{RD};
      staticVarsVisitor.TraverseFunctionDecl(
          const_cast<FunctionDecl *>(FuncDefinition));
      CallExprVisitor callExprVisitor{RD};
      callExprVisitor.TraverseFunctionDecl(
          const_cast<FunctionDecl *>(FuncDefinition));
      TypeHandlerVisitor Visitor{RD};
      Visitor.TraverseFunctionDecl(const_cast<FunctionDecl *>(FuncDefinition));

      // This is location dependent
      // TODO funcRes.members = ...
      funcRes = memberHandlerVisitor.getResult();
      funcRes.usedPrivateVarsCount += staticVarsVisitor.getResult();
      funcRes.usedPrivateMethodsCount += callExprVisitor.getResult();
      funcRes.types.usedPrivateCount = Visitor.getResult();

      funcRes.locationStr = srcLoc.printToString(*Result.SourceManager);
      funcRes.friendDeclLoc = srcLoc;
      funcRes.defLoc = FuncDefinition->getLocation();

      funcRes.parentPrivateVarsCount = classCounts.privateVarsCount;
      funcRes.parentPrivateMethodsCount = classCounts.privateMethodsCount;
      funcRes.types.parentPrivateCount = classCounts.privateTypesCount;

      auto &funcResultsPerSrcLoc = result.FuncResults[srcLoc];
      funcResultsPerSrcLoc.insert({FuncDefinition, funcRes});
    };

    if (FunctionDecl *FuncD = dyn_cast<FunctionDecl>(ND)) {
      handleFuncD(FuncD);
    } else if (FunctionTemplateDecl *FTD = dyn_cast<FunctionTemplateDecl>(ND)) {
      for (FunctionDecl *spec : FTD->specializations()) {

        // Note,
        // Internally clang uses the same class to represent a function template
        // explicit specialization and a function template instantiation.
        // An explicit specialization is generated, no matter what, so we
        // include that in the statistics.
        // An instantiation is generated only if it is requested explicitly
        // (explicit instantiation),
        // or only if the given expression requires it (implicit instantiation).
        // If once an explicit specialization is provided and later an
        // instantiation is
        // requested, then both are represented by the same
        // FunctionTemplateSpecializationInfo*.

        // Because of the above this code is not needed:
        // auto tsk_kind = spec->getTemplateSpecializationKind();
        // if (tsk_kind == TSK_ImplicitInstantiation ||
        // tsk_kind == TSK_ExplicitInstantiationDefinition ||
        // tsk_kind == TSK_ExplicitInstantiationDeclaration) {
        // handleFuncD(spec);
        //}

        handleFuncD(spec);
      }
    }
    // We want to handle only the instantiatiions! Therefore we do not
    // investigate the primary template.
    // handleFuncD(FTD->getTemplatedDecl());
  }
};

