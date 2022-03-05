// SPDX-License-Identifier: MIT

#ifndef ALIGN_H
#define ALIGN_H

#include <cstddef>

namespace plac {

constexpr size_t AlignUp(unsigned N) {
  //  size_t t = ((unsigned)-1) << 5;
  //  size_t tm = (N - t - 1) & t;
  size_t t = (1 << 5) - 1;
  size_t tm = (N + t) & ~t;
  return tm;
}
constexpr size_t AlignDown(const size_t x) { return x & (-4); }

} // namespace plac

#endif // ALIGN_H
