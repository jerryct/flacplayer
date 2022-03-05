// SPDX-License-Identifier: MIT

#include "align.h"
#include <gtest/gtest.h>

namespace plac {
namespace {

TEST(AlignUp, AlignUp) {
  EXPECT_EQ(0, AlignUp(0));
  EXPECT_EQ(32, AlignUp(1));
  EXPECT_EQ(32, AlignUp(31));
  EXPECT_EQ(32, AlignUp(32));
  EXPECT_EQ(64, AlignUp(33));
}

TEST(AlignDown, AlignDown) {
  EXPECT_EQ(-8, AlignDown(-5));
  EXPECT_EQ(-4, AlignDown(-4));
  EXPECT_EQ(-4, AlignDown(-3));
  EXPECT_EQ(-4, AlignDown(-2));
  EXPECT_EQ(-4, AlignDown(-1));
  EXPECT_EQ(0, AlignDown(0));
  EXPECT_EQ(0, AlignDown(1));
  EXPECT_EQ(0, AlignDown(2));
  EXPECT_EQ(0, AlignDown(3));
  EXPECT_EQ(4, AlignDown(4));
  EXPECT_EQ(4, AlignDown(5));
}

} // namespace
} // namespace plac
