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

struct FriendStatsHeader : ::testing::Test {
  SmallString<128> CurrentDir;
  SmallString<128> HeaderA;
  SmallString<128> FileA;
  SmallString<128> FileB;
  std::unique_ptr<tooling::FixedCompilationDatabase> Compilations;
  std::unique_ptr<tooling::ClangTool> Tool;
  std::vector<std::string> Sources;
  FriendHandler Handler;
  TuHandler tuHandler;
  MatchFinder Finder;
  FriendStatsHeader() {
    // The directory used is not important since the path gets mapped to a
    // virtual
    // file anyway. What is important is that we have an absolute path with
    // which
    // to use with mapVirtualFile().
    std::error_code EC = llvm::sys::fs::current_path(CurrentDir);
    assert(!EC);
    (void)EC;

    HeaderA = CurrentDir;
    llvm::sys::path::append(HeaderA, "a.h");
    FileA = CurrentDir;
    llvm::sys::path::append(FileA, "a.cc");
    FileB = CurrentDir;
    llvm::sys::path::append(FileB, "b.cc");

    Compilations.reset(new tooling::FixedCompilationDatabase(CurrentDir.str(),
                                                             {"-std=c++14"}));
    // Sources.push_back(HeaderA.str());
    Sources.push_back(FileA.str());
    Sources.push_back(FileB.str());
    Tool.reset(new tooling::ClangTool(*Compilations, Sources));

    Finder.addMatcher(FriendMatcher, &Handler);
    // Dump the whole ast of the translation unit if debug is on
    Finder.addMatcher(TuMatcher, &tuHandler);
  }
};


inline const Result::FuncResult &getFirstFuncResult(const Result &res) {
  return (*res.FuncResults.begin()->second.begin()).second;
}
inline const Result::FuncResultsForFriendDecl::value_type &getFirstFuncResPair(const Result &res) {
  return (*res.FuncResults.begin()->second.begin());
}
inline const Result::FuncResultsForFriendDecl &
getFuncResultsFor1stFriendDecl(const Result &res) {
  return res.FuncResults.begin()->second;
}
inline const Result::FuncResultsForFriendDecl &
getFuncResultsFor2ndFriendDecl(const Result &res) {
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

// Class specific result getters
inline const Result::ClassResultsForFriendDecl &
getClassResultsFor1stFriendDecl(const Result &res) {
  return res.ClassResults.begin()->second;
}
inline const Result::ClassResultsForFriendDecl &
getClassResultsFor2ndFriendDecl(const Result &res) {
  return (++res.ClassResults.begin())->second;
}
inline const Result::ClassResult &
get1stClassResult(const Result::ClassResultsForFriendDecl &r) {
  return r.begin()->second;
}
inline const Result::ClassResult &
get2ndClassResult(const Result::ClassResultsForFriendDecl &r) {
  assert(r.size() > 1);
  return (++r.begin())->second;
}
inline const Result::ClassResult &
get3rdClassResult(const Result::ClassResultsForFriendDecl &r) {
  assert(r.size() > 2);
  auto it = r.begin();
  ++it;
  ++it;
  return it->second;
}
// TODO use getNthMemberFuncResult(const Result::ClassResult &r, std::size_t N)
inline const Result::FuncResult &
get1stMemberFuncResult(const Result::ClassResult &r) {
  //return r.memberFuncResults.at(0).funcResult;
  assert(r.memberFuncResults.size() > 0);
  return r.memberFuncResults.begin()->second;
}
inline const Result::FuncResult &
get2ndMemberFuncResult(const Result::ClassResult &r) {
  assert(r.memberFuncResults.size() > 1);
  auto it = r.memberFuncResults.begin();
  ++it;
  return it->second;
}
inline const Result::FuncResult &
getNthMemberFuncResult(const Result::ClassResult &r, std::size_t N) {
  assert(r.memberFuncResults.size() > N-1);
  auto it = r.memberFuncResults.begin();
  std::advance(it, N-1);
  return it->second;
}
inline const Result::FuncResult &getFirstMemberFuncResult(const Result &res) {
  return get1stMemberFuncResult(
      get1stClassResult(getClassResultsFor1stFriendDecl(res)));
}

