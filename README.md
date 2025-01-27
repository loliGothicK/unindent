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

## Supported OS/Compiler

- Linux
  - GCC: 13, 14
  - Clang: 17, 18, 19
- Apple
  - Apple Clang 15.0.0 (macOS 15)
  - Apple Clang 18.1.8 (macOS 15)
- Windows
  - MSVC: Visual Studio 2022
