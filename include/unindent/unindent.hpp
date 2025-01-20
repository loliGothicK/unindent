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

namespace details {
// fixed_string (structural type)
template <class CharT, std::size_t N> struct fixed_string {
  static constexpr std::size_t size = N;
  using char_type = CharT;

  constexpr fixed_string(const CharT (&s)[N])
      : fixed_string(s, std::make_index_sequence<N>{}) {}
  template <std::size_t... Indices>
  constexpr fixed_string(const CharT (&s)[N], std::index_sequence<Indices...>)
      : s{s[Indices]...} {}

  const std::array<CharT, N> s;
};

template <class CharT, std::size_t N>
inline std::ostream &operator<<(std::ostream &os, fixed_string<CharT, N> fs) {
  return os << fs.s;
}
} // namespace details

namespace details {
template <class CharT, std::size_t N>
consteval auto to_unindented(std::array<CharT, N> raw) {
  namespace views = std::ranges::views;
  using namespace std::literals;
  using std::size_t;
  auto str = std::basic_string_view<CharT>(raw.data());

  while (str.ends_with(' '))
    str.remove_suffix(1);
  while (str.ends_with('\n'))
    str.remove_suffix(1);
  while (str.starts_with('\n'))
    str.remove_prefix(1);

  auto is_space = [](CharT c) { return c == ' '; };

  auto lines = str | views::split("\n"sv);
  auto lines_without_empty =
      str | views::split("\n"sv) |
      views::filter([](auto line) { return !line.empty(); });

  auto min_indent = std::numeric_limits<size_t>::max();

  std::array<CharT, N> buffer = {};
  size_t index = 0;

  for (auto line : lines_without_empty) {
    auto iter = std::find_if_not(line.begin(), line.end(), is_space);
    min_indent = std::min(
        {min_indent, static_cast<size_t>(std::distance(line.begin(), iter))});
  }

  if (min_indent == std::numeric_limits<size_t>::max() || min_indent == 0) {
    for (auto c : str) {
      buffer[index++] = c;
    }
    return buffer;
  }

  for (auto line : lines | views::transform([min_indent](auto line) {
                     std::basic_string_view<CharT> line_view{line.begin(),
                                                             line.end()};
                     if (line_view.size() < min_indent)
                       return line_view;
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

template <class CharT, std::size_t N>
consteval auto to_folded(std::array<CharT, N> raw) {
  namespace views = std::ranges::views;

  auto is_return = [](CharT c) { return c == '\n'; };

  std::array<CharT, N> buffer = {};
  size_t index = 0;
  size_t returns = 0;

  for (auto c : to_unindented(raw)) {
    if (is_return(c)) {
      returns++;
    } else {
      if (returns > 1) {
        buffer[index++] = '\n';
      } else if (returns == 1) {
        buffer[index++] = ' ';
      }
      buffer[index++] = c;
      returns = 0;
    }
  }

  return buffer;
}
} // namespace details

// unindented string (value type)
template <details::fixed_string Lit> class unindented {
public:
  using char_type = decltype(Lit)::char_type;

private:
  static constexpr std::array<char_type, decltype(Lit)::size> value_ =
      details::to_unindented(Lit.s);

public:
  template <auto S>
    requires std::same_as<char_type, typename unindented<S>::char_type>
  constexpr std::strong_ordering operator<=>(unindented<S>) const noexcept {
    return unindented::value_ <=> unindented<S>::value_;
  }

  constexpr operator std::basic_string_view<char_type>() const {
    return std::basic_string_view<char_type>(value_.data());
  }
  static constexpr std::basic_string_view<char_type> value() {
    return std::basic_string_view<char_type>(value_.data());
  }
};

template <details::fixed_string S>
inline std::ostream &operator<<(std::ostream &os, unindented<S>) {
  return os << unindented<S>::value();
}

// folded string (value type)
template <details::fixed_string Lit> class folded {
public:
  using char_type = decltype(Lit)::char_type;

private:
  static constexpr std::array<char_type, decltype(Lit)::size> value_ =
      details::to_folded(Lit.s);

public:
  template <auto S>
    requires std::same_as<char_type, typename folded<S>::char_type>
  constexpr std::strong_ordering operator<=>(folded<S>) const noexcept {
    return folded::value_ <=> folded<S>::value_;
  }

  constexpr operator std::basic_string_view<char_type>() const {
    return std::basic_string_view<char_type>(value_.data());
  }
  static constexpr std::basic_string_view<char_type> value() {
    return std::basic_string_view<char_type>(value_.data());
  }
};

template <details::fixed_string S>
inline std::ostream &operator<<(std::ostream &os, folded<S>) {
  return os << folded<S>::value();
}

} // namespace mitama::unindent

namespace mitama::unindent::inline literals {

template <details::fixed_string S>
inline constexpr unindented<S> operator""_i() {
  return {};
}

template <details::fixed_string S> inline constexpr folded<S> operator""_i1() {
  return {};
}

} // namespace mitama::unindent::inline literals
