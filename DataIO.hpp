#pragma once

#include "FriendStats.hpp"
#include "DataCrunching.hpp"

inline void print(const Result::FuncResult &funcRes) {
  llvm::outs() << "friendDeclLoc: " << funcRes.friendDeclLocStr << "\n";
  llvm::outs() << "defLoc: " << funcRes.defLocStr << "\n";
  llvm::outs() << "diagName: " << funcRes.diagName << "\n";
  llvm::outs() << "usedPrivateVarsCount: " << funcRes.usedPrivateVarsCount
               << "\n";
  llvm::outs() << "parentPrivateVarsCount: " << funcRes.parentPrivateVarsCount
               << "\n";
  llvm::outs() << "usedPrivateMethodsCount: " << funcRes.usedPrivateMethodsCount
               << "\n";
  llvm::outs() << "parentPrivateMethodsCount: "
               << funcRes.parentPrivateMethodsCount << "\n";
  llvm::outs() << "types.usedPrivateCount: " << funcRes.types.usedPrivateCount
               << "\n";
  llvm::outs() << "types.parentPrivateCount: "
               << funcRes.types.parentPrivateCount << "\n";
}

inline raw_ostream &operator<<(raw_ostream &os,
                                const std::pair<int, int>& p) {
  os << "(" << p.first << "," << p.second << ")";
  return os;
}

template <typename T>
inline raw_ostream &operator<<(raw_ostream &os,
                                const DiscreteDistribution<T>& d) {
  const auto& store = d.get();
  for (const auto& v : store) {
    std::string s;
    llvm::raw_string_ostream ss{s};
    ss << v.first;
    if(ss.str().size() < 8) {
      os << v.first << "\t\t" << v.second << "\n";
    } else {
      os << v.first << "\t" << v.second << "\n";
    }
  }
  return os;
}

