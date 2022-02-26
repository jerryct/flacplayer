// SPDX-License-Identifier: MIT

#include "audio_buffer.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <gtest/gtest.h>
#include <numeric>
#include <vector>

namespace plac {
namespace {

struct Reader {
  ssize_t operator()(const AudioFormat format, const u_char *data, const size_t count) {
    if (do_error_injection_) {
      return -23;
    } else {
      const auto channel_size = format.bits / 8;
      const auto frame_size = channel_size * 2;

      for (size_t i{0}; i < count; ++i) {
        int left{};
        std::memcpy(&left, &data[frame_size * i], channel_size);
        left_.push_back(left);
        int right{};
        std::memcpy(&right, &data[frame_size * i + channel_size], channel_size);
        right_.push_back(right);
      }
      return static_cast<ssize_t>(count);
    }
  };
  void Flush() {
    left_.clear();
    right_.clear();
  }

  bool do_error_injection_{false};
  std::vector<int> left_{};
  std::vector<int> right_{};
};

template <typename T> class AudioBufferTest : public ::testing::Test {
protected:
  void SetUp() override {
    std::iota(left_.begin(), left_.end(), 1);
    std::copy(left_.cbegin(), left_.cend(), right_.rbegin());
  }
  void TearDown() override {}

  void FillBuffer() {
    while (buffer_.GetFillLevel() < 1.0F) {
      buffer_.Write(format_, left_.cbegin(), right_.cbegin(), 8);
    }
    ASSERT_FLOAT_EQ(1.0F, buffer_.GetFillLevel());
  }

  void FillBufferExceptLast() {
    const size_t last{NumberOfSamplesPerStride() - 1};
    for (size_t i{0}; i < last; ++i) {
      buffer_.Write(format_, left_.cbegin(), right_.cbegin(), 8);
    }
    ASSERT_FLOAT_EQ(last / static_cast<float>(NumberOfSamplesPerStride()), buffer_.GetFillLevel());
  }

  // 16bps: 4 Bytes per sample, 192 Bytes have 48 samples, 5 write with 8 samples per call/stride
  // 24bps: 6 Bytes per sample, 192 Bytes have 32 samples, 4 write with 8 samples per call/stride
  // 32bps: 8 Bytes per sample, 192 Bytes have 24 samples, 3 write with 8 samples per call/stride
  size_t NumberOfSamplesPerStride() const { return AsFrames(format_, 192) / 8; }

  std::array<int, 16> left_;
  std::array<int, 16> right_;
  AudioFormat format_{T(), 2, 44100};
  Reader reader_;

  AudioBuffer<192> buffer_;
};

using MyTypes = ::testing::Types<std::integral_constant<unsigned, 16>, std::integral_constant<unsigned, 24>>;
TYPED_TEST_SUITE(AudioBufferTest, MyTypes);

TYPED_TEST(AudioBufferTest, Construct) {
  EXPECT_FLOAT_EQ(0.0F, this->buffer_.GetFillLevel());
  EXPECT_TRUE(this->buffer_.IsEmpty());
}

TYPED_TEST(AudioBufferTest, GetFillLevel) {
  int i{1};
  while (this->buffer_.GetFillLevel() < 1.0F) {
    EXPECT_EQ(8, this->buffer_.Write(this->format_, this->left_.cbegin(), this->right_.cbegin(), 8));
    EXPECT_FLOAT_EQ(i / static_cast<float>(this->NumberOfSamplesPerStride()), this->buffer_.GetFillLevel());
    EXPECT_FALSE(this->buffer_.IsEmpty());
    ++i;
  }
}

TYPED_TEST(AudioBufferTest, Write) {
  EXPECT_EQ(8, this->buffer_.Write(this->format_, this->left_.cbegin(), this->right_.cbegin(), 8));
}

TYPED_TEST(AudioBufferTest, WriteWhenFull) {
  this->FillBuffer();

  EXPECT_EQ(0, this->buffer_.Write(this->format_, this->left_.cbegin(), this->right_.cbegin(), 8));
}

TYPED_TEST(AudioBufferTest, WriteWhenTooManyFrames) {
  this->FillBufferExceptLast();

  EXPECT_EQ(8, this->buffer_.Write(this->format_, this->left_.cbegin(), this->right_.cbegin(), 16));
}

TYPED_TEST(AudioBufferTest, Read) {
  EXPECT_EQ(8, this->buffer_.Write(this->format_, this->left_.cbegin(), this->right_.cbegin(), 8));

  EXPECT_EQ(8, this->buffer_.Read(this->format_, 8, this->reader_));
  EXPECT_EQ(this->reader_.left_, (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8}));
  EXPECT_EQ(this->reader_.right_, (std::vector<int>{16, 15, 14, 13, 12, 11, 10, 9}));
}

TYPED_TEST(AudioBufferTest, ReadWhenEmpty) {
  ASSERT_TRUE(this->buffer_.IsEmpty());

  EXPECT_EQ(0, this->buffer_.Read(this->format_, 8, this->reader_));
  EXPECT_EQ(0, this->reader_.left_.size());
  EXPECT_EQ(0, this->reader_.right_.size());
}

TYPED_TEST(AudioBufferTest, ReadWhenTooManyFrames) {
  EXPECT_EQ(8, this->buffer_.Write(this->format_, this->left_.cbegin(), this->right_.cbegin(), 8));

  EXPECT_EQ(0, this->buffer_.Read(this->format_, 9, this->reader_));
  EXPECT_EQ(0, this->reader_.left_.size());
  EXPECT_EQ(0, this->reader_.right_.size());
}

TYPED_TEST(AudioBufferTest, ReadWhenWrapAroundWithoutRest) {
  this->FillBuffer();
  EXPECT_EQ(0, this->buffer_.Drain(this->format_, this->reader_));
  ASSERT_TRUE(this->buffer_.IsEmpty());
  this->reader_.Flush();
  EXPECT_EQ(8, this->buffer_.Write(this->format_, this->left_.cbegin(), this->right_.cbegin(), 8));

  EXPECT_EQ(8, this->buffer_.Read(this->format_, 8, this->reader_));
  EXPECT_EQ(this->reader_.left_, (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8}));
  EXPECT_EQ(this->reader_.right_, (std::vector<int>{16, 15, 14, 13, 12, 11, 10, 9}));
}

