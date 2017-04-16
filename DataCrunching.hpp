#pragma once

#include <map>
#include <unordered_set>
#include "FriendStats.hpp"

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
      ++numZeroDenom;
    }
  }
  double get() const { return sum / num; }
};

struct SelfDiagnostics {
  // Returns true if the stat entry is sane
  bool operator()(const Result::FuncResult &funcRes) {
    if (funcRes.usedPrivateVarsCount > funcRes.parentPrivateVarsCount ||
        funcRes.usedPrivateMethodsCount > funcRes.parentPrivateMethodsCount ||
        funcRes.types.usedPrivateCount > funcRes.types.parentPrivateCount) {
      return false;
    }
    return true;
  }
};

struct HostClassesWithZeroPrivate {
  std::unordered_set<std::shared_ptr<ClassInfo>> classes;
  void operator()(const Result::FuncResult &funcRes) {
    PrivateUsage usage = privateUsage(funcRes);
    if (usage.denominator == 0 ) {
      classes.insert(funcRes.parentClassInfo);
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

struct ZeroPrivInHost {
  bool operator()(const Result::FuncResult &funcRes) {
    PrivateUsage usage = privateUsage(funcRes);
    return usage.denominator == 0.0;
  }
};

// friendly function instances with zero private usage
struct ZeroPrivInFriend {
  bool operator()(const Result::FuncResult &funcRes) {
    PrivateUsage usage = privateUsage(funcRes);
    return usage.usage == 0.0;
  }
};

struct StrongCandidate {
  std::size_t count = 0;
  bool operator()(const Result::FuncResult &funcRes) {
    PrivateUsage usage = privateUsage(funcRes);
    bool result = (1 <= usage.numerator && usage.numerator <= 3) &&
                  (0.0 < usage.usage && usage.usage <= 0.5);
    if (result) {
      ++count;
    }
    return result;
  }
};

struct StrongCandidateBecauseMemberVars {
  std::size_t count = 0;
  bool operator()(const Result::FuncResult &funcRes) {
    const auto &uc = funcRes.usedPrivateVarsCount;
    const double up = funcRes.parentPrivateVarsCount == 0
                          ? 0.0
                          : static_cast<double>(funcRes.usedPrivateVarsCount) /
                                funcRes.parentPrivateVarsCount;
    bool result = (1 <= uc && uc <= 3) && (0.0 < up && up <= 0.5);
    if (result) {
      ++count;
    }
    return result;
  }
};

struct MeyersCandidate {
  std::size_t count = 0;
  bool
  operator()(const Result::FuncResultsForFriendDecl::value_type &funcResPair) {
    const auto &key = funcResPair.first;
    const auto &funcRes = funcResPair.second;
    static ZeroPrivInFriend zpf;
    // TODO false positives: non template operator<, operator<= and operator<<
    bool match = zpf(funcRes) && key.first.find("<") != std::string::npos &&
                 funcRes.defLocStr == funcRes.friendDeclLocStr;
    if (match)
      ++count;
    return match;
  }
};

// possibly incorrect friend function instances
struct PossiblyIncorrectFriend {
  bool
  operator()(const Result::FuncResultsForFriendDecl::value_type &funcResPair) {
    const auto &funcRes = funcResPair.second;
    static ZeroPrivInHost zph;
    static ZeroPrivInFriend zpf;
    static MeyersCandidate mc;
    return !zph(funcRes) && zpf(funcRes) && !mc(funcResPair);
  }
};

struct IncorrectFriendClassFunctionInstance {
  bool operator()(const Result::FuncResult &funcRes) {
    static ZeroPrivInHost zph;
    static ZeroPrivInFriend zpf;
    return !zph(funcRes) && zpf(funcRes);
  }
};

struct IncorrectFriendClass {
  bool result = true;
  void operator()(const Result::FuncResult &funcRes) {
    static IncorrectFriendClassFunctionInstance incFunc;
    result = result && incFunc(funcRes);
  }
};

