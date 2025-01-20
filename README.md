# unindent

This is a small C++20 library that provides a simple way to unindent the raw string literals at compile time.

## Example

```cpp
#include <iostream>
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

  static_assert(unindented_str == "def foo():\n  print(\"Hello\")\n  print(\"World\")"sv);
}
```

