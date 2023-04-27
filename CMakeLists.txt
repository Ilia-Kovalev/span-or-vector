cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    span_or_vector
    VERSION 0.1.0
    DESCRIPTION "A header-only library with an object that behaves as std::vector but can be initialized over existing data as a span. If the object was initialized as a span, then it allocates memory only on operations incompatible with the current span."
    HOMEPAGE_URL "https://github.com/Tristis116/span-or-vector"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----
add_library(span_or_vector_span_or_vector INTERFACE)
add_library(span_or_vector::span_or_vector ALIAS span_or_vector_span_or_vector)

find_package(Boost 1.78.0 REQUIRED)
target_link_libraries(span_or_vector_span_or_vector INTERFACE Boost::boost)

set_property(
    TARGET span_or_vector_span_or_vector PROPERTY
    EXPORT_NAME span_or_vector
)

target_include_directories(
    span_or_vector_span_or_vector ${warning_guard}
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT span_or_vector_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of span_or_vector"
  )
endif()

include(cmake/dev-mode.cmake)
