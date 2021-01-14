// SPDX-License-Identifier: MIT

#include "audio_buffer.h"
#include <array>
#include <cstring>
#include <gtest/gtest.h>

namespace plac {
namespace {

class AudioBufferTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}

  std::array<int, 12> left_{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  std::array<int, 12> right_{12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
  AudioFormat format_{24, 2, 44100};
};

TEST_F(AudioBufferTest, Construct) {
  AudioBuffer<24> buffer;
  auto reader = [](AudioFormat, const u_char *, const size_t count) { return static_cast<ssize_t>(count); };

  EXPECT_FLOAT_EQ(0.0F, buffer.GetFillLevel());
  EXPECT_TRUE(buffer.IsEmpty());
  EXPECT_EQ(0, buffer.Read(format_, 23, reader));
}

TEST_F(AudioBufferTest, Write) {
  AudioBuffer<24> buffer;

  EXPECT_EQ(1, buffer.Write(format_, left_.cbegin(), right_.cbegin(), 1));
  EXPECT_FLOAT_EQ(0.25F, buffer.GetFillLevel());
  EXPECT_FALSE(buffer.IsEmpty());

  EXPECT_EQ(2, buffer.Write(format_, left_.cbegin(), right_.cbegin(), 2));
  EXPECT_FLOAT_EQ(0.75F, buffer.GetFillLevel());
  EXPECT_FALSE(buffer.IsEmpty());

  EXPECT_EQ(1, buffer.Write(format_, left_.cbegin(), right_.cbegin(), 2));
  EXPECT_FLOAT_EQ(1.0F, buffer.GetFillLevel());
  EXPECT_FALSE(buffer.IsEmpty());
}

TEST_F(AudioBufferTest, Read) {
  AudioBuffer<24> buffer;
  auto reader = [](AudioFormat, const u_char *, const size_t count) { return static_cast<ssize_t>(count); };

  EXPECT_EQ(4, buffer.Write(format_, left_.cbegin(), right_.cbegin(), 4));
  EXPECT_FLOAT_EQ(1.0F, buffer.GetFillLevel());
  EXPECT_FALSE(buffer.IsEmpty());

  EXPECT_EQ(4, buffer.Read(format_, 4, reader));
  EXPECT_FLOAT_EQ(0.0F, buffer.GetFillLevel());
  EXPECT_TRUE(buffer.IsEmpty());

  EXPECT_EQ(0, buffer.Read(format_, 4, reader));
  EXPECT_FLOAT_EQ(0.0F, buffer.GetFillLevel());
  EXPECT_TRUE(buffer.IsEmpty());
}

TEST_F(AudioBufferTest, ReadWhenWrapAround) {
  AudioBuffer<24> buffer;
  auto reader = [](AudioFormat, const u_char *, const size_t count) { return static_cast<ssize_t>(count); };

  EXPECT_EQ(3, buffer.Write(format_, left_.cbegin(), right_.cbegin(), 3));
  EXPECT_EQ(0, buffer.Drain(format_, reader));
  EXPECT_EQ(4, buffer.Write(format_, left_.cbegin(), right_.cbegin(), 4));

  EXPECT_EQ(1, buffer.Read(format_, 4, reader));
}

TEST_F(AudioBufferTest, ReadWhenError) {
  AudioBuffer<24> buffer;
  auto reader = [](AudioFormat, const u_char *, size_t) { return -23; };

  EXPECT_EQ(3, buffer.Write(format_, left_.cbegin(), right_.cbegin(), 3));
  EXPECT_FLOAT_EQ(0.75F, buffer.GetFillLevel());

  EXPECT_EQ(-23, buffer.Read(format_, 3, reader));
  EXPECT_FLOAT_EQ(0.75F, buffer.GetFillLevel());
}

TEST_F(AudioBufferTest, DrainWhenError) {
  AudioBuffer<24> buffer;
  auto reader = [](AudioFormat, const u_char *, size_t) { return -23; };

  EXPECT_EQ(3, buffer.Write(format_, left_.cbegin(), right_.cbegin(), 3));
  EXPECT_FLOAT_EQ(0.75F, buffer.GetFillLevel());

  EXPECT_EQ(-23, buffer.Drain(format_, reader));
  EXPECT_FLOAT_EQ(0.75F, buffer.GetFillLevel());
}

TEST_F(AudioBufferTest, WriteRead) {
  AudioBuffer<24> buffer;
  auto reader = [](AudioFormat, const u_char *, const size_t count) { return static_cast<ssize_t>(count); };

  EXPECT_EQ(4, buffer.Write(format_, left_.cbegin(), right_.cbegin(), 4));
  EXPECT_FLOAT_EQ(1.0F, buffer.GetFillLevel());
  EXPECT_FALSE(buffer.IsEmpty());

  EXPECT_EQ(0, buffer.Write(format_, left_.cbegin(), right_.cbegin(), 4));
  EXPECT_FLOAT_EQ(1.0F, buffer.GetFillLevel());
  EXPECT_FALSE(buffer.IsEmpty());

  EXPECT_EQ(0, buffer.Read(format_, 6, reader));
  EXPECT_FLOAT_EQ(1.0F, buffer.GetFillLevel());
  EXPECT_FALSE(buffer.IsEmpty());

  EXPECT_EQ(2, buffer.Read(format_, 2, reader));
  EXPECT_FLOAT_EQ(0.5F, buffer.GetFillLevel());
  EXPECT_FALSE(buffer.IsEmpty());

  EXPECT_EQ(2, buffer.Write(format_, left_.cbegin(), right_.cbegin(), 4));
  EXPECT_FLOAT_EQ(1.0F, buffer.GetFillLevel());
  EXPECT_FALSE(buffer.IsEmpty());
}

TEST_F(AudioBufferTest, Recover) {
  AudioBuffer<24> buffer;
  std::vector<int> left_recovered;
  std::vector<int> right_recovered;

  auto reader = [&left_recovered, &right_recovered](const AudioFormat format, const u_char *const data,
                                                    const size_t count) {
    const auto channel_size = format.bits / 8;
    const auto frame_size = channel_size * 2;

    for (size_t i{0}; i < count; ++i) {
      int left{};
      std::memcpy(&left, &data[frame_size * i], channel_size);
      left_recovered.push_back(left);
      int right{};
      std::memcpy(&right, &data[frame_size * i + channel_size], channel_size);
      right_recovered.push_back(right);
    }
    return static_cast<ssize_t>(count);
  };

  auto left = left_.cbegin();
  auto right = right_.cbegin();
  for (; (left != left_.cend()) || (right != right_.cend()); left += 3, right += 3) {
    EXPECT_EQ(3, buffer.Write(format_, left, right, 3));
    EXPECT_EQ(0, buffer.Drain(format_, reader));
  }

  EXPECT_TRUE(std::equal(left_.cbegin(), left_.cend(), left_recovered.cbegin(), left_recovered.cend()));
  EXPECT_TRUE(std::equal(right_.cbegin(), right_.cend(), right_recovered.cbegin(), right_recovered.cend()));
}

} // namespace
} // namespace plac
