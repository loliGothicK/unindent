#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <limits>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace mitama::unindent {
template <std::size_t N, class CharT> struct fixed_string {
  static constexpr std::size_t size = N;
  using char_type = CharT;

  constexpr fixed_string(const CharT (&s)[N])
      : fixed_string(s, std::make_index_sequence<N>{}) {}
  template <std::size_t... Indices>
  constexpr fixed_string(const CharT (&s)[N], std::index_sequence<Indices...>)
      : s{s[Indices]...} {}

  const std::array<CharT, N> s;
};

template <std::size_t N, class CharT>
std::ostream &operator<<(std::ostream &os, fixed_string<N, CharT> fs) {
  return os << fs.s;
}

// non-type template enabled static string class
//
// S: fixed_string<N, CharT>
template <fixed_string S> struct static_string {
  using char_type = typename decltype(S)::char_type;
  static constexpr const std::basic_string_view<char_type> value = {
      S.s, decltype(S)::size};

  template <auto T>
    requires std::same_as<char_type, typename static_string<T>::char_type>
  constexpr std::strong_ordering operator<=>(static_string<T>) const noexcept {
    return static_string::value <=> static_string<T>::value;
  }

  constexpr operator std::basic_string_view<char_type>() const { return value; }
};

template <auto S> std::ostream &operator<<(std::ostream &os, static_string<S>) {
  return os << static_string<S>::value;
}

template <fixed_string S> class unindented {
public:
  using char_type = decltype(S)::char_type;

private:
  static consteval std::array<char_type, decltype(S)::size>
  cvt(std::array<char_type, decltype(S)::size> raw) {
    namespace views = std::ranges::views;
    using namespace std::literals;
    using std::size_t;
    auto str = std::basic_string_view<char_type>(raw.data());

    while (str.ends_with(' '))
      str.remove_suffix(1);
    while (str.starts_with('\n'))
      str.remove_prefix(1);

    auto is_space = [](char_type c) { return c == ' '; };

    auto lines = str | views::split("\n"sv) | views::filter([](auto line) {
      return !line.empty();
    });

    auto min_indent = std::numeric_limits<size_t>::max();

    std::array<char_type, decltype(S)::size> buffer = {};
    size_t index = 0;

    for (auto line : lines) {
      auto iter = std::find_if_not(line.begin(), line.end(), is_space);
      min_indent = std::min({min_indent, static_cast<size_t>(std::distance(line.begin(), iter))});
    }

    if (min_indent == std::numeric_limits<size_t>::max() || min_indent == 0) {
      for (auto c : str) {
        buffer[index++] = c;
      }
      return buffer;
    }

    for (auto line : lines | views::transform([min_indent](auto line) {
                    std::basic_string_view<char_type> line_view{line.begin(),
                                                                line.end()};
                    line_view.remove_prefix(min_indent);
                    return line_view;
                  })) {
      for (auto c : line) {
        buffer[index++] = c;
      }
      buffer[index++] = '\n';
    }
    buffer[index - 1] = '\0';
    return buffer;
  }

private:
  static constexpr std::array<char_type, decltype(S)::size> raw_ = S.s;
  static constexpr std::array<char_type, decltype(S)::size> value_ = cvt(S.s);

public:
  template <auto T>
    requires std::same_as<char_type, typename unindented<T>::char_type>
  constexpr std::strong_ordering operator<=>(unindented<T>) const noexcept {
    return unindented::value_ <=> unindented<T>::value_;
  }

  constexpr operator std::basic_string_view<char_type>() const {
    return std::basic_string_view<char_type>(value_.data());
  }
  static constexpr std::basic_string_view<char_type> value() {
    return std::basic_string_view<char_type>(value_.data());
  }
};

template <fixed_string S>
std::ostream &operator<<(std::ostream &os, unindented<S>) {
  return os << unindented<S>::value();
}

template <fixed_string S> inline constexpr unindented<S> operator""_i() {
  return {};
}
} // namespace mitama::unindent
