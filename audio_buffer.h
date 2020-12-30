// SPDX-License-Identifier: MIT

#ifndef AUDIO_BUFFER_H
#define AUDIO_BUFFER_H

#include "audio_format.h"
#include "conditions.h"
#include <atomic>
#include <cstddef>
#include <sys/types.h>

namespace plac {

template <unsigned int N> class AudioBuffer {
public:
  float GetFillLevel() const { return static_cast<float>(size_) / static_cast<float>(N); }
  bool IsEmpty() const { return size_ == 0; }

  ssize_t Write(const AudioFormat format, const int *const left, const int *const right, const size_t count) {
    const size_t result{std::min(count, AsFrames(format, N - size_))};

    if (format.bits == 16) {
      for (int i{0}; i < result; ++i) {
        std::uint32_t interleaved{static_cast<std::uint32_t>(right[i])};
        interleaved <<= 16;
        interleaved |= static_cast<std::uint32_t>(left[i]) & 0xFFFF;
        std::memcpy(&data_[in_], &interleaved, 4);

        in_ += 4;
        if (in_ == N) {
          in_ = 0;
        }
      }
    } else if (format.bits == 24) {
      for (int i{0}; i < result; ++i) {
        std::uint64_t interleaved{static_cast<std::uint32_t>(right[i])};
        interleaved <<= 24;
        interleaved |= static_cast<std::uint64_t>(left[i]) & 0xFFFFFF;
        std::memcpy(&data_[in_], &interleaved, 6);

        in_ += 6;
        if (in_ == N) {
          in_ = 0;
        }
      }
    }
    size_ += AsBytes(format, result);

    return result;
  }

  template <typename T> ssize_t Read(const AudioFormat format, const size_t count, T &&pipe) {
    if (size_ < AsBytes(format, count)) {
      return 0;
    }

    ssize_t result{0};

    if ((out_ + AsBytes(format, count)) <= N) {
      result = std::forward<T>(pipe)(format, &data_[out_], count);
    } else {
      const size_t rest{AsFrames(format, (out_ + AsBytes(format, count)) % N)};
      result = std::forward<T>(pipe)(format, &data_[out_], (count - rest));
    }

    if (result >= 0) {
      out_ = (out_ + AsBytes(format, result)) % N;
      size_ -= AsBytes(format, result);
    }

    return result;
  }

  template <typename F> ssize_t Drain(const AudioFormat format, F &&Write) {
    size_t count{AsFrames(format, size_)};
    while (count != 0) {
      const ssize_t n{Read(format, count, std::forward<F>(Write))};
      if (n < 0) {
        LOG_ERROR("dropping {} frames", count);
        return n;
      }
      count -= static_cast<std::size_t>(n);
    }
    return 0;
  }

private:
  static_assert((N % 4) == 0, "16bps with channels does not fit evenly into data_.");
  static_assert((N % 6) == 0, "24bps with channels does not fit evenly into data_.");
  unsigned char data_[N];
  std::atomic<unsigned int> size_{0};
  unsigned int in_{0};
  unsigned int out_{0};
};

} // namespace plac

#endif
