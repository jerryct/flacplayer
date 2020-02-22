// SPDX-License-Identifier: MIT

#ifndef AUDIO_FORMAT_H
#define AUDIO_FORMAT_H

#include <cstddef>

namespace plac {

struct AudioFormat {
  unsigned int bits;
  unsigned int channels;
  unsigned int rate;
};

constexpr bool operator==(const AudioFormat lhs, const AudioFormat rhs) {
  return (lhs.bits == rhs.bits) && (lhs.channels == rhs.channels) && (lhs.rate == rhs.rate);
}

constexpr bool operator!=(const AudioFormat lhs, const AudioFormat rhs) { return !(lhs == rhs); }

constexpr size_t AsBytes(AudioFormat format, size_t frames) { return frames * format.bits * format.channels / 8; }
constexpr size_t AsFrames(AudioFormat format, size_t bytes) { return bytes * 8 / format.bits / format.channels; }

} // namespace plac

#endif
