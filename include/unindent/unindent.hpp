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
// This is a structural type representing a fixed string.
//
// template parameters:
// - `CharT`: a character type of the string literal.
// - `N`: The non-type template parameter, the size of the string literal.
//
// [Note 1: `basic_fixed_string` is initialized with a string literals.
// In addition, due to CTAD (Class Template Argument Deduction), `CharT` and `N`
// is automatically deduced. [Example:
//   ```
//   constexpr basic_fixed_string fs = "abc"; // basic_fixed_string<char, 4>
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
//   template <basic_fixed_string S>
//   struct foo {};
//
//   foo<"abc"> f; // foo<basic_fixed_string<char, 4>{"abc"}>
//   ```
// — end example]
//
// ref: https://timsong-cpp.github.io/cppwp/n4861/temp.arg.nontype#1
// — end note]
//
// [Note 3: Since C++20, structural type can be used as a template parameter
// of a user-defined literals. [Example:
//   ```
//   template <basic_fixed_string S>
//   inline constexpr auto operator""_fixed() {
//     return S;
//   }
//
//   "abc"_fixed; // operator""_fixed<"abc">()
//   ```
// — end example]
//
// ref: https://timsong-cpp.github.io/cppwp/n4861/lex.ext#5
// -- end Note]
template <class CharT, std::size_t N>
struct basic_fixed_string
{
  static constexpr std::size_t size = N;
  using char_type = CharT;

  consteval basic_fixed_string(const CharT (&init)[N + 1])
      : data{ [&]<std::size_t... Indices>(std::index_sequence<Indices...>) {
          return std::array{ init[Indices]... };
        }(std::make_index_sequence<N + 1>{}) } {}

  auto operator<=>(const basic_fixed_string&) const = default;

  [[nodiscard]] constexpr auto to_str() const {
    return std::basic_string_view<CharT>(data.data());
  }

  const std::array<CharT, N + 1> data;
};

// deduction guide
template <typename CharT, std::size_t N>
basic_fixed_string(const CharT (&)[N]) -> basic_fixed_string<CharT, N - 1>;

// alias template
template <std::size_t N>
using fixed_string = basic_fixed_string<char, N>;

template <class CharT, std::size_t N>
inline std::ostream&
operator<<(std::ostream& os, basic_fixed_string<CharT, N> fs) {
  return os << fs.data;
}

namespace details
{
  // editor function for unindented string
  inline constexpr auto to_unindented =
      []<typename CharT, std::size_t N>(std::array<CharT, N> raw) consteval {
        namespace views = ::std::ranges::views;
        using namespace std::literals;

        auto str = std::basic_string_view<CharT>(raw.data());

        // strip leading and trailing returns
        while (str.starts_with('\n'))
          str.remove_prefix(1);
        while (str.ends_with(' ') or str.ends_with('\n'))
          str.remove_suffix(1);

        auto lines = str | views::split("\n"sv);

        // clang-format off
        // aggregates indent sizes (except empty lines)
        auto indents
          = lines
          | views::filter([](auto line) { return !line.empty(); })
          | views::transform([](auto line) {
              return std::ranges::distance(
                line | views::take_while([](CharT c) { return c == ' '; }));
            });
        // clang-format on

        std::size_t min = std::ranges::min(indents);

        auto remove_indent = [min](auto line) {
          return line.size() >= min ? line | views::drop(min) : line;
        };

        std::array<CharT, N> buffer = {};
        std::size_t index = 0;

        for (auto line : lines | views::transform(remove_indent)) {
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

        // Replace multiple returns with a single return
        // and replace a single return with a space.
        auto flush = [&] {
          if (returns > 1) {
            buffer[index++] = '\n';
          } else if (returns == 1) {
            buffer[index++] = ' ';
          }
          returns = 0; // reset here
        };

        for (auto c : to_unindented(raw)) {
          if (c == '\n') {
            returns++;
          } else {
            flush();
            buffer[index++] = c;
          }
        }

        return buffer;
      };
} // namespace details

// This is a class for static storage of result of editing the original string.
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
//    { Editor(Lit.data) } -> std::convertible_to<decltype(Lit.data)>;
//  }
//  ```
//
//  `Lit.data` is a `const std::array` of `CharT` that represents the original
//  string.
//  Note that the return value must be null terminated in order to pass the
//  `std::array::data()` to `basic_string_view::basic_string_view(const CharT*)`
//  when converting `edited_string` to `basic_string_view` (for more detail,
//  see: https://timsong-cpp.github.io/cppwp/n4861/string.view.cons#2).
//
//  To make your own literal operator, you can use `edited_string` as follows:
//  [Example:
//    ```
//    template <mitama::unindent::basic_fixed_string S>
//    inline consteval auto operator""_xxx() {
//      return mitama::unindent::edited_string<S, {Your CPO}>{};
//    }
//    ```
//  - end example]
//
//  See the `basic_fixed_string` documentation for detailed principles.
// — end note]
template <basic_fixed_string Lit, auto Editor>
  requires requires {
    { Editor(Lit.data) } -> std::convertible_to<decltype(Lit.data)>;
  }
class [[nodiscard]] edited_string final
{
  static constexpr decltype(Lit.data) value_ = Editor(Lit.data);
  using Self = edited_string;

public:
  // type members
  using char_type = decltype(Lit)::char_type;

