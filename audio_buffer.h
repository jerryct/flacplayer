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
    const unsigned int frame_size{format.bits / 8 * format.channels};

    for (int i{0}; i < count; ++i) {
      if ((N - size_) < frame_size) {
        return i;
      }

      if (format.bits == 16) {
        data_[in_] = left[i] & 0xFF;
        data_[(in_ + 1) % N] = left[i] >> 8 & 0xFF;
        data_[(in_ + 2) % N] = right[i] & 0xFF;
        data_[(in_ + 3) % N] = right[i] >> 8 & 0xFF;
      } else if (format.bits == 24) {
        data_[in_] = left[i] & 0xFF;
        data_[(in_ + 1) % N] = left[i] >> 8 & 0xFF;
        data_[(in_ + 2) % N] = left[i] >> 16 & 0xFF;
        data_[(in_ + 3) % N] = right[i] & 0xFF;
        data_[(in_ + 4) % N] = right[i] >> 8 & 0xFF;
        data_[(in_ + 5) % N] = right[i] >> 16 & 0xFF;
      }

      in_ = (in_ + frame_size) % N;
      size_ += frame_size;
    }

    return count;
  }

  template <typename T> ssize_t Read(const AudioFormat format, const size_t count, T &&pipe) {
    if (AsFrames(format, size_) < count) {
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
