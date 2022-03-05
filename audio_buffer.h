// SPDX-License-Identifier: MIT

#ifndef AUDIO_BUFFER_H
#define AUDIO_BUFFER_H

#include "align.h"
#include "as_const.h"
#include "audio_format.h"
#include "conditions.h"
#include <atomic>
#include <cstddef>
#include <nmmintrin.h>
#include <sys/types.h>

namespace plac {

template <unsigned int N> class AudioBuffer {
public:
  float GetFillLevel() const { return static_cast<float>(size_) / static_cast<float>(N); }
  bool IsEmpty() const { return size_ == 0; }

  ssize_t Write(const AudioFormat format, const int *const left, const int *const right, const size_t count) {
    const size_t result{AlignDown(std::min(count, AsFrames(format, N - size_)))};

    if (format.bits == 16) {
      for (int i{0}; i < result; i += 4) {
        const auto l = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&left[i]));
        const auto r = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&right[i]));
        const auto inter0 = _mm_unpacklo_epi32(l, r);
        const auto inter1 = _mm_unpackhi_epi32(l, r);

        const auto p16 = _mm_packs_epi32(inter0, inter1);

        _mm_store_si128(reinterpret_cast<__m128i *>(&data_[in_]), p16);

        in_ += 16;
        if (in_ == N) {
          in_ = 0;
        }
      }
    } else if (format.bits == 24) {
      for (int i{0}; i < result; i += 4) {
        const auto l = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&left[i]));
        const auto r = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&right[i]));
        const auto inter0 = _mm_unpacklo_epi32(l, r);
        const auto inter1 = _mm_unpackhi_epi32(l, r);

        const auto mask0 = _mm_set_epi8(-1, -1, -1, -1, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0);
        const auto shuf0 = _mm_shuffle_epi8(inter0, mask0);
        const auto mask1 = _mm_set_epi8(4, 2, 1, 0, -1, -1, -1, -1, 14, 13, 12, 10, 9, 8, 6, 5);
        const auto shuf1 = _mm_shuffle_epi8(inter1, mask1);
        const auto blend_mask = _mm_set_epi8(0xFF, 0xFF, 0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        const auto blend = _mm_blendv_epi8(shuf0, shuf1, blend_mask);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(&data_[in_]), blend);
        _mm_storeu_si64(reinterpret_cast<__m128i *>(&data_[in_ + 16]), shuf1);

        in_ += 24;
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
      result = std::forward<T>(pipe)(format, &AsConst(*this).data_[out_], count);
    } else {
      const size_t rest{AsFrames(format, (out_ + AsBytes(format, count)) % N)};
      result = std::forward<T>(pipe)(format, &AsConst(*this).data_[out_], (count - rest));
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
  static_assert((N % (4 * 4)) == 0, "at least 4x16bps must fit evenly into data_.");
  static_assert((N % (4 * 6)) == 0, "at least 4x24bps must fit evenly into data_.");

  alignas(64) std::atomic<unsigned int> size_{0};
  alignas(64) unsigned char data_[N];
  unsigned int in_{0};
  unsigned int out_{0};
};

} // namespace plac

#endif
