#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <string_view>

#include <unindent/unindent.hpp>

TEST_CASE("unindent#1", "[unindent]") {
  using namespace std::literals;
  using namespace mitama::unindent::literals;
  constexpr std::string_view unindented_str = R"(
    def foo():
      print("Hello")
      print("World")
  )"_i;

  static_assert(unindented_str ==
                "def foo():\n  print(\"Hello\")\n  print(\"World\")"sv);
}

TEST_CASE("folded#1", "[folded]") {
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

// comprison between unindented and strings
TEST_CASE("comprisons#2", "[unindented]") {
  using namespace std::literals;
  using namespace mitama::unindent::literals;
  constexpr auto unindented_str = R"(
    def foo():
      print("Hello")
      print("World")
  )"_i;

  static_assert(unindented_str ==
                "def foo():\n  print(\"Hello\")\n  print(\"World\")"_i);
  static_assert(unindented_str ==
                "def foo():\n  print(\"Hello\")\n  print(\"World\")"sv);
  static_assert(unindented_str ==
                "def foo():\n  print(\"Hello\")\n  print(\"World\")");

  static_assert("def foo():\n  print(\"Hello\")\n  print(\"World\")"sv ==
                unindented_str);
  static_assert("def foo():\n  print(\"Hello\")\n  print(\"World\")" ==
                unindented_str);
}