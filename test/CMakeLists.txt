cmake_minimum_required(VERSION 3.14)

project(span_or_vectorTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)
include(../cmake/windows-set-path.cmake)

# ---- Dependencies ----

find_package(Boost REQUIRED COMPONENTS unit_test_framework)

if(PROJECT_IS_TOP_LEVEL)
  find_package(span_or_vector REQUIRED)
  enable_testing()
endif()

# ---- Tests ----

add_executable(span_or_vector_test "source/span_or_vector.cpp")
target_link_libraries(span_or_vector_test PRIVATE span_or_vector::span_or_vector Boost::unit_test_framework)

add_test(NAME span_or_vector_test COMMAND span_or_vector_test)

# ---- End-of-file commands ----

add_folders(Test)
