// SPDX-License-Identifier: MIT

#include "audio_buffer.h"
#include "stream.h"
#include <gtest/gtest.h>

namespace plac {
namespace {

class FlacDecodeTest : public ::testing::Test {
protected:
  AudioBuffer<228000> audio_buffer_;
};

struct DummyWriter16Bps {
  ssize_t operator()(const AudioFormat format, const u_char *const data, const size_t count) {
    EXPECT_EQ(16, format.bits);
    for (int i = 0; i < count; ++i) {
      EXPECT_EQ(total & 0xFF, data[4 * i + 0]);
      EXPECT_EQ((total >> 8) & 0xFF, data[4 * i + 1]);
      EXPECT_EQ((total >> 16) & 0xFF, data[4 * i + 2]);
      EXPECT_EQ(total & 0xFF, data[4 * i + 3]);
      ++total;
    }

    return count;
  }
  size_t total = 0;
};

struct DummyWriter24Bps {
  ssize_t operator()(const AudioFormat format, const u_char *const data, const size_t count) {
    EXPECT_EQ(24, format.bits);
    for (int i = 0; i < count; ++i) {
      EXPECT_EQ((total >> 0) & 0xFF, data[6 * i + 0]);
      EXPECT_EQ((total >> 8) & 0xFF, data[6 * i + 1]);
      EXPECT_EQ((total >> 16) & 0xFF, data[6 * i + 2]);
      EXPECT_EQ((total >> 16) & 0xFF, data[6 * i + 3]);
      EXPECT_EQ((total >> 8) & 0xFF, data[6 * i + 4]);
      EXPECT_EQ((total >> 0) & 0xFF, data[6 * i + 5]);
      ++total;
    }

    return count;
  }
  size_t total = 0;
};

TEST_F(FlacDecodeTest, Play16Bps) {
  FlowControl flow{};
  Stream stream{audio_buffer_, flow};
  DummyWriter16Bps writer{};

  for (const char *name : {
           "../assets/16bps_part0.flac",
           "../assets/16bps_part1.flac",
           "../assets/16bps_part2.flac",
           "../assets/16bps_part3.flac",
           "../assets/16bps_part4.flac",
           "../assets/16bps_part5.flac",
           "../assets/16bps_part6.flac",
           "../assets/16bps_part7.flac",
           "../assets/16bps_part8.flac",
           "../assets/16bps_part9.flac",
           "../assets/16bps_part10.flac",
           "../assets/16bps_part11.flac",
           "../assets/16bps_part12.flac",
           "../assets/16bps_part13.flac",
           "../assets/16bps_part14.flac",
       }) {
    ASSERT_TRUE(stream.Reset(name));

    FLAC__StreamDecoderState state = FLAC__stream_decoder_get_state(stream.decoder_);
    while (state != FLAC__STREAM_DECODER_END_OF_STREAM) {
      const ssize_t r{audio_buffer_.Read(stream.format_, 5513, writer)};
      ASSERT_GE(r, 0);
      ASSERT_LE(r, 5513);

      ASSERT_TRUE(FLAC__stream_decoder_process_single(stream.decoder_));
      state = FLAC__stream_decoder_get_state(stream.decoder_);
      usleep(100);
    }
  }
  audio_buffer_.Drain(stream.format_, writer);

  EXPECT_EQ(150000, writer.total);
  EXPECT_TRUE(audio_buffer_.IsEmpty());
}

TEST_F(FlacDecodeTest, Play24Bps) {
  FlowControl flow{};
  Stream stream{audio_buffer_, flow};
  DummyWriter24Bps writer{};

  for (const char *name : {
           "../assets/24bps_part0.flac",
           "../assets/24bps_part1.flac",
           "../assets/24bps_part2.flac",
           "../assets/24bps_part3.flac",
           "../assets/24bps_part4.flac",
           "../assets/24bps_part5.flac",
           "../assets/24bps_part6.flac",
           "../assets/24bps_part7.flac",
           "../assets/24bps_part8.flac",
           "../assets/24bps_part9.flac",
           "../assets/24bps_part10.flac",
           "../assets/24bps_part11.flac",
           "../assets/24bps_part12.flac",
           "../assets/24bps_part13.flac",
           "../assets/24bps_part14.flac",
       }) {
    ASSERT_TRUE(stream.Reset(name));

    FLAC__StreamDecoderState state = FLAC__stream_decoder_get_state(stream.decoder_);
    while (state != FLAC__STREAM_DECODER_END_OF_STREAM) {
      const ssize_t r{audio_buffer_.Read(stream.format_, 5513, writer)};
      ASSERT_GE(r, 0);
      ASSERT_LE(r, 5513);

      ASSERT_TRUE(FLAC__stream_decoder_process_single(stream.decoder_));
      state = FLAC__stream_decoder_get_state(stream.decoder_);
      usleep(100);
    }
  }
  audio_buffer_.Drain(stream.format_, writer);

  EXPECT_EQ(150000, writer.total);
  EXPECT_TRUE(audio_buffer_.IsEmpty());
}

} // namespace
} // namespace plac
