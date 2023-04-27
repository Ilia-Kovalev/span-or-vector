# Span or vector

[![Continuous Integration](https://github.com/Tristis116/span-or-vector/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/Tristis116/span-or-vector/actions/workflows/ci.yml)

This is a small cross-platform header-only library that has an `span_or_vector` container 
that behaves like a `std::vector` but may be initialized as a span over existing data. 
If it is initialized as a span, then it behaves like `std::vector` with memory
allocated at provided pointer and capacity as a span size. When called method requires memory 
reallocation, `span_or_vector` allocates its own memory an behaves identically to `std::vector`.

# Building and installing

See the [BUILDING](BUILDING.md) document.

# Licensing

See the [LICENSE](LICENSE.md) document.
