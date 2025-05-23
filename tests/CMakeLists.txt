find_package(Catch2 3 CONFIG REQUIRED)
add_executable(tests test.cpp regressions.cpp)
target_compile_features(tests PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(tests PRIVATE /W4 /permissive- /Zc:twoPhase-)
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(tests PRIVATE -Wall -Wextra -ftemplate-backtrace-limit=0)
else() # Clang
    target_compile_options(tests PRIVATE -Wall -Wextra -pedantic-errors -ftemplate-backtrace-limit=0)
endif()

if(APPLE)
    target_compile_definitions(tests PRIVATE _GNU_SOURCE)
endif()

target_include_directories(tests
    PRIVATE ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(tests)

enable_testing()
