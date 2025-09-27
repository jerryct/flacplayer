// SPDX-License-Identifier: MIT

#ifndef CONDITIONS_H
#define CONDITIONS_H

#include <cstdlib>
#include <fmt/format.h>

namespace plac {

class Printer {
public:
  constexpr Printer(const fmt::string_view file, const int line, const fmt::string_view func)
      : file_{file}, func_{func}, line_{line} {}

  template <typename... Ts> constexpr void operator()(const fmt::format_string<Ts...> s, Ts &&... v) const {
    fmt::print(stderr, "flacplayer [{}:{} ({})] ", file_, line_, func_);
    fmt::print(stderr, s, std::forward<Ts>(v)...);
    fmt::print(stderr, "\n");
  }

private:
  fmt::string_view file_;
  fmt::string_view func_;
  int line_;
};

class Conditions {
public:
  constexpr Conditions(const fmt::string_view file, const int line, const fmt::string_view func)
      : p_{file, line, func} {}

  template <typename... Ts>
  constexpr void operator()(const bool condition, const fmt::format_string<Ts...> s, Ts &&... v) const {
    if (!condition) {
      p_(s, std::forward<Ts>(v)...);
      std::abort();
    }
  }

private:
  Printer p_;
};

} // namespace plac

#define EXPECTS (::plac::Conditions{__FILE__, __LINE__, __func__})
#define ENSURES (::plac::Conditions{__FILE__, __LINE__, __func__})
#define LOG_ERROR (::plac::Printer{__FILE__, __LINE__, __func__})

#endif
