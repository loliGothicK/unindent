#include <catch2/catch_test_macros.hpp>

#include <format>
#include <iostream>
#include <string_view>
#include <unindent/unindent.hpp>

TEST_CASE("CTAD#1", "[fixed_string]") {
  [[maybe_unused]] constexpr mitama::unindent::fixed_string _ = "abc";
}

template <mitama::unindent::fixed_string S>
struct foo
{};

TEST_CASE("CTAD#2", "[fixed_string]") {
  [[maybe_unused]] constexpr foo<"abc"> _;
}

TEST_CASE("unindent#1", "[unindent]") {
  using namespace std::literals;
  using namespace mitama::unindent::literals;
  constexpr std::string_view unindented_str = R"(
    def foo():
      print("Hello")
      print("World")
  )"_i.to_str();

  static_assert(
      unindented_str == "def foo():\n  print(\"Hello\")\n  print(\"World\")"sv
  );
}

TEST_CASE("folded#1", "[folded]") {
  using namespace std::literals;
  using namespace mitama::unindent::literals;
  constexpr std::string_view folded_str = R"(
    This is the first line.
    This line is appended to the first.

    This line follows a line break.
      This line ends up indented by two spaces.
  )"_i1.to_str();

  static_assert(
      folded_str ==
      "This is the first line. This line is appended to the first.\nThis line follows a line break.   This line ends up indented by two spaces."sv);
}

// comprison between unindented and folded
TEST_CASE("comprisons#1", "[unindented]") {
  using namespace std::literals;
  using namespace mitama::unindent::literals;
  constexpr auto unindented_str = R"(
    def foo():
      print("Hello")
      print("World")
  )"_i;

  constexpr auto folded_str = R"(
    def foo():
      print("Hello")
      print("World")
  )"_i1;

  static_assert(unindented_str <=> folded_str != std::strong_ordering::equal);
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

  static_assert(
      unindented_str == "def foo():\n  print(\"Hello\")\n  print(\"World\")"_i
  );
  static_assert(
      unindented_str == "def foo():\n  print(\"Hello\")\n  print(\"World\")"sv
  );
  static_assert(
      unindented_str == "def foo():\n  print(\"Hello\")\n  print(\"World\")"
  );

  static_assert(
      "def foo():\n  print(\"Hello\")\n  print(\"World\")"sv == unindented_str
  );
  static_assert(
      "def foo():\n  print(\"Hello\")\n  print(\"World\")" == unindented_str
  );
}

TEST_CASE("format#1", "[folded]") {
  using namespace std::literals;
  using namespace mitama::unindent::literals;
  auto str = std::format(
      R"(
    {}
    {}
  )"_i1.to_str(),
      "Hello", "World"
  );
  REQUIRE(str == "Hello World"sv);
}

TEST_CASE("format#2", "[folded]") {
  using namespace std::literals;
  using namespace mitama::unindent::literals;
  constexpr auto fmt = R"(
    {}
    {}
  )"_i1;
  auto str = fmt.format("Hello", "World");
  REQUIRE(str == "Hello World"sv);
}

// iterator test
TEST_CASE("iterator#1", "[folded]") {
  using namespace std::literals;
  using namespace mitama::unindent::literals;
  constexpr auto folded_str = R"(
    first
    second
  )"_i1;

  static_assert(folded_str == "first second"sv);
}

TEST_CASE("iv#1", "[unindented]") {
  using namespace std::literals;
  using namespace mitama::unindent::literals;
  constexpr auto sv = R"(
    first
    second
  )"_iv;

  static_assert(sv == "first\nsecond"sv);
}

TEST_CASE("i1v#1", "[folded]") {
  using namespace std::literals;
  using namespace mitama::unindent::literals;
  constexpr auto sv = R"(
    first
    second
  )"_i1v;

  static_assert(sv == "first second"sv);
}
