// SPDX-License-Identifier: MIT

#include "stream.h"
#include <fstream>
#include <gtest/gtest.h>

namespace plac {
namespace {

class FlacDecodeTest : public ::testing::Test {
protected:
};

TEST_F(FlacDecodeTest, Play16Bps) {
    Stream stream{plac::AlsaAudioDevice::Output::file};

    bool first{true};
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
        if (first) {
            first = false;
            stream.device_.Init(stream.format_, ::plac::AlsaAudioDevice::LogLevel::verbose);
        }
        stream.Decode();
    }
    stream.device_.Drain();

    std::ifstream file("uln2-raw-S16_LE-44100-2.raw", std::ios::binary);
    ASSERT_TRUE(file.is_open());

    std::istreambuf_iterator<char> it(file);
    std::istreambuf_iterator<char> end;

    size_t total = 0;

    for (; it != end;) {
        ASSERT_EQ(total & 0xFF, static_cast<std::uint8_t>(*(it++))) << total;
        ASSERT_EQ((total >> 8) & 0xFF, static_cast<uint8_t>(*(it++))) << total;
        ASSERT_EQ((total >> 16) & 0xFF, static_cast<uint8_t>(*(it++))) << total;
        ASSERT_EQ(total & 0xFF, static_cast<uint8_t>(*(it++))) << total;
        ++total;
    }

    EXPECT_EQ(150000, total);
}

TEST_F(FlacDecodeTest, Play24Bps) {
  Stream stream{plac::AlsaAudioDevice::Output::file};

  bool first{true};
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
    if (first) {
        first = false;
        stream.device_.Init(stream.format_, ::plac::AlsaAudioDevice::LogLevel::verbose);
    }
    stream.Decode();
  }
  stream.device_.Drain();

  std::ifstream file("uln2-raw-S24_3LE-44100-2.raw", std::ios::binary);
  ASSERT_TRUE(file.is_open());

  std::istreambuf_iterator<char> it(file);
  std::istreambuf_iterator<char> end;

  size_t total = 0;

  for (; it != end;) {
      ASSERT_EQ((total >> 0) & 0xFF, static_cast<uint8_t>(*(it++))) << total;
      ASSERT_EQ((total >> 8) & 0xFF, static_cast<uint8_t>(*(it++))) << total;
      ASSERT_EQ((total >> 16) & 0xFF, static_cast<uint8_t>(*(it++))) << total;
      ASSERT_EQ((total >> 16) & 0xFF, static_cast<uint8_t>(*(it++))) << total;
      ASSERT_EQ((total >> 8) & 0xFF, static_cast<uint8_t>(*(it++))) << total;
      ASSERT_EQ((total >> 0) & 0xFF, static_cast<uint8_t>(*(it++))) << total;
      ++total;
  }

  EXPECT_EQ(150000, total);
}

} // namespace
} // namespace plac
