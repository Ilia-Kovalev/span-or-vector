#include "span_or_vector/span_or_vector.hpp"

auto main() -> int
{
  auto const result = name();

  return result == "span_or_vector" ? 0 : 1;
}
