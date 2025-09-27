// SPDX-License-Identifier: MIT

#include "file_desc.h"
#include <gtest/gtest.h>

namespace plac {
namespace {

TEST(FileDescTest, DefaultConstructor) {
  FileDesc f{};
  EXPECT_FALSE(f.IsValid());
}

TEST(FileDescTest, Constructor) {
  FileDesc f{"/dev/null"};
  EXPECT_TRUE(f.IsValid());
}

TEST(FileDescTest, MoveConstructor) {
  FileDesc f1{"/dev/null"};
  EXPECT_TRUE(f1.IsValid());
  FileDesc f2{std::move(f1)};
  EXPECT_FALSE(f1.IsValid());
  EXPECT_TRUE(f2.IsValid());
}

TEST(FileDescTest, MoveAssignment) {
  FileDesc f1{"/dev/null"};
  EXPECT_TRUE(f1.IsValid());
  FileDesc f2{"/dev/zero"};
  EXPECT_TRUE(f2.IsValid());
  f2 = std::move(f1);
  EXPECT_FALSE(f1.IsValid());
  EXPECT_TRUE(f2.IsValid());
}

TEST(FileDescTest, SelfMoveAssignment) {
    FileDesc f{"/dev/null"};
    EXPECT_TRUE(f.IsValid());
    FileDesc& alias{f}; // silence warning for intentional self-assign
    f = std::move(alias);
    EXPECT_TRUE(f.IsValid());
}

} // namespace
} // namespace plac
