# Unindent

This is a small C++20 library that provides a simple way to make indent-adjusted multiline raw string literals at compile time.

## Test Status

[![Full Test](https://github.com/LoliGothick/unindent/actions/workflows/test.yaml/badge.svg)](https://github.com/LoliGothick/unindent/actions/workflows/test.yaml)

## Synopsis

Unindent provides two new user-defined literals for indented strings: `"..."_i` and `"..."_i1`:

The `"..."_i` UDL removes the indent applied in the source file. The behaviour is similar to Coffeescript's [multiline string literals](https://coffeescript.org/#strings):

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
  )"_i;

  static_assert(unindented_str ==
                "def foo():\n  print(\"Hello\")\n\n  print(\"World\")"sv);
}
```

The `"..."_i1` UDL is like `"..."_i`, except it folds "paragraphs" of text into single lines. The behaviour is similar to [YAML's folded multiline strings](https://yaml.org/spec/1.2-old/spec.html#id2796251):

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
  )"_i1;

  static_assert(
      folded_str ==
      "This is the first line. This line is appended to the first.\nThis line follows a line break.   This line ends up indented by two spaces."sv);
}
```

## Guide Level Exlpanation

`"..."_i` and `"..."_i1` are user-defined literals that return `edited_string` objects. `edited_string` is a class template that represents a string that has been edited by an editor function.

`edited_string` has the following APIs:

### format(auto&& ...args)

Invokes `std::format`.
`"..."_i.format(args...)` is same as `std::format("..."_i.to_str(), args...)`.

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
template <mitama::unindent::fixed_string S>
inline constexpr mitama::unindent::edited_string<S, {Your CPO}>
operator ""_your_awesome_user_literal_name() {
  return {};
}
```

### edited_string

`edited_string<fixed_string, Editor>` is a class template that represents a string that has been edited by `Editor`.

`Lit.s` is a `std::array` of original characters that is passed to `Editor`.
`Editor` is a immidiate function that takes `Lit.s` and returns a new `std::array` of characters.

```cpp
template <fixed_string Lit, auto Editor>
  requires requires {
    { Editor(Lit.s) } -> std::convertible_to<decltype(Lit.s)>;
  }
class [[nodiscard]] edited_string {
  // ...
};
```

### Principals

This part explain the design principles of the [P0732R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0732r2.pdf).

- Note 1: `fixed_string` is initialized with a string literals. In addition, due to CTAD (Class Template Argument Deduction), `CharT` and `N` is automatically deduced.
  
  ```cpp
  constexpr fixed_string fs = "abc"; // fixed_string<char, 4>
  ```

- Note 2: Since C++20, `fixed_string` can be used as non-type template arguments with CTAD.
  
  ```cpp
  template <fixed_string S>
  struct foo {};

  foo<"abc"> f; // foo<fixed_string<char, 4>{"abc"}>
  ```

- Note 3: Since C++20, `fixed_string` can be used as a template parameter of a user-defined literals.
  
  ```cpp
  template <fixed_string S>
  inline constexpr auto operator""_fixed() {
    return S;
  }

  "abc"_fixed; // fixed_string<char, 4>{"abc"}
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
