// SPDX-License-Identifier: MIT

#ifndef CONDITIONS_H
#define CONDITIONS_H

#include <cstdlib>
#include <format>
#include <iostream>
#include <string_view>

namespace plac {

class Printer {
public:
  Printer(const std::string_view file, const int line, const std::string_view func)
      : file_{file}, func_{func}, line_{line} {}

  template <typename... Ts> void operator()(const std::format_string<Ts...> s, Ts &&... v) const {
    std::cerr << std::format("flacplayer [{}:{} ({})] ", file_, line_, func_);
    std::cerr << std::format(s, std::forward<Ts>(v)...);
    std::cerr << std::format("\n");
  }

private:
  std::string_view file_;
  std::string_view func_;
  int line_;
};

class Conditions {
public:
  Conditions(const std::string_view file, const int line, const std::string_view func)
      : p_{file, line, func} {}

  template <typename... Ts>
  void operator()(const bool condition, const std::format_string<Ts...> s, Ts &&... v) const {
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
