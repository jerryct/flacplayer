// SPDX-License-Identifier: MIT

#include "audio_format.h"
#include <gtest/gtest.h>

namespace plac {
namespace {

TEST(AudioFormatTest, Equality) {
  EXPECT_EQ((AudioFormat{24, 2, 44100}), (AudioFormat{24, 2, 44100}));
  EXPECT_NE((AudioFormat{24, 2, 44100}), (AudioFormat{23, 2, 44100}));
  EXPECT_NE((AudioFormat{24, 2, 44100}), (AudioFormat{24, 1, 44100}));
  EXPECT_NE((AudioFormat{24, 2, 44100}), (AudioFormat{24, 2, 44000}));
}

TEST(AudioFormatTest, AsBytes) { EXPECT_EQ(6, AsBytes(AudioFormat{24, 2, 1}, 1)); }
TEST(AudioFormatTest, AsFrames) { EXPECT_EQ(1, AsFrames(AudioFormat{24, 2, 1}, 6)); }

} // namespace
} // namespace plac