TYPED_TEST(AudioBufferTest, ReadWhenWrapAroundWithRest) {
  this->FillBufferExceptLast();
  EXPECT_EQ(0, this->buffer_.Drain(this->format_, this->reader_));
  ASSERT_TRUE(this->buffer_.IsEmpty());
  this->reader_.Flush();
  EXPECT_EQ(16, this->buffer_.Write(this->format_, this->left_.cbegin(), this->right_.cbegin(), 16));

  EXPECT_EQ(8, this->buffer_.Read(this->format_, 16, this->reader_));
  EXPECT_EQ(8, this->buffer_.Read(this->format_, 8, this->reader_));
  EXPECT_EQ(this->reader_.left_, (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
  EXPECT_EQ(this->reader_.right_, (std::vector<int>{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1}));
}

TYPED_TEST(AudioBufferTest, ReadWhenError) {
  this->reader_.do_error_injection_ = true;
  this->FillBuffer();

  EXPECT_EQ(-23, this->buffer_.Read(this->format_, 3, this->reader_));
  EXPECT_EQ(0, this->reader_.left_.size());
  EXPECT_EQ(0, this->reader_.right_.size());
}

TYPED_TEST(AudioBufferTest, Drain) {
  EXPECT_EQ(16, this->buffer_.Write(this->format_, this->left_.cbegin(), this->right_.cbegin(), 16));

  EXPECT_EQ(0, this->buffer_.Drain(this->format_, this->reader_));
  EXPECT_TRUE(this->buffer_.IsEmpty());
  EXPECT_EQ(this->reader_.left_, (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
  EXPECT_EQ(this->reader_.right_, (std::vector<int>{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1}));
}

TYPED_TEST(AudioBufferTest, DrainWhenFull) {
  this->FillBuffer();

  EXPECT_EQ(0, this->buffer_.Drain(this->format_, this->reader_));
  EXPECT_TRUE(this->buffer_.IsEmpty());
  EXPECT_EQ(AsFrames(this->format_, 192), this->reader_.left_.size());
  EXPECT_EQ(AsFrames(this->format_, 192), this->reader_.right_.size());
}

TYPED_TEST(AudioBufferTest, DrainWhenError) {
  this->reader_.do_error_injection_ = true;
  this->FillBuffer();

  EXPECT_EQ(-23, this->buffer_.Drain(this->format_, this->reader_));
  EXPECT_EQ(0, this->reader_.left_.size());
  EXPECT_EQ(0, this->reader_.right_.size());
}

} // namespace
} // namespace plac
