/*
 * MIT License
 *
 * Copyright (c) 2023 Ilia Kovalev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * input the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included input
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define BOOST_TEST_MODULE test span_or_vector

#include "span_or_vector/span_or_vector.hpp"

#include <boost/test/included/unit_test.hpp>

namespace span_or_vector
{
template<class T>
class test_allocator : private std::allocator<T>
{
  using base = std::allocator<T>;

public:
  using value_type = typename base::value_type;
  using type = test_allocator;
  using is_always_equal = std::true_type;
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap = std::true_type;

  test_allocator() = default;
  explicit test_allocator(std::string label)
      : label_(std::move(label))
  {
  }

  template<class U>
  explicit test_allocator(const test_allocator<U>&) noexcept
  {
  }

  auto allocate(std::size_t n) -> T*
  {
    ++alloc_counter_;
    return base::allocate(n);
  }

  using base::deallocate;

  auto n_allocations() const -> int { return alloc_counter_; }

  auto label() const -> std::string { return label_; }

private:
  int alloc_counter_ = 0;
  std::string label_;
};

template<class T>
auto operator!=(const test_allocator<T>& lhs, const test_allocator<T>& rhs)
    -> bool
{
  return !(lhs == rhs);
}

template<class T>
auto operator==(const test_allocator<T>&, const test_allocator<T>&) -> bool
{
  return true;
}

template<class T>
using test_type = span_or_vector<T, test_allocator<T>>;

// NOLINTBEGIN
// cppcheck-suppress unknownMacro
BOOST_AUTO_TEST_SUITE(test_constructors)

BOOST_AUTO_TEST_CASE(test_default)
{
  test_type<int> out;

  BOOST_CHECK(out.empty());
  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
}

BOOST_AUTO_TEST_CASE(test_alloc)
{
  test_allocator<int> in_alloc {"a"};

  test_type<int> out {in_alloc};

  BOOST_CHECK(out.empty());
  BOOST_CHECK(out.is_vector());

  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
}

BOOST_AUTO_TEST_CASE(test_count_value_alloc)
{
  const std::vector<int> input(3, 0);
  test_allocator<int> in_alloc {"a"};

  test_type<int> out {input.size(), 0, in_alloc};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
}

BOOST_AUTO_TEST_CASE(test_first_last_alloc)
{
  const std::vector<int> input {1, 2, 3};
  test_allocator<int> in_alloc {"a"};

  test_type<int> out(input.begin(), input.end(), in_alloc);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
}

BOOST_AUTO_TEST_CASE(test_copy_from_vector)
{
  const std::vector<int, test_allocator<int>> input {{1, 2, 3},
                                                     test_allocator<int> {"a"}};

  test_type<int> out {input};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations()
                        - input.get_allocator().n_allocations(),
                    1);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), input.get_allocator().label());
  BOOST_CHECK_NE(out.data(), input.data());
}

BOOST_AUTO_TEST_CASE(test_copy_from_vector_with_alloc)
{
  std::vector<int, test_allocator<int>> input {1, 2, 3};
  test_allocator<int> in_alloc {"a"};

  test_type<int> out {input, in_alloc};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_NE(out.data(), input.data());
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
}

BOOST_AUTO_TEST_CASE(test_move_from_vector)
{
  std::vector<int, test_allocator<int>> input {{1, 2, 3},
                                               test_allocator<int> {"a"}};
  const auto exp {input};

  test_type<int> out {std::move(input)};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), exp.get_allocator().label());
}

BOOST_AUTO_TEST_CASE(test_move_from_vector_with_alloc)
{
  std::vector<int, test_allocator<int>> input {1, 2, 3};
  test_allocator<int> in_alloc {"a"};
  const auto exp = input;

  test_type<int> out {std::move(input), in_alloc};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
}

BOOST_AUTO_TEST_CASE(test_from_init_list)
{
  auto const input = {1, 2, 3};
  test_allocator<int> in_alloc {"a"};

  test_type<int> out {input, in_alloc};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
}

BOOST_AUTO_TEST_CASE(test_from_empty_list)
{
  test_type<int> out {};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK(out.empty());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
}

BOOST_AUTO_TEST_CASE(test_span)
{
  std::vector<int> input {1, 2, 3};

  test_type<int> out {input.data(), input.size()};

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
  BOOST_CHECK_EQUAL(out.capacity(), input.size());
}

BOOST_AUTO_TEST_CASE(test_span_with_alloc)
{
  std::vector<int> input {1, 2, 3};
  test_allocator<int> in_alloc {"a"};

  test_type<int> out {input.data(), input.size(), in_alloc};

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
  BOOST_CHECK_EQUAL(out.capacity(), input.size());
}

BOOST_AUTO_TEST_CASE(test_copy_as_span)
{
  std::vector<int> in_data {1, 2, 3};
  const test_type<int> input {
      in_data.data(), in_data.size(), test_allocator<int> {"a"}};

  const test_type<int> out {input};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_NE(input.data(), out.data());
  BOOST_CHECK_EQUAL(input.get_allocator().label(), out.get_allocator().label());
}

BOOST_AUTO_TEST_CASE(test_copy_as_vector)
{
  const test_type<int> input {{1, 2, 3}, test_allocator<int> {"a"}};

  const test_type<int> out {input};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    input.get_allocator().n_allocations() + 1);
  BOOST_CHECK_EQUAL(input.get_allocator().label(), out.get_allocator().label());
  BOOST_CHECK_NE(input.data(), out.data());
}

BOOST_AUTO_TEST_SUITE_END()

// cppcheck-suppress unknownMacro
BOOST_AUTO_TEST_SUITE(test_assignments)

BOOST_AUTO_TEST_CASE(test_copy_span_to_span)
{
  std::vector<int> in_data1 {1, 2, 3};
  std::vector<int> in_data2 {4, 5, 6, 7};
  const test_type<int> input {
      in_data2.data(), in_data2.size(), test_allocator<int> {"a"}};

  test_type<int> out {in_data1.data(), in_data1.size()};
  out = input;

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), input.get_allocator().label());
  BOOST_CHECK_NE(out.data(), input.data());
}

BOOST_AUTO_TEST_SUITE_END()

// NOLINTEND

}  // namespace span_or_vector
