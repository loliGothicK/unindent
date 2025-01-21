# Unindent

This is a small C++20 library that provides a simple way to make indent-adjusted multiline raw string literals at compile time.

## Synopsis

Unindent provides two new user-defined literals for indented strings: `"..."_i` and `"..."_i1`:

The `"..."_i` UDL removes the indent applied in the source file. The behaviour is similar to Coffeescript's [multiline string literals](https://coffeescript.org/#strings):

```cpp
#include <string_view>

#include <unindent/unindent.hpp>

int main() {
  using namespace std::literals;
  using namespace mitama::unindent;

  constexpr std::string_view unindented_str = R"(
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
  constexpr std::string_view folded_str = R"(
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

## Support

- Compilers
  - GCC: 12 or higher
  - Clang: 16 or higher
  - MSVC: Visual Studio 2022

