// SPDX-License-Identifier: MIT

#include "audio_buffer.h"
#include <benchmark/benchmark.h>
#include <vector>

namespace {

void Write16Bit(benchmark::State &state) {
  constexpr plac::AudioFormat format{16, 2, 96000};
  constexpr std::size_t size{57000};
  plac::AudioBuffer<AsBytes(format, size)> b;
  std::vector<int> left;
  std::vector<int> right;
  left.resize(size);
  right.resize(size);

  for (auto _ : state) {
    benchmark::DoNotOptimize(b.Write(format, left.data(), right.data(), size));
    benchmark::ClobberMemory();

    b.Drain(format, [](const plac::AudioFormat, u_char *, const size_t count) { return count; });
  }
}

void Write24Bit(benchmark::State &state) {
  constexpr plac::AudioFormat format{24, 2, 96000};
  constexpr std::size_t size{57000};
  plac::AudioBuffer<AsBytes(format, size)> b;
  std::vector<int> left;
  std::vector<int> right;
  left.resize(size);
  right.resize(size);

  for (auto _ : state) {
    benchmark::DoNotOptimize(b.Write(format, left.data(), right.data(), size));
    benchmark::ClobberMemory();

    b.Drain(format, [](const plac::AudioFormat, u_char *, const size_t count) { return count; });
  }
}

BENCHMARK(Write16Bit);
BENCHMARK(Write24Bit);

} // namespace

BENCHMARK_MAIN();
