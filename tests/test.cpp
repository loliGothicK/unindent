#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <string_view>

#include <unindent/unindent.hpp>

TEST_CASE("unindent", "[unindent]") {
  using namespace std::literals;
  using namespace mitama::unindent;
  constexpr std::string_view unindented_str = R"(
    def foo():
      print("Hello")
      print("World")
  )"_i;

  REQUIRE(unindented_str == "def foo():\n  print(\"Hello\")\n  print(\"World\")");
}
