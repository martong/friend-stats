#pragma once

#include <map>
#include "FriendStats.hpp"

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

struct Average {
  double sum = 0.0;
  int num = 0;
  int numZeroDenom = 0;
  void operator()(const Result::FuncResult &funcRes) {
    int numerator = funcRes.usedPrivateVarsCount +
                    funcRes.usedPrivateMethodsCount +
                    funcRes.types.usedPrivateCount;
    int denominator = funcRes.parentPrivateVarsCount +
                      funcRes.parentPrivateMethodsCount +
                      funcRes.types.parentPrivateCount;
    ++num;
    // If denominator is zero that means there are no priv or protected
    // entitties
    // in the class, only publicly availble entities are there.
    // If a friend function accesses only public entites that means, it should
    // not be friend at all, therefore we add nothing (zero) to sum in such
    // cases.
    if (denominator) {
      sum += static_cast<double>(numerator) / denominator;
    } else {
      // llvm::outs() << "ZERO PRIV" << "\n";
      //<< "\n";
      ++numZeroDenom;
    }
    // print(funcRes);
  }
  double get() const { return sum / num; }
};

struct SelfDiagnostics {
  void operator()(const Result::FuncResult &funcRes) {
    if (funcRes.usedPrivateVarsCount > funcRes.parentPrivateVarsCount ||
        funcRes.usedPrivateMethodsCount > funcRes.parentPrivateMethodsCount ||
        funcRes.types.usedPrivateCount > funcRes.types.parentPrivateCount) {
      llvm::errs() << "WRONG MEASURE here: \n" << funcRes.friendDeclLocStr
                   << "\n";
      print(funcRes);
      exit(1);
    }
  }
};

template <typename T> class DiscreteDistribution {
  using Store = std::map<T, int>;
  Store store;

public:
  void addValue(const T &value) { ++store[value]; }
  const Store &get() const { return store; }
};

inline std::pair<int, int> getInterval(const double &d) {
  return std::make_pair(int(d), int(d + 1.));
}

