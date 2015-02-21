#pragma once

#include <map>

// FIXME don't do this in headers
// using namespace clang;

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

