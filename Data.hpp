#pragma once

#include <string>
#include <map>
#include <clang/Basic/SourceLocation.h>

// FIXME
using namespace clang;

// Holds the number of private or protected variables, methods, types
// in a class.
struct ClassCounts {
  int privateVarsCount = 0;
  int privateMethodsCount = 0;
  int privateTypesCount = 0;
};

struct Result {

  // Number of friend decls which refer to a class or class template
  int friendClassDeclCount = 0;
  // Number of friend decls which refer to a function or function template
  int friendFuncDeclCount = 0;

  struct FuncResult {

    std::string diagName;
    SourceLocation friendDeclLoc; // The location of the friend declaration
    std::string friendDeclLocStr;
    SourceLocation defLoc; // The location of the definition of the friend
    std::string defLocStr;

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
  using FriendDeclId = std::string;
  using BefriendingClassInstantiationId = std::string;
  using FunctionTemplateInstantiationId = std::string;
  using FuncResultKey = std::pair<BefriendingClassInstantiationId,
                                  FunctionTemplateInstantiationId>;
  using FuncResultsForFriendDecl =
      std::map<FuncResultKey, FuncResult>;
  std::map<FriendDeclId, FuncResultsForFriendDecl> FuncResults;

  struct ClassResult {
    std::string diagName;
    FuncResultsForFriendDecl memberFuncResults;
  };
  // Each friend class template could have different specializations or
  // instantiations. We handle one non-template class exactly the same as it was
  // an instantiation/specialization of a class template. We collect statistics
  // from every member functions of such records(class template inst/spec or
  // non-template class). Also we collect stats from member function templates.
  // We do this exactly the same as we do it with direct friend functions and
  // function templates.
  using ClassTemplateInstantiationId = std::string;
  using ClassResultKey = std::pair<BefriendingClassInstantiationId,
                                  ClassTemplateInstantiationId>;
  using ClassResultsForFriendDecl =
      std::map<ClassResultKey, ClassResult>;
  std::map<FriendDeclId, ClassResultsForFriendDecl> ClassResults;
};

