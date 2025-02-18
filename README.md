# Unindent

This is a small C++20 library that provides a simple way to make indent-adjusted multiline raw string literals at compile time.

## Test Status

[![Full Test](https://github.com/LoliGothick/unindent/actions/workflows/test.yaml/badge.svg)](https://github.com/LoliGothick/unindent/actions/workflows/test.yaml)

## Synopsis

Unindent provides four new user-defined literals for indented strings: `_i`,  `_i1`, `_iv`, and `_i1v`.

- `_i` removes the indent applied in the source file and returns a `edited_string`.
- `_i1` folds "paragraphs" of text into single lines and returns a `edited_string`.
- `_iv` is the same as `_i`, but returns a `std::string_view`.
- `_i1v` is the same as `_i1`, but returns a `std::string_view`.

`_i` and `_iv` UDL removes the indent applied in the source file. The behaviour is similar to Coffeescript's [multiline string literals](https://coffeescript.org/#strings):

```cpp
#include <string_view>

#include <unindent/unindent.hpp>

int main() {
  using namespace std::literals;
  using namespace mitama::unindent;

  constexpr auto unindented_str = R"(
    def foo():
      print("Hello")
      print("World")
  )"_iv;

  static_assert(unindented_str ==
                "def foo():\n  print(\"Hello\")\n\n  print(\"World\")"sv);
}
```

The `_i1` UDL is like `_i`, except it folds "paragraphs" of text into single lines. The behaviour is similar to [YAML's folded multiline strings](https://yaml.org/spec/1.2-old/spec.html#id2796251):

```cpp
#include <string_view>

#include <unindent/unindent.hpp>

int main() {
  using namespace std::literals;
  using namespace mitama::unindent::literals;
  constexpr auto folded_str = R"(
    This is the first line.
    This line is appended to the first.

    This line follows a line break.
      This line ends up indented by two spaces.
  )"_i1v;

  static_assert(
      folded_str ==
      "This is the first line. This line is appended to the first.\nThis line follows a line break.   This line ends up indented by two spaces."sv);
}
```

### Differences between `_i` and `_iv` (`_i1` and `_i1v`)

For normal use, you can use `_iv` and `_i1v`, but these are inconvenient when used with `std::format`.
Because `std::format_string` is initialised in the constructor of the immediate function, it is not possible to pass `std::string_view` variables stored in temporary variables.
In this case, you can use `_i` and `_i1` and then call `format` to get the result.

```cpp
#include <string_view>

#include <unindent/unindent.hpp>

int main() {
  using namespace std::literals;
  using namespace mitama::unindent;

  constexpr auto fmt = R"(
    {}
    {}
  )"_i1;
  auto str = fmt.format("Hello", "World");
  // Hello
  // World
}
```

## Guide Level Exlpanation

`_i` and `_i1` are user-defined literals that return `edited_string` objects. `edited_string` is a class template that represents a string that has been edited by an editor function.

`edited_string` has the following APIs:

### format(auto&& ...args)

Invokes `std::format`.
`_i.format(args...)` is same as `std::format(_i.to_str(), args...)`.

```cpp
  using namespace mitama::unindent::literals;
  constexpr auto fmt = R"(
    {}
    {}
  )"_i1;
  auto str = fmt.format("Hello", "World");
  // Hello
  // World
```

### to_str()

Returns `basic_string_view`.

```cpp
  using namespace mitama::unindent;

  constexpr std::string_view _ = R"(
    def foo():
      print("Hello")
      print("World")
  )"_i.to_str();
```

### iterator support

```cpp
  constexpr auto begin() const noexcept;
  constexpr auto end() const noexcept;
  constexpr auto cbegin() const noexcept;
  constexpr auto cend() const noexcept;
  constexpr auto rbegin() const noexcept;
  constexpr auto rend() const noexcept;
  constexpr auto crbegin() const noexcept;
  constexpr auto crend() const noexcept;
```

### conparison operators

`edited_string` and `std::string_view` can be compared with each other.
Also, `edited_string` and `edited_string` can be compared.

`<=>`, `==`, `!=`, `<`, `>`, `<=`, `>=` are supported.

## Reference Level Explanation

Customization Point Object `Editor` is a Non-template function object that takes a `std::array` of characters and returns a new `std::array` of characters.

To make your own literal operator, you can use `edited_string` as follows:

```cpp
template <mitama::unindent::basic_fixed_string S>
inline constexpr mitama::unindent::edited_string<S, {Your CPO}>
operator ""_your_awesome_user_literal_name() {
  return {};
}
```

### edited_string

`edited_string<S, Editor>` is a class template that represents a string that has been edited by `Editor`.

`Lit.s` is a `std::array` of original characters that is passed to `Editor`.
`Editor` is a immidiate function that takes `Lit.s` and returns a new `std::array` of characters.

```cpp
template <basic_fixed_string Lit, auto Editor>
  requires requires {
    { Editor(Lit.s) } -> std::convertible_to<decltype(Lit.s)>;
  }
class [[nodiscard]] edited_string {
  // ...
};
```

### Principals

This part explain the design principles of this library and how it works.
If you want to read original paper, please refer to the [P0732R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0732r2.pdf).

- First: `basic_fixed_string` is initialized with a string literals. In addition, due to CTAD (Class Template Argument Deduction), `CharT` and `N` is automatically deduced.
  
  ```cpp
  constexpr basic_fixed_string fs = "abc"; // basic_fixed_string<char, 4>
  ```

- Second: Since C++20, `basic_fixed_string` can be used as non-type template arguments with CTAD.
  
  ```cpp
  template <basic_fixed_string S>
  struct foo {};

  foo<"abc"> f; // foo<basic_fixed_string<char, 4>{"abc"}>
  ```

- Third: Since C++20, `basic_fixed_string` can be used as a template parameter of a user-defined literals because `operator""_udl<"abc">()` is a well-formed.
  
  ```cpp
  template <basic_fixed_string S>
  inline consteval auto operator""_fixed() {
    return S;
  }

  "abc"_fixed; // operator""_fixed<"abc">()
  ```

- Finally, make class template as the return type of user-defined literals. This class is needed for statically storing the result of edited strings.

  ```cpp
  template <basic_fixed_string S, auto Editor>
  class edited_string {
    static constexpr decltype(S.data) s = Editor(S.data);
    // ...
  };

  template <basic_fixed_string S>
  inline consteval auto operator""_i() {
    return edited_string<S, details::to_unindented>{};
  }
  ```

  ```cpp
  template <basic_fixed_string S>
  inline consteval std::string_view operator""_iv() {
    return edited_string<S, details::to_unindented>::value();
  }
  ```

## Supported OS/Compiler

- Linux
  - GCC: 13, 14
  - Clang: 17, 18, 19
- Apple
  - Apple Clang 15.0.0 (on macOS 15)
  - Apple Clang 18.1.8 (on macOS 15)
- Windows
  - MSVC: Visual Studio 17 2022 (on Windows Server 2022 and 2025)
