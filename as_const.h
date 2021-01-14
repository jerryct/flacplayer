// SPDX-License-Identifier: MIT

#ifndef AS_CONST_H
#define AS_CONST_H

#include <type_traits>

namespace plac {

template <typename T> constexpr std::add_const_t<T> &AsConst(T &t) noexcept { return t; }
template <typename T> void AsConst(const T &&) = delete;

} // namespace plac

#endif
