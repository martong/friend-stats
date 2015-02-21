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

struct PrivateUsage {
  int numerator = 0;
  int denominator = 0;
  double usage = 0.0;
};
inline PrivateUsage privateUsage(const Result::FuncResult &funcRes) {
  PrivateUsage usage;
  usage.numerator = funcRes.usedPrivateVarsCount +
                    funcRes.usedPrivateMethodsCount +
                    funcRes.types.usedPrivateCount;
  usage.denominator = funcRes.parentPrivateVarsCount +
                      funcRes.parentPrivateMethodsCount +
                      funcRes.types.parentPrivateCount;
  if (usage.denominator) {
    usage.usage = static_cast<double>(usage.numerator) / usage.denominator;
  }
  return usage;
}

struct Average {
  double sum = 0.0;
  int num = 0;
  int numZeroDenom = 0;
  void operator()(const Result::FuncResult &funcRes) {
    PrivateUsage usage = privateUsage(funcRes);
    ++num;
    // If denominator is zero that means there are no priv or protected
    // entitties
    // in the class, only publicly availble entities are there.
    // If a friend function accesses only public entites that means, it should
    // not be friend at all, therefore we add nothing (zero) to sum in such
    // cases.
    if (usage.denominator) {
      sum += usage.usage;
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

inline std::pair<int, int> getInterval(const double &d) {
  return std::make_pair(int(d), int(d + 1.));
}

struct PercentageDistribution {
  DiscreteDistribution<std::pair<int, int>> dist;
  void operator()(const Result::FuncResult &funcRes) {
    PrivateUsage usage = privateUsage(funcRes);
    if (usage.usage == 0.0) {
      dist.addValue(getInterval(-.5));
    } else {
      dist.addValue(getInterval(usage.usage * 100));
    }
  }
};

struct NumberOfUsedPrivsDistribution {
  DiscreteDistribution<int> dist;
  void operator()(const Result::FuncResult &funcRes) {
    PrivateUsage usage = privateUsage(funcRes);
    dist.addValue(usage.numerator);
  }
};

// Ideal candidate for "friend for" is with the highest candidate value.
// 0 < candidate value <= 100
// candidate value == -1 means it should not be a friend at all
struct CandidateDistribution {
  DiscreteDistribution<std::pair<int, int>> dist;
  void operator()(const Result::FuncResult &funcRes) {
    PrivateUsage usage = privateUsage(funcRes);
    if (usage.numerator != 0 && usage.usage != 0.0) {
      double candidate = 1.0 / (usage.numerator * usage.usage);
      dist.addValue(getInterval(candidate));
    }
    else {
      dist.addValue(getInterval(-.5));
    }
  }
};

