#include <gtest/gtest.h>
#include "../DataCrunching.hpp"

TEST(getInterval, First) {
  double d = 0.2345;
  auto p = getInterval(d);
  EXPECT_EQ(p.first, 0);
  EXPECT_EQ(p.second, 1);
}

TEST(getInterval, Zero) {
  double d = 0.0;
  auto p = getInterval(d);
  EXPECT_EQ(p.first, 0);
  EXPECT_EQ(p.second, 1);
}

TEST(getInterval, One) {
  double d = 1.0;
  auto p = getInterval(d);
  EXPECT_EQ(p.first, 1);
  EXPECT_EQ(p.second, 2);
}

