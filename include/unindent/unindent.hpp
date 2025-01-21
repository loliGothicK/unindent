#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <concepts>
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
inline constexpr auto to_unindented = []<typename CharT, std::size_t N>(
                                          std::array<CharT, N> raw) consteval {
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

  auto fn = [min_indent](auto line) {
    std::basic_string_view<CharT> line_view{line.begin(), line.end()};
    if (line_view.size() < min_indent)
      return line_view;
    line_view.remove_prefix(min_indent);
    return line_view;
  };

  for (auto line : lines | views::transform(fn)) {
    for (auto c : line) {
      buffer[index++] = c;
    }
    buffer[index++] = '\n';
  }
  buffer[index - 1] = '\0';
  return buffer;
};

inline constexpr auto to_folded =
    []<typename CharT, std::size_t N>(std::array<CharT, N> raw) consteval {
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
    };
} // namespace details

// edited string (value type)
template <details::fixed_string Lit, auto Editor>
  requires requires {
    {
      Editor(Lit.s)
    } -> std::same_as<
        std::array<typename decltype(Lit)::char_type, decltype(Lit)::size>>;
  }
class edited {
public:
  // type members
  using char_type = decltype(Lit)::char_type;

private:
  static constexpr std::array<char_type, decltype(Lit)::size> value_ =
      Editor(Lit.s);

public:
  // member functions

  // #region comparison operators
  constexpr inline friend std::strong_ordering
  operator<=>(std::basic_string_view<char_type> lhs, edited) noexcept {
    return lhs <=> edited::value();
  }

  constexpr inline friend bool operator==(std::basic_string_view<char_type> lhs,
                                          edited) noexcept {
    return lhs <=> edited::value() == std::strong_ordering::equal;
  }

  constexpr inline friend bool operator<(std::basic_string_view<char_type> lhs,
                                         edited) noexcept {
    return lhs <=> edited::value() == std::strong_ordering::less;
  }

  constexpr inline friend bool operator>(std::basic_string_view<char_type> lhs,
                                         edited) noexcept {
    return lhs <=> edited::value() == std::strong_ordering::greater;
  }

  constexpr inline friend std::strong_ordering
  operator<=>(edited, std::basic_string_view<char_type> rhs) noexcept {
    return edited::value() <=> rhs;
  }

  constexpr inline friend bool
  operator==(edited, std::basic_string_view<char_type> rhs) noexcept {
    return edited::value() <=> rhs == std::strong_ordering::equal;
  }

  constexpr inline friend bool
  operator<(edited, std::basic_string_view<char_type> rhs) noexcept {
    return edited::value() <=> rhs == std::strong_ordering::less;
  }

  constexpr inline friend bool
  operator>(edited, std::basic_string_view<char_type> rhs) noexcept {
    return edited::value() <=> rhs == std::strong_ordering::greater;
  }
  // #endregion

  // iterator support
  constexpr auto begin() { return edited::value().begin(); }
  constexpr auto end() { return edited::value().end(); }
  constexpr auto cbegin() const { return edited::value().cbegin(); }
  constexpr auto cend() const { return edited::value().cend(); }
  constexpr auto rbegin() { return edited::value().rbegin(); }
  constexpr auto rend() { return edited::value().rend(); }
  constexpr auto crbegin() const { return edited::value().crbegin(); }
  constexpr auto crend() const { return edited::value().crend(); }

  // conversion operator
  constexpr operator std::basic_string_view<char_type>() const {
    return std::basic_string_view<char_type>(value_.data());
  }

  // static member function
  // access the value of the edited string
  static constexpr std::basic_string_view<char_type> value() {
    return std::basic_string_view<char_type>(value_.data());
  }
};

namespace details {
template <class> struct is_edited_strings : std::false_type {};
template <auto S, auto _>
struct is_edited_strings<edited<S, _>> : std::true_type {};

template <class T>
concept edited_strings = is_edited_strings<T>::value;
} // namespace details

template <details::edited_strings S1, details::edited_strings S2>
  requires std::same_as<typename S1::char_type, typename S2::char_type>
constexpr inline std::strong_ordering operator<=>(S1, S2) noexcept {
  return S1::value() <=> S2::value();
}

template <details::edited_strings S1, details::edited_strings S2>
  requires std::same_as<typename S1::char_type, typename S2::char_type>
constexpr inline bool operator==(S1, S2) noexcept {
  return S1::value() <=> S2::value() == std::strong_ordering::equal;
}

template <details::edited_strings S1, details::edited_strings S2>
  requires std::same_as<typename S1::char_type, typename S2::char_type>
constexpr inline bool operator<(S1, S2) noexcept {
  return S1::value() <=> S2::value() == std::strong_ordering::less;
}

template <details::edited_strings S1, details::edited_strings S2>
  requires std::same_as<typename S1::char_type, typename S2::char_type>
constexpr inline bool operator>(S1, S2) noexcept {
  return S1::value() <=> S2::value() == std::strong_ordering::greater;
}

inline std::ostream &operator<<(std::ostream &os,
                                details::edited_strings auto _) {
  return os << decltype(_)::value();
}
} // namespace mitama::unindent

namespace mitama::unindent::inline literals {
template <details::fixed_string S>
inline constexpr edited<S, details::to_unindented> operator""_i() {
  return {};
}

template <details::fixed_string S>
inline constexpr edited<S, details::to_folded> operator""_i1() {
  return {};
}
} // namespace mitama::unindent::inline literals
