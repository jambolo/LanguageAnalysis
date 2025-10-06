cmake_minimum_required(VERSION 3.23)

# Create test executable
add_executable(SubtlexImporter_test
    SubtlexImporter_test.cpp
)

# Link against the library being tested and Google Test
target_link_libraries(SubtlexImporter_test
    PRIVATE
    SubtlexImporter
    GTest::gtest
    GTest::gtest_main
)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(SubtlexImporter_test)
