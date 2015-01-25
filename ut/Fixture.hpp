#pragma once

#include <gtest/gtest.h>
#include "clang/Tooling/Tooling.h"
#include "../FriendStats.hpp"

// Note, it is not nice to do this in a header,
// but there are no plans to make this visible, or to be used
// as a lib.
using namespace clang::tooling;
using namespace llvm;
using namespace clang;

struct FriendStats : ::testing::Test {
  SmallString<128> CurrentDir;
  SmallString<128> FileA;
  std::unique_ptr<tooling::FixedCompilationDatabase> Compilations;
  std::unique_ptr<tooling::ClangTool> Tool;
  std::vector<std::string> Sources;
  FriendHandler Handler;
  TuHandler tuHandler;
  MatchFinder Finder;
  FriendStats() {
    // The directory used is not important since the path gets mapped to a
    // virtual
    // file anyway. What is important is that we have an absolute path with
    // which
    // to use with mapVirtualFile().
    std::error_code EC = llvm::sys::fs::current_path(CurrentDir);
    assert(!EC);
    (void)EC;

    FileA = CurrentDir;
    llvm::sys::path::append(FileA, "a.cc");

    Compilations.reset(new tooling::FixedCompilationDatabase(CurrentDir.str(),
                                                             {"-std=c++14"}));
    Sources.push_back(FileA.str());
    Tool.reset(new tooling::ClangTool(*Compilations, Sources));

    Finder.addMatcher(FriendMatcher, &Handler);
    // Dump the whole ast of the translation unit if debug is on
    Finder.addMatcher(TuMatcher, &tuHandler);
  }
};

inline const Result::FuncResultsForFriendDecl &
getResultsFor1stFriendDecl(const Result &res) {
  return res.FuncResults.begin()->second;
}
inline const Result::FuncResultsForFriendDecl &
getResultsFor2ndFriendDecl(const Result &res) {
  assert(res.FuncResults.size() > 1);
  return (++res.FuncResults.begin())->second;
}
inline const Result::FuncResult &
get1stFuncResult(const Result::FuncResultsForFriendDecl &r) {
  return r.begin()->second;
}
inline const Result::FuncResult &
get2ndFuncResult(const Result::FuncResultsForFriendDecl &r) {
  assert(r.size() > 1);
  return (++r.begin())->second;
}

inline const Result::FuncResult &getFirstFuncResult(const Result &res) {
  return (*res.FuncResults.begin()->second.begin()).second;
}
inline const Result::FuncResult &getFirstMemberFuncResult(const Result &res) {
  return res.ClassResults.begin()
      ->second.at(0)
      .memberFuncResults.front()
      .funcResult;
}