  // #region comparison operators
  constexpr inline friend auto
  operator<=>(std::basic_string_view<char_type> lhs, const Self&) noexcept {
    return lhs <=> Self::value();
  }

  constexpr inline friend bool
  operator!=(std::basic_string_view<char_type> lhs, const Self&) noexcept {
    return lhs != Self::value();
  }

  constexpr inline friend bool
  operator==(std::basic_string_view<char_type> lhs, const Self&) noexcept {
    return lhs == Self::value();
  }

  constexpr inline friend bool
  operator<(std::basic_string_view<char_type> lhs, const Self&) noexcept {
    return lhs < Self::value();
  }

  constexpr inline friend bool
  operator>(std::basic_string_view<char_type> lhs, const Self&) noexcept {
    return lhs > Self::value();
  }

  constexpr inline friend auto
  operator<=>(const Self&, std::basic_string_view<char_type> rhs) noexcept {
    return Self::value() <=> rhs;
  }

  constexpr inline friend bool
  operator!=(const Self&, std::basic_string_view<char_type> rhs) noexcept {
    return Self::value() != rhs;
  }

  constexpr inline friend bool
  operator==(const Self&, std::basic_string_view<char_type> rhs) noexcept {
    return Self::value() == rhs;
  }

  constexpr inline friend bool
  operator<(const Self&, std::basic_string_view<char_type> rhs) noexcept {
    return Self::value() < rhs;
  }

  constexpr inline friend bool
  operator>(const Self&, std::basic_string_view<char_type> rhs) noexcept {
    return Self::value() > rhs;
  }
  // #endregion

  // iterator support
  constexpr auto begin() const noexcept {
    return Self::value().begin();
  }
  constexpr auto end() const noexcept {
    return Self::value().end();
  }
  constexpr auto cbegin() const noexcept {
    return Self::value().cbegin();
  }
  constexpr auto cend() const noexcept {
    return Self::value().cend();
  }
  constexpr auto rbegin() const noexcept {
    return Self::value().rbegin();
  }
  constexpr auto rend() const noexcept {
    return Self::value().rend();
  }
  constexpr auto crbegin() const noexcept {
    return Self::value().crbegin();
  }
  constexpr auto crend() const noexcept {
    return Self::value().crend();
  }

  // static member function
  // access the value of the edited_string string
  static constexpr std::basic_string_view<char_type> value() noexcept {
    return std::basic_string_view<char_type>(value_.data());
  }

  // Returns formatted string with `std::format`
  //
  // `s.format(args...)` is same as `std::format(s.to_str(), args...)`.
  //
  // Example:
  // ```cpp
  //  constexpr auto fmt = R"(
  //    def foo():
  //      print("Hello")
  //      print("{}")
  //  )"_i;
  //
  //  std::cout << fmt.format("World");
  //  // Output:
  //  // def foo():
  //  //   print("Hello")
  //  //   print("World")
  // ```
  auto format(auto&&... args) const {
    return std::format(value(), std::forward<decltype(args)>(args)...);
  }

  // Returns basic_string_view<char_type> of the edited string
  //
  // Example:
  // ```cpp
  //  constexpr std::string_view str = R"(
  //    def foo():
  //      print("Hello")
  //      print("World")
  //  )"_i.to_str();
  //```
  [[nodiscard]] constexpr std::basic_string_view<char_type> to_str() const {
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
constexpr inline auto
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

template <basic_fixed_string Lit>
inline constexpr auto unindented =
    edited_string<Lit, details::to_unindented>{}; // unindented string

template <basic_fixed_string Lit>
inline constexpr auto folded =
    edited_string<Lit, details::to_folded>{}; // folded string

} // namespace mitama::unindent

namespace mitama::unindent::inline literals
{
// indent-adjunsted multiline string literal
// This literal operator returns an indent-adjunsted string.
//
// Example:
// ```cpp
//  constexpr std::string_view unindented_str = R"(
//    def foo():
//      print("Hello")
//      print("World")
//  )"_iv;
//
//  std::cout << unindented_str;
//  // Output:
//  // def foo():
//  //   print("Hello")
//  //   print("World")
// ```
template <basic_fixed_string S>
inline consteval auto
operator""_iv() {
  return unindented<S>.to_str();
}

// indent-adjunsted multiline string literal
// This literal operator returns an indent-adjunsted string.
//
// Example:
// ```cpp
//  constexpr auto unindented_str = R"(
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
template <basic_fixed_string S>
inline consteval auto
operator""_i() {
  return unindented<S>;
}

// folded multiline string literal
// This literal operator returns a folded string.
//
// Example:
// ```cpp
//  constexpr std::string_view folded_str = R"(
//    cmake
//    -DCMAKE_BUILD_TYPE=Release
//    -B build
//    -S .
//  )"_i1v;
//
//  std::cout << folded_str;
//  // Output:
//  // cmake -DCMAKE_BUILD_TYPE=Release -B build -S .
// ```
template <basic_fixed_string S>
inline consteval auto
operator""_i1v() {
  return folded<S>.to_str();
}

// folded multiline string literal
// This literal operator returns a folded string.
//
// Example:
// ```cpp
//  constexpr auto folded_str = R"(
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
template <basic_fixed_string S>
inline consteval auto
operator""_i1() {
  return folded<S>;
}
} // namespace mitama::unindent::inline literals
