#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <concepts>
#include <cstddef>
#include <format>
#include <functional>
#include <iostream>
#include <limits>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace mitama::unindent
{
// fixed_string (structural type)
// This is a class representing a string literal.
//
// template parameters:
// - `CharT`: a character type of the string literal.
// - `N`: The non-type template parameter, the size of the string literal.
//
// [Note 1: `fixed_string` is initialized with a string literals.
// In addition, due to CTAD (Class Template Argument Deduction), `CharT` and `N`
// is automatically deduced. [Example:
//   ```
//   constexpr fixed_string fs = "abc"; // fixed_string<char, 4>
//   ```
// — end example]
//
// ref:
// https://en.cppreference.com/w/cpp/language/class_template_argument_deduction
// — end note]
//
// [Note 2: Since C++20, `fixed_string` can be used as non-type template
// arguments with CTAD. [Example:
//   ```
//   template <fixed_string S>
//   struct foo {};
//
//   foo<"abc"> f; // foo<fixed_string<char, 4>{"abc"}>
//   ```
// — end example]
//
// ref: https://timsong-cpp.github.io/cppwp/n4861/temp.arg.nontype#1
// — end note]
//
// [Note 3: Since C++20, `fixed_string` can be used as a template parameter
// of a user-defined literals. [Example:
//   ```
//   template <fixed_string S>
//   inline constexpr auto operator""_fixed() {
//     return S;
//   }
//
//   "abc"_fixed; // fixed_string<char, 4>{"abc"}
//   ```
// — end example]
//
// ref: https://timsong-cpp.github.io/cppwp/n4861/lex.ext#5
// -- end Note]
template <class CharT, std::size_t N>
struct fixed_string
{
  static constexpr std::size_t size = N;
  using char_type = CharT;

  constexpr fixed_string(const CharT (&init)[N])
      : s{ [&]<std::size_t... Indices>(std::index_sequence<Indices...>) {
          return std::array{ init[Indices]... };
        }(std::make_index_sequence<N>{}) } {}

  const std::array<CharT, N> s;
};

template <class CharT, std::size_t N>
inline std::ostream&
operator<<(std::ostream& os, fixed_string<CharT, N> fs) {
  return os << fs.s;
}

namespace details
{
  // editor function for unindented string
  inline constexpr auto to_unindented =
      []<typename CharT, std::size_t N>(std::array<CharT, N> raw) consteval {
        namespace views = ::std::ranges::views;
        namespace ranges = ::std::ranges;
        using namespace std::literals;

        auto str = std::basic_string_view<CharT>(raw.data());

        while (str.starts_with('\n'))
          str.remove_prefix(1);
        while (str.ends_with(' ') or str.ends_with('\n'))
          str.remove_suffix(1);

        auto lines = str | views::split("\n"sv);

        // clang-format off
      auto indents
        = lines
        | views::filter([](auto line) { return !line.empty(); })
        | views::transform([](auto line) {
            return ranges::distance(
              line | views::take_while([](CharT c) { return c == ' '; }));
          });
        // clang-format on

        auto fn = [n = static_cast<std::size_t>(ranges::min(indents))](auto line
                  ) { return line.size() >= n ? line | views::drop(n) : line; };

        std::array<CharT, N> buffer = {};
        std::size_t index = 0;

        for (auto line : lines | views::transform(fn)) {
          for (auto c : line) {
            buffer[index++] = c;
          }
          buffer[index++] = '\n';
        }
        buffer[index - 1] = '\0';
        return buffer;
      };

  // editor function for folded string
  inline constexpr auto to_folded =
      []<typename CharT, std::size_t N>(std::array<CharT, N> raw) consteval {
        namespace views = std::ranges::views;

        std::array<CharT, N> buffer = {};
        size_t index = 0;
        size_t returns = 0;

        // First, adjust the indentation of the string.
        // Second, replace multiple returns with a single return
        // and replace a single return with a space.
        for (auto c : to_unindented(raw)) {
          if (c == '\n') {
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

// This is a class representing a string that has been edited by `Editor`.
//
// template parameters:
// - `Lit`: The non-type template parameter, a `fixed_string` representing the
// original string.
// - `Editor`: The non-type template parameter, a function object specifying how
// to edit the original string.
//
// [Note: `Editor` is a CPO (Customization Point Object) that is a function
// object. The function object must be a immidiate function object that
// satisfies the following requirements:
//
//  ```
//  requires {
//    { std::invoke(Editor, Lit.s) } -> std::convertible_to<decltype(Lit.s)>;
//  }
//  ```
//
//  `Lit.s` is a `const std::array` of `CharT` that represents the original
//  string.
//  Note that the return value must be null terminated in order to pass the
//  `std::array::data()` to `basic_string_view::basic_string_view(const CharT*)`
//  when converting `edited_string` to `basic_string_view` (for more detail,
//  see:
//  https://en.cppreference.com/w/cpp/string/basic_string_view/basic_string_view).
//
//  To make your own literal operator, you can use `edited_string` as follows:
//  [Example:
//    ```
//    template <mitama::unindent::fixed_string S>
//    inline constexpr mitama::unindent::edited_string<S, {Your CPO}>
//    operator""_xxx() {
//      return {};
//    }
//    ```
//  - end example]
//
//  See the `fixed_string` documentation for detailed principles.
// — end note]
template <fixed_string Lit, auto Editor>
  requires requires {
    { Editor(Lit.s) } -> std::convertible_to<decltype(Lit.s)>;
  }
class edited_string
{
  static constexpr decltype(Lit.s) value_ = Editor(Lit.s);

public:
  // type members
  using char_type = decltype(Lit)::char_type;

public:
  // member functions

  // #region comparison operators
  constexpr inline friend std::strong_ordering
  operator<=>(std::basic_string_view<char_type> lhs, const edited_string&) noexcept {
    return lhs <=> edited_string::value();
  }

  constexpr inline friend bool
  operator!=(std::basic_string_view<char_type> lhs, const edited_string&) noexcept {
    return lhs <=> edited_string::value() != std::strong_ordering::equal;
  }

  constexpr inline friend bool
  operator==(std::basic_string_view<char_type> lhs, const edited_string&) noexcept {
    return lhs <=> edited_string::value() == std::strong_ordering::equal;
  }

  constexpr inline friend bool
  operator<(std::basic_string_view<char_type> lhs, const edited_string&) noexcept {
    return lhs <=> edited_string::value() == std::strong_ordering::less;
  }

  constexpr inline friend bool
  operator>(std::basic_string_view<char_type> lhs, const edited_string&) noexcept {
    return lhs <=> edited_string::value() == std::strong_ordering::greater;
  }

  constexpr inline friend std::strong_ordering operator<=>(
      const edited_string&, std::basic_string_view<char_type> rhs
  ) noexcept {
    return edited_string::value() <=> rhs;
  }

  constexpr inline friend bool operator!=(
      const edited_string&, std::basic_string_view<char_type> rhs
  ) noexcept {
    return edited_string::value() <=> rhs != std::strong_ordering::equal;
  }

  constexpr inline friend bool operator==(
      const edited_string&, std::basic_string_view<char_type> rhs
  ) noexcept {
    return edited_string::value() <=> rhs == std::strong_ordering::equal;
  }

  constexpr inline friend bool operator<(
      const edited_string&, std::basic_string_view<char_type> rhs
  ) noexcept {
    return edited_string::value() <=> rhs == std::strong_ordering::less;
  }

  constexpr inline friend bool operator>(
      const edited_string&, std::basic_string_view<char_type> rhs
  ) noexcept {
    return edited_string::value() <=> rhs == std::strong_ordering::greater;
  }
  // #endregion

  // iterator support
  constexpr auto begin() const noexcept {
    return edited_string::value().begin();
  }
  constexpr auto end() const noexcept {
    return edited_string::value().end();
  }
  constexpr auto cbegin() const noexcept {
    return edited_string::value().cbegin();
  }
  constexpr auto cend() const noexcept {
    return edited_string::value().cend();
  }
  constexpr auto rbegin() const noexcept {
    return edited_string::value().rbegin();
  }
  constexpr auto rend() const noexcept {
    return edited_string::value().rend();
  }
  constexpr auto crbegin() const noexcept {
    return edited_string::value().crbegin();
  }
  constexpr auto crend() const noexcept {
    return edited_string::value().crend();
  }

  // static member function
  // access the value of the edited_string string
  static constexpr std::basic_string_view<char_type> value() noexcept {
    return std::basic_string_view<char_type>(value_.data());
  }

  // format with args
  template <class... Args>
  auto format(Args&&... args) const {
    return std::format(value(), std::forward<Args>(args)...);
  }

  constexpr std::basic_string_view<char_type> to_str() const {
    return value();
  }
};

namespace details
{
  template <class>
  struct is_edited_strings : std::false_type
  {};
  template <auto S, auto _>
  struct is_edited_strings<edited_string<S, _>> : std::true_type
  {};

  template <class T>
  concept edited_strings = is_edited_strings<std::remove_cvref_t<T>>::value;
} // namespace details

template <details::edited_strings S1, details::edited_strings S2>
  requires std::same_as<
      typename std::remove_cvref_t<S1>::char_type,
      typename std::remove_cvref_t<S2>::char_type>
constexpr inline std::strong_ordering
operator<=>(S1&&, S2&&) noexcept {
  return std::remove_cvref_t<S1>::value() <=> std::remove_cvref_t<S2>::value();
}

template <details::edited_strings S1, details::edited_strings S2>
  requires std::same_as<
      typename std::remove_cvref_t<S1>::char_type,
      typename std::remove_cvref_t<S2>::char_type>
constexpr inline bool
operator!=(S1&&, S2&&) noexcept {
  return std::remove_cvref_t<S1>::value() != std::remove_cvref_t<S2>::value();
}

template <details::edited_strings S1, details::edited_strings S2>
  requires std::same_as<
      typename std::remove_cvref_t<S1>::char_type,
      typename std::remove_cvref_t<S2>::char_type>
constexpr inline bool
operator==(S1&&, S2&&) noexcept {
  return std::remove_cvref_t<S1>::value() == std::remove_cvref_t<S2>::value();
}

template <details::edited_strings S1, details::edited_strings S2>
  requires std::same_as<
      typename std::remove_cvref_t<S1>::char_type,
      typename std::remove_cvref_t<S2>::char_type>
constexpr inline bool
operator<(S1&&, S2&&) noexcept {
  return std::remove_cvref_t<S1>::value() < std::remove_cvref_t<S2>::value();
}

template <details::edited_strings S1, details::edited_strings S2>
  requires std::same_as<
      typename std::remove_cvref_t<S1>::char_type,
      typename std::remove_cvref_t<S2>::char_type>
constexpr inline bool
operator>(S1&&, S2&&) noexcept {
  return std::remove_cvref_t<S1>::value() > std::remove_cvref_t<S2>::value();
}

inline std::ostream&
operator<<(std::ostream& os, details::edited_strings auto&& _) {
  return os << std::remove_cvref_t<decltype(_)>::value();
}
} // namespace mitama::unindent

namespace mitama::unindent::inline literals
{
// indent-adjunsted multiline string literal
// This literal operator returns an indent-adjunsted string.
//
// Usage:
// ```cpp
//  constexpr std::string_view unindented_str = R"(
//    def foo():
//      print("Hello")
//      print("World")
//  )"_i;
//
//  std::cout << unindented_str;
//  // Output:
//  // def foo():
//  //   print("Hello")
//  //   print("World")
// ```
template <fixed_string S>
inline constexpr edited_string<S, details::to_unindented>
operator""_i() {
  return {};
}

// folded multiline string literal
// This literal operator returns a folded string.
//
// Usage:
// ```cpp
//  constexpr std::string_view folded_str = R"(
//    cmake
//    -DCMAKE_BUILD_TYPE=Release
//    -B build
//    -S .
//  )"_i1;
//
//  std::cout << folded_str;
//  // Output:
//  // cmake -DCMAKE_BUILD_TYPE=Release -B build -S .
// ```
template <fixed_string S>
inline constexpr edited_string<S, details::to_folded>
operator""_i1() {
  return {};
}
} // namespace mitama::unindent::inline literals
