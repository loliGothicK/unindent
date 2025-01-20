find_package(Catch2 3 REQUIRED)
# These tests can use the Catch2-provided main
add_executable(tests test.cpp)
target_compile_features(tests PRIVATE cxx_std_20)
target_include_directories(tests
    PRIVATE ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

enable_testing()
