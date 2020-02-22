// SPDX-License-Identifier: MIT

#ifndef BIT_CAST_H
#define BIT_CAST_H

#include <cstring>
#include <type_traits>

namespace plac {

template <typename To, typename From> To BitCast(const From &src) noexcept {
  static_assert((sizeof(To) == sizeof(From)), "");
  static_assert(std::is_trivially_copyable<From>::value, "");
  static_assert(std::is_trivial<To>::value, "");

  To dst;
  std::memcpy(&dst, &src, sizeof(To));
  return dst;
}

} // namespace plac

#endif
