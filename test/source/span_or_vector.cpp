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

#define BOOST_TEST_MODULE span_or_vector

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

template<class T>
using vector_type = std::vector<T, test_allocator<T>>;

// NOLINTBEGIN(bugprone-use-after-move, hicpp-invalid-access-moved,
// clang-diagnostic-self-assign-overloaded,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

namespace constructors
{
// cppcheck-suppress unknownMacro
BOOST_AUTO_TEST_SUITE(constructors)

BOOST_AUTO_TEST_CASE(default_constructor)
{
  test_type<int> out;

  BOOST_CHECK(out.empty());
  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
}  // namespace span_or_vector

BOOST_AUTO_TEST_CASE(alloc)
{
  test_allocator<int> in_alloc {"a"};

  test_type<int> out {in_alloc};

  BOOST_CHECK(out.empty());
  BOOST_CHECK(out.is_vector());

  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
}

BOOST_AUTO_TEST_CASE(count_value_alloc)
{
  const vector_type<int> input(3, 0);
  test_allocator<int> in_alloc {"a"};

  test_type<int> out {input.size(), 0, in_alloc};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
}

BOOST_AUTO_TEST_CASE(first_last_alloc)
{
  const vector_type<int> input {1, 2, 3};
  test_allocator<int> in_alloc {"a"};

  test_type<int> out(input.begin(), input.end(), in_alloc);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
}

BOOST_AUTO_TEST_CASE(copy_vector)
{
  const vector_type<int> input {{1, 2, 3}, test_allocator<int> {"a"}};

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

BOOST_AUTO_TEST_CASE(copy_vector_with_alloc)
{
  vector_type<int> input {1, 2, 3};
  test_allocator<int> in_alloc {"a"};

  test_type<int> out {input, in_alloc};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_NE(out.data(), input.data());
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
}

BOOST_AUTO_TEST_CASE(move_vector)
{
  vector_type<int> input {{1, 2, 3}, test_allocator<int> {"a"}};
  const auto exp {input};

  test_type<int> out {std::move(input)};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), exp.get_allocator().label());
}

BOOST_AUTO_TEST_CASE(move_vector_with_alloc)
{
  vector_type<int> input {1, 2, 3};
  test_allocator<int> in_alloc {"a"};
  const auto exp = input;

  test_type<int> out {std::move(input), in_alloc};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
}

BOOST_AUTO_TEST_CASE(ilist)
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

BOOST_AUTO_TEST_CASE(empty_ilist)
{
  const std::initializer_list<int> input = {};
  test_type<int> out {input};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK(out.empty());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
}

BOOST_AUTO_TEST_CASE(as_span)
{
  vector_type<int> input {1, 2, 3};

  test_type<int> out {input.data(), input.size()};

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
  BOOST_CHECK_EQUAL(out.capacity(), input.size());
}

BOOST_AUTO_TEST_CASE(as_span_with_alloc)
{
  vector_type<int> input {1, 2, 3};
  test_allocator<int> in_alloc {"a"};

  test_type<int> out {input.data(), input.size(), in_alloc};

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
  BOOST_CHECK_EQUAL(out.capacity(), input.size());
}

BOOST_AUTO_TEST_CASE(copy_sov_is_span)
{
  vector_type<int> in_data {1, 2, 3};
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

BOOST_AUTO_TEST_CASE(copy_sov_is_vector)
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

BOOST_AUTO_TEST_CASE(move_sov_is_span)
{
  vector_type<int> in_data {1, 2, 3};
  std::size_t in_size {2};
  test_allocator<int> in_alloc {"a"};

  auto expected = in_data;
  expected.resize(2);

  test_type<int> input {in_data.data(), in_data.size(), in_alloc};
  input.resize(in_size);

  test_type<int> out {std::move(input)};

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.data(), in_data.data());
  BOOST_CHECK_EQUAL(out.capacity(), in_data.size());
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), expected.begin(), expected.end());

  BOOST_CHECK(input.empty());
  BOOST_CHECK_EQUAL(input.data(), nullptr);
  BOOST_CHECK(input.is_vector());
  BOOST_CHECK_EQUAL(input.get_allocator().label(), "");
}

BOOST_AUTO_TEST_CASE(move_sov_is_vector)
{
  vector_type<int> in_data {1, 2, 3};
  test_allocator<int> in_alloc {"a"};

  test_type<int> input {in_data.begin(), in_data.end(), in_alloc};

  test_type<int> out {std::move(input)};

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), in_data.size());
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), in_data.begin(), in_data.end());

  BOOST_CHECK(input.empty());
  BOOST_CHECK_EQUAL(input.data(), nullptr);
  BOOST_CHECK(input.is_vector());
  BOOST_CHECK_EQUAL(input.get_allocator().label(), "");
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace constructors

namespace assignments
{

// cppcheck-suppress unknownMacro
BOOST_AUTO_TEST_SUITE(assignments)

BOOST_AUTO_TEST_CASE(self_assignment)
{
  test_type<int> input {1, 2, 3};
  test_type<int> expected {1, 2, 3};
  input = input;

  BOOST_CHECK_EQUAL_COLLECTIONS(
      input.begin(), input.end(), expected.begin(), expected.end());
  BOOST_CHECK_EQUAL(input.get_allocator().n_allocations(),
                    expected.get_allocator().n_allocations());
}

BOOST_AUTO_TEST_CASE(copy_sov_as_span_to_span)
{
  vector_type<int> in_data1 {1, 2, 3};
  vector_type<int> in_data2 {4, 5, 6, 7};
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

BOOST_AUTO_TEST_CASE(copy_sov_as_span_to_bigger_vector)
{
  vector_type<int> in_data1 {1, 2, 3};
  vector_type<int> in_data2 {4, 5, 6, 7};
  const test_type<int> input {
      in_data1.data(), in_data1.size(), test_allocator<int> {"a"}};

  test_type<int> out {in_data2.begin(), in_data2.end()};
  out = input;

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), input.get_allocator().label());
  BOOST_CHECK_NE(out.data(), input.data());
}

BOOST_AUTO_TEST_CASE(copy_sov_as_span_to_smaller_vector)
{
  vector_type<int> in_data1 {1, 2, 3};
  vector_type<int> in_data2 {4, 5};
  const test_type<int> input {
      in_data1.data(), in_data1.size(), test_allocator<int> {"a"}};

  test_type<int> out {in_data2.begin(), in_data2.end()};
  out = input;

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), input.get_allocator().label());
  BOOST_CHECK_NE(out.data(), input.data());
}

BOOST_AUTO_TEST_CASE(copy_sov_as_vector_to_bigger_span)
{
  const test_type<int> input {{1, 2, 3}, test_allocator<int> {"a"}};
  vector_type<int> in_data {4, 5, 6, 7};

  test_type<int> out {in_data.data(), in_data.size()};
  out = input;

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations()
                        - input.get_allocator().n_allocations(),
                    0);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), input.get_allocator().label());
  BOOST_CHECK_NE(out.data(), input.data());
  BOOST_CHECK_EQUAL(out.data(), in_data.data());
}

BOOST_AUTO_TEST_CASE(copy_sov_as_vector_to_smaller_span)
{
  const test_type<int> input {{1, 2, 3}, test_allocator<int> {"a"}};
  vector_type<int> in_data {4, 5};

  test_type<int> out {in_data.data(), in_data.size()};
  out = input;

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations()
                        - input.get_allocator().n_allocations(),
                    1);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), input.get_allocator().label());
  BOOST_CHECK_NE(out.data(), input.data());
  BOOST_CHECK_NE(out.data(), in_data.data());
}
BOOST_AUTO_TEST_CASE(copy_sov_as_vector_to_bigger_vector)
{
  const test_type<int> input {{1, 2, 3}, test_allocator<int> {"a"}};

  test_type<int> out {{4, 5, 6, 7}, test_allocator<int> {"b"}};
  out = input;

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations()
                        - input.get_allocator().n_allocations(),
                    0);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), input.get_allocator().label());
  BOOST_CHECK_NE(out.data(), input.data());
}

BOOST_AUTO_TEST_CASE(copy_sov_as_vector_to_smaller_vector)
{
  const test_type<int> input {{1, 2, 3}, test_allocator<int> {"a"}};

  test_type<int> out {{4, 5}, test_allocator<int> {"b"}};
  out = input;

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations()
                        - input.get_allocator().n_allocations(),
                    1);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), input.get_allocator().label());
  BOOST_CHECK_NE(out.data(), input.data());
}

BOOST_AUTO_TEST_CASE(move_sov_as_span_to_span)
{
  vector_type<int> in_data1 {1, 2, 3};
  vector_type<int> in_data2 {4, 5};

  test_allocator<int> in_alloc {"a"};

  test_type<int> input {in_data1.data(), in_data1.size(), in_alloc};
  test_type<int> out {
      in_data2.data(), in_data2.size(), test_allocator<int> {"b"}};

  out = std::move(input);

  BOOST_CHECK(input.is_vector());
  BOOST_CHECK(input.empty());
  BOOST_CHECK_EQUAL(input.data(), nullptr);

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.data(), in_data1.data());
  BOOST_CHECK_EQUAL(out.size(), in_data1.size());
  BOOST_CHECK_EQUAL(out.capacity(), in_data1.size());
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
}

BOOST_AUTO_TEST_CASE(move_sov_as_span_to_vector)
{
  vector_type<int> in_data1 {1, 2, 3};
  vector_type<int> in_data2 {4, 5};

  test_allocator<int> in_alloc {"a"};

  test_type<int> input {in_data1.data(), in_data1.size(), in_alloc};
  test_type<int> out {
      in_data2.begin(), in_data2.end(), test_allocator<int> {"b"}};

  out = std::move(input);

  BOOST_CHECK(input.is_vector());
  BOOST_CHECK(input.empty());
  BOOST_CHECK_EQUAL(input.data(), nullptr);

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.data(), in_data1.data());
  BOOST_CHECK_EQUAL(out.size(), in_data1.size());
  BOOST_CHECK_EQUAL(out.capacity(), in_data1.size());
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
}

BOOST_AUTO_TEST_CASE(move_sov_as_vector_to_span)
{
  vector_type<int> in_data1 {1, 2, 3};
  vector_type<int> in_data2 {4, 5};
  const auto in_data2_copy = in_data2;

  test_allocator<int> in_alloc {"a"};

  test_type<int> input {in_data1.begin(), in_data1.end(), in_alloc};
  test_type<int> out {
      in_data2.data(), in_data2.size(), test_allocator<int> {"b"}};

  out = std::move(input);

  BOOST_CHECK(input.is_vector());
  BOOST_CHECK(input.empty());
  BOOST_CHECK_EQUAL(input.data(), nullptr);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.size(), in_data1.size());
  BOOST_CHECK_EQUAL(out.capacity(), in_data1.size());
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations()
                        - input.get_allocator().n_allocations(),
                    0);

  BOOST_CHECK_EQUAL_COLLECTIONS(in_data2.begin(),
                                in_data2.end(),
                                in_data2_copy.begin(),
                                in_data2_copy.end());
}

BOOST_AUTO_TEST_CASE(move_sov_as_vector_to_vector)
{
  vector_type<int> in_data1 {1, 2, 3};
  vector_type<int> in_data2 {4, 5};

  std::size_t in_capacity {10};

  test_allocator<int> in_alloc {"a"};

  test_type<int> input {in_data1.begin(), in_data1.end(), in_alloc};
  input.reserve(in_capacity);
  test_type<int> out {
      in_data2.begin(), in_data2.end(), test_allocator<int> {"b"}};

  out = std::move(input);

  BOOST_CHECK(input.is_vector());
  BOOST_CHECK(input.empty());
  BOOST_CHECK_EQUAL(input.data(), nullptr);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.size(), in_data1.size());
  BOOST_CHECK_EQUAL(out.capacity(), in_capacity);
  BOOST_CHECK_EQUAL(out.get_allocator().label(), in_alloc.label());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations()
                        - input.get_allocator().n_allocations(),
                    0);
}

BOOST_AUTO_TEST_CASE(copy_vector)
{
  vector_type<int> input {{1, 2, 3}, test_allocator<int> {"a"}};
  input.reserve(10);

  const auto exp = input;

  test_type<int> out;
  out = input;

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_NE(out.data(), input.data());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().label(), exp.get_allocator().label());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
}

BOOST_AUTO_TEST_CASE(move_vector)
{
  vector_type<int> input {{1, 2, 3}, test_allocator<int> {"a"}};
  input.reserve(10);
  vector_type<int> input2 {{1, 2, 3}, test_allocator<int> {"a"}};
  input2.reserve(10);

  const auto exp = std::move(input2);

  test_type<int> out;
  out = std::move(input);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(input.data(), input2.data());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().label(), exp.get_allocator().label());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
}

BOOST_AUTO_TEST_CASE(ilist)
{
  const auto input = {1, 2, 3};

  vector_type<int> exp;
  exp = input;

  test_type<int> out;
  out = input;

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
}

BOOST_AUTO_TEST_CASE(assign_count_value_to_small_span)
{
  vector_type<int> input {1, 2, 3};
  const int in_value = 10;
  const std::size_t in_count = 5;

  vector_type<int> exp {input};
  exp.assign(in_count, in_value);

  test_type<int> out {input.data(), input.size()};
  out.assign(in_count, in_value);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
}

BOOST_AUTO_TEST_CASE(assign_count_value_to_large_span)
{
  vector_type<int> input {1, 2, 3};
  const int in_value = 10;
  const std::size_t in_count = 2;
  const std::size_t in_size = 1;

  vector_type<int> exp {input};
  exp.resize(in_size);
  exp.assign(in_count, in_value);

  test_type<int> out {input.data(), input.size()};
  out.resize(in_size);
  out.assign(in_count, in_value);

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
}

BOOST_AUTO_TEST_CASE(assign_count_value_to_vector)
{
  const vector_type<int> input {1, 2, 3};
  const int in_value = 10;
  const std::size_t in_count = 5;

  vector_type<int> exp {input};
  exp.assign(in_count, in_value);

  test_type<int> out {input};
  out.assign(in_count, in_value);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
}

BOOST_AUTO_TEST_CASE(assign_first_last_to_small_span)
{
  vector_type<int> input {1, 2, 3};
  const std::initializer_list<int> in_data_to_assign {5, 6, 7, 8};

  vector_type<int> exp {input};
  exp.assign(in_data_to_assign.begin(), in_data_to_assign.end());

  test_type<int> out {input.data(), input.size()};
  out.assign(in_data_to_assign.begin(), in_data_to_assign.end());

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
}

BOOST_AUTO_TEST_CASE(assign_first_last_to_large_span)
{
  vector_type<int> input {1, 2, 3};
  const std::initializer_list<int> in_data_to_assign {5, 6};
  const std::size_t in_size = 1;

  vector_type<int> exp {input};
  exp.resize(in_size);
  exp.assign(in_data_to_assign.begin(), in_data_to_assign.end());

  test_type<int> out {input.data(), input.size()};
  out.resize(in_size);
  out.assign(in_data_to_assign.begin(), in_data_to_assign.end());

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
}

BOOST_AUTO_TEST_CASE(assign_first_last_to_vector)
{
  const vector_type<int> input {1, 2, 3};
  const std::initializer_list<int> in_data_to_assign {5, 6};

  vector_type<int> exp {input};
  exp.assign(in_data_to_assign.begin(), in_data_to_assign.end());

  test_type<int> out {input};
  out.assign(in_data_to_assign.begin(), in_data_to_assign.end());

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
}

BOOST_AUTO_TEST_CASE(assign_ilist_to_small_span)
{
  vector_type<int> input {1, 2, 3};
  const std::initializer_list<int> in_data_to_assign {5, 6, 7, 8};

  vector_type<int> exp {input};
  exp.assign(in_data_to_assign);

  test_type<int> out {input.data(), input.size()};
  out.assign(in_data_to_assign);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
}

BOOST_AUTO_TEST_CASE(assign_ilist_to_large_span)
{
  vector_type<int> input {1, 2, 3};
  const std::initializer_list<int> in_data_to_assign {5, 6};
  const std::size_t in_size = 1;

  vector_type<int> exp {input};
  exp.resize(in_size);
  exp.assign(in_data_to_assign);

  test_type<int> out {input.data(), input.size()};
  out.resize(in_size);
  out.assign(in_data_to_assign);

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 0);
}

BOOST_AUTO_TEST_CASE(assign_ilist_to_vector)
{
  const vector_type<int> input {1, 2, 3};
  const std::initializer_list<int> in_data_to_assign {5, 6};

  vector_type<int> exp {input};
  exp.assign(in_data_to_assign);

  test_type<int> out {input};
  out.assign(in_data_to_assign);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace assignments

namespace element_access
{
BOOST_AUTO_TEST_SUITE(element_access)

BOOST_AUTO_TEST_CASE(at_as_span)
{
  vector_type<int> input {1, 2, 3};

  test_type<int> out {input.data(), input.size()};
  out.at(1) = 10;

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.end());
}

BOOST_AUTO_TEST_CASE(at_as_vector)
{
  vector_type<int> input {1, 2, 3};
  const std::size_t in_i {1};
  const int in_val {10};

  test_type<int> out {input};
  out.at(in_i) = in_val;

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.at(in_i), in_val);
}

BOOST_AUTO_TEST_CASE(at_out_of_range)
{
  const test_type<int> input {1, 2, 3};

  BOOST_CHECK_THROW(input.at(input.size() + 10), std::out_of_range);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace element_access

namespace capacity
{
BOOST_AUTO_TEST_SUITE(capacity)

BOOST_AUTO_TEST_CASE(empty_as_span)
{
  vector_type<int> input {1, 2, 3};

  const test_type<int> out {input.data(), 0};

  BOOST_CHECK(out.is_span());
  BOOST_CHECK(out.empty());
}

BOOST_AUTO_TEST_CASE(empty_as_span_after_resize)
{
  vector_type<int> input {1, 2, 3};

  test_type<int> out {input.data(), input.size()};
  out.resize(0);

  BOOST_CHECK(out.is_span());
  BOOST_CHECK(out.empty());
}

BOOST_AUTO_TEST_CASE(empty_as_vector)
{
  const test_type<int> out;

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK(out.empty());
}

BOOST_AUTO_TEST_CASE(size_as_span)
{
  vector_type<int> input {1, 2, 3};

  const test_type<int> out {input.data(), input.size()};

  BOOST_CHECK_EQUAL(out.size(), input.size());
}

BOOST_AUTO_TEST_CASE(size_as_vector)
{
  vector_type<int> input {1, 2, 3};

  const test_type<int> out {input};

  BOOST_CHECK_EQUAL(out.size(), input.size());
}

BOOST_AUTO_TEST_CASE(max_size_as_span)
{
  vector_type<int> input {1, 2, 3};

  const test_type<int> out {input.data(), input.size()};

  BOOST_CHECK_EQUAL(out.max_size(), input.max_size());
}

BOOST_AUTO_TEST_CASE(max_size_as_vector)
{
  vector_type<int> input {1, 2, 3};

  const test_type<int> out {input};

  BOOST_CHECK_EQUAL(out.max_size(), input.max_size());
}

BOOST_AUTO_TEST_CASE(reserve_as_span)
{
  vector_type<int> input {1, 2, 3};
  std::size_t new_cap {10};

  test_type<int> out {input.data(), input.size()};

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), input.size());

  out.reserve(input.size());
  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.size(), input.size());
  BOOST_CHECK_EQUAL(out.capacity(), input.size());

  out.reserve(new_cap);
  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.size(), input.size());
  BOOST_CHECK_EQUAL(out.capacity(), new_cap);
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
}

BOOST_AUTO_TEST_CASE(reserve_as_vector)
{
  const auto input = {1, 2, 3};

  vector_type<int> exp {input};
  std::size_t new_cap {10};

  test_type<int> out {input};

  exp.reserve(exp.size());
  out.reserve(out.size());

  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());

  exp.reserve(new_cap);
  out.reserve(new_cap);

  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
}

BOOST_AUTO_TEST_CASE(shrink_to_fit_as_span)
{
  vector_type<int> input {1, 2, 3};
  const std::size_t in_size {2};

  test_type<int> out {input.data(), input.size()};

  out.shrink_to_fit();
  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), input.size());

  out.resize(in_size);
  out.shrink_to_fit();

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), in_size);
  BOOST_CHECK_EQUAL_COLLECTIONS(
      out.begin(), out.end(), input.begin(), input.begin() + in_size);
}

BOOST_AUTO_TEST_CASE(shrink_to_fit_as_vector)
{
  vector_type<int> input {1, 2, 3};
  const std::size_t in_size {2};

  test_type<int> out {input};
  vector_type<int> exp {input};

  out.shrink_to_fit();
  exp.shrink_to_fit();

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());

  out.resize(in_size);
  exp.resize(in_size);

  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace capacity

namespace modifiers
{
BOOST_AUTO_TEST_SUITE(modifiers)

BOOST_AUTO_TEST_CASE(clear_as_span)
{
  vector_type<int> input {1, 2, 3};

  test_type<int> out {input.data(), input.size()};

  out.clear();

  BOOST_CHECK(out.is_span());
  BOOST_CHECK(out.empty());
  BOOST_CHECK_EQUAL(out.capacity(), input.size());
}

BOOST_AUTO_TEST_CASE(clear_as_vector)
{
  vector_type<int> input {1, 2, 3};

  test_type<int> out {input};
  vector_type<int> exp {input};

  out.clear();
  exp.clear();

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
}

BOOST_AUTO_TEST_CASE(insert_pos_value_as_span_with_enough_capacity)
{
  vector_type<int> input {1, 2, 3};
  const int in_value {6};
  auto exp = input;

  test_type<int> out {input.data(), input.size()};

  exp.resize(2);
  out.resize(2);

  const auto out_it = out.insert(out.begin() + 1, in_value);
  const auto exp_it = exp.insert(exp.begin() + 1, in_value);

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), input.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(insert_pos_value_as_span_with_not_enough_capacity)
{
  vector_type<int> input {1, 2, 3};
  const int in_value {6};
  auto exp = input;

  test_type<int> out {input.data(), input.size()};

  const auto out_it = out.insert(out.begin() + 1, in_value);
  const auto exp_it = exp.insert(exp.begin() + 1, in_value);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), out.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(insert_pos_value_as_vector)
{
  vector_type<int> input {1, 2, 3};
  const int in_value {6};
  auto exp = input;

  test_type<int> out {input};

  const auto out_it = out.insert(out.begin() + 1, in_value);
  const auto exp_it = exp.insert(exp.begin() + 1, in_value);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(insert_pos_rvalue_as_span_with_enough_capacity)
{
  vector_type<std::string> input {"a", "b", "c"};
  auto exp = input;
  std::string in_value {"d"};
  std::string in_value_exp {in_value};

  test_type<std::string> out {input.data(), input.size()};

  exp.resize(2);
  out.resize(2);

  const auto out_it = out.insert(out.begin() + 1, std::move(in_value));
  const auto exp_it = exp.insert(exp.begin() + 1, std::move(in_value_exp));

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), input.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());

  BOOST_CHECK_EQUAL(in_value, in_value_exp);
}

BOOST_AUTO_TEST_CASE(insert_pos_rvalue_as_span_with_not_enough_capacity)
{
  vector_type<std::string> input {"a", "b", "c"};
  auto exp = input;
  std::string in_value {"d"};
  std::string in_value_exp {in_value};

  test_type<std::string> out {input.data(), input.size()};

  const auto out_it = out.insert(out.begin() + 1, std::move(in_value));
  const auto exp_it = exp.insert(exp.begin() + 1, std::move(in_value_exp));

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), out.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());

  BOOST_CHECK_EQUAL(in_value, in_value_exp);
}

BOOST_AUTO_TEST_CASE(insert_pos_rvalue_as_vector)
{
  vector_type<std::string> input {"a", "b", "c"};
  auto exp = input;
  std::string in_value {"d"};
  std::string in_value_exp {in_value};

  test_type<std::string> out {input};

  const auto out_it = out.insert(out.begin() + 1, std::move(in_value));
  const auto exp_it = exp.insert(exp.begin() + 1, std::move(in_value_exp));

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());

  BOOST_CHECK_EQUAL(in_value, in_value_exp);
}

BOOST_AUTO_TEST_CASE(insert_pos_value_count_as_span_with_enough_capacity)
{
  vector_type<int> input {1, 2, 3};
  auto exp = input;

  test_type<int> out {input.data(), input.size()};

  exp.resize(1);
  out.resize(1);

  const auto out_it = out.insert(out.begin(), 2, 6);
  const auto exp_it = exp.insert(exp.begin(), 2, 6);

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), input.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(insert_pos_value_count_as_span_with_not_enough_capacity)
{
  vector_type<int> input {1, 2, 3};
  auto exp = input;

  test_type<int> out {input.data(), input.size()};

  const auto out_it = out.insert(out.begin(), 6, 2);
  const auto exp_it = exp.insert(exp.begin(), 6, 2);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), out.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(insert_pos_value_count_as_vector)
{
  vector_type<int> input {1, 2, 3};
  auto exp = input;

  test_type<int> out {input};

  const auto out_it = out.insert(out.begin(), 6, 2);
  const auto exp_it = exp.insert(exp.begin(), 6, 2);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(insert_pos_first_last_as_span_with_enough_capacity)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  const vector_type<int> in_inserted {6, 7};

  test_type<int> out {input.data(), input.size()};

  exp.resize(input.size() - in_inserted.size());
  out.resize(input.size() - in_inserted.size());

  const auto out_it =
      out.insert(out.end(), in_inserted.begin(), in_inserted.end());
  const auto exp_it =
      exp.insert(exp.end(), in_inserted.begin(), in_inserted.end());

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(insert_pos_first_last_as_span_with_not_enough_capacity)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  const vector_type<int> in_inserted {6, 7};

  test_type<int> out {input.data(), input.size()};

  const auto exp_it =
      exp.insert(exp.end(), in_inserted.begin(), in_inserted.end());
  const auto out_it =
      out.insert(out.end(), in_inserted.begin(), in_inserted.end());

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), out.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(insert_pos_first_last_as_vector)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  const vector_type<int> in_inserted {6, 7};

  test_type<int> out {input};

  const auto exp_it =
      exp.insert(exp.end(), in_inserted.begin(), in_inserted.end());
  const auto out_it =
      out.insert(out.end(), in_inserted.begin(), in_inserted.end());

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(insert_pos_ilist_as_span_with_enough_capacity)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  const auto in_inserted = {6, 7};

  test_type<int> out {input.data(), input.size()};

  exp.resize(input.size() - in_inserted.size());
  out.resize(input.size() - in_inserted.size());

  const auto out_it = out.insert(out.end(), in_inserted);
  const auto exp_it = exp.insert(exp.end(), in_inserted);

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(insert_pos_ilist_as_span_with_not_enough_capacity)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  const auto in_inserted = {6, 7};

  test_type<int> out {input.data(), input.size()};

  const auto out_it = out.insert(out.end(), in_inserted);
  const auto exp_it = exp.insert(exp.end(), in_inserted);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), out.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(insert_pos_ilist_as_span_empty_ilist)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  const std::initializer_list<int> in_inserted = {};

  test_type<int> out {input.data(), input.size()};

  const auto out_it = out.insert(out.end(), in_inserted);
  const auto exp_it = exp.insert(exp.end(), in_inserted);

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), out.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(insert_ilist_as_vector)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  const auto in_inserted = {6, 7};

  test_type<int> out {input};

  const auto out_it = out.insert(out.end(), in_inserted);
  const auto exp_it = exp.insert(exp.end(), in_inserted);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(emplace_as_span_with_enough_capacity)
{
  vector_type<std::string> input {"1", "2", "3", "4", "5"};
  auto exp = input;

  const std::size_t n_emplaced = 2;

  test_type<std::string> out {input.data(), input.size()};

  exp.resize(input.size() - n_emplaced - 1);
  out.resize(input.size() - n_emplaced - 1);

  const auto out_it = out.emplace(out.begin() + 1, 3u, 'x');
  const auto exp_it = exp.emplace(exp.begin() + 1, 3u, 'x');

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(emplace_as_span_with_not_enough_capacity)
{
  vector_type<std::string> input {"1", "2", "3", "4", "5"};
  auto exp = input;

  test_type<std::string> out {input.data(), input.size()};

  const auto out_it = out.emplace(out.begin() + 1, 3u, 'x');
  const auto exp_it = exp.emplace(exp.begin() + 1, 3u, 'x');

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), out.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(emplace_as_vector)
{
  vector_type<std::string> input {"1", "2", "3", "4", "5"};
  auto exp = input;

  test_type<std::string> out {input};

  const auto out_it = out.emplace(out.begin() + 1, 3u, 'x');
  const auto exp_it = exp.emplace(exp.begin() + 1, 3u, 'x');

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(erase_pos_as_span)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  test_type<int> out {input.data(), input.size()};

  auto out_it = out.erase(out.begin());
  auto exp_it = exp.erase(exp.begin());

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());

  out_it = out.erase(out.end() - 1);
  exp_it = exp.erase(exp.end() - 1);

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());

  out_it = out.erase(out.begin() + 2);
  exp_it = exp.erase(exp.begin() + 2);

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(erase_pos_as_vector)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  test_type<int> out {input};

  auto out_it = out.erase(out.begin());
  auto exp_it = exp.erase(exp.begin());

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());

  out_it = out.erase(out.end() - 1);
  exp_it = exp.erase(exp.end() - 1);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());

  out_it = out.erase(out.begin() + 2);
  exp_it = exp.erase(exp.begin() + 2);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(erase_first_last_as_span)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  test_type<int> out {input.data(), input.size()};

  auto out_it = out.erase(out.begin(), out.begin() + 1);
  auto exp_it = exp.erase(exp.begin(), exp.begin() + 1);

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());

  out_it = out.erase(out.end() - 1, out.end());
  exp_it = exp.erase(exp.end() - 1, exp.end());

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());

  out_it = out.erase(out.begin(), out.begin());
  exp_it = exp.erase(exp.begin(), exp.begin());

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());

  out_it = out.erase(out.begin(), out.end());
  exp_it = exp.erase(exp.begin(), exp.end());

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(erase_first_last_as_vector)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  test_type<int> out {input};

  auto out_it = out.erase(out.begin(), out.begin() + 1);
  auto exp_it = exp.erase(exp.begin(), exp.begin() + 1);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());

  out_it = out.erase(out.end() - 1, out.end());
  exp_it = exp.erase(exp.end() - 1, exp.end());

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());

  out_it = out.erase(out.begin(), out.begin());
  exp_it = exp.erase(exp.begin(), exp.begin());

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());

  out_it = out.erase(out.begin(), out.end());
  exp_it = exp.erase(exp.begin(), exp.end());

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
  BOOST_CHECK_EQUAL(out_it - out.begin(), exp_it - exp.begin());
}

BOOST_AUTO_TEST_CASE(push_back_as_span_with_enough_capacity)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  const int in_inserted {6};

  test_type<int> out {input.data(), input.size()};

  exp.resize(input.size() - 1);
  out.resize(input.size() - 1);

  out.push_back(in_inserted);
  exp.push_back(in_inserted);

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
}

BOOST_AUTO_TEST_CASE(push_back_as_span_with_not_enough_capacity)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  const int in_inserted {6};

  test_type<int> out {input.data(), input.size()};

  out.push_back(in_inserted);
  exp.push_back(in_inserted);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), out.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
}

BOOST_AUTO_TEST_CASE(push_back_as_vector)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  const int in_inserted {6};

  test_type<int> out {input};

  out.push_back(in_inserted);
  exp.push_back(in_inserted);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
}

BOOST_AUTO_TEST_CASE(push_back_rvalue_as_span_with_enough_capacity)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  test_type<int> out {input.data(), input.size()};

  exp.resize(input.size() - 1);
  out.resize(input.size() - 1);

  out.push_back(6);
  exp.push_back(6);

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
}

BOOST_AUTO_TEST_CASE(push_back_rvalue_as_span_with_not_enough_capacity)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  test_type<int> out {input.data(), input.size()};

  out.push_back(6);
  exp.push_back(6);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), out.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
}

BOOST_AUTO_TEST_CASE(push_back_rvalue_as_vector)
{
  vector_type<int> input {1, 2, 3, 4, 5};
  auto exp = input;

  test_type<int> out {input};

  out.push_back(6);
  exp.push_back(6);

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
}

BOOST_AUTO_TEST_CASE(emplace_back_as_span_with_enough_capacity)
{
  vector_type<std::string> input {"1", "2", "3", "4", "5"};
  auto exp = input;

  test_type<std::string> out {input.data(), input.size()};

  exp.resize(input.size() - 1);
  out.resize(input.size() - 1);

  out.emplace_back(3u, 'x');
  exp.emplace_back(3u, 'x');

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
}

BOOST_AUTO_TEST_CASE(emplace_back_as_span_with_not_enough_capacity)
{
  vector_type<std::string> input {"1", "2", "3", "4", "5"};
  auto exp = input;

  test_type<std::string> out {input.data(), input.size()};

  out.emplace_back(3u, 'x');
  exp.emplace_back(3u, 'x');

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), out.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(), 1);
}

BOOST_AUTO_TEST_CASE(emplace_back_as_vector)
{
  vector_type<std::string> input {"1", "2", "3", "4", "5"};
  auto exp = input;

  test_type<std::string> out {input};

  out.emplace_back(3u, 'x');
  exp.emplace_back(3u, 'x');

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
}

BOOST_AUTO_TEST_CASE(pop_back_as_span)
{
  vector_type<int> input {1, 2, 3};
  auto exp = input;

  test_type<int> out {input.data(), input.size()};

  out.pop_back();
  exp.pop_back();

  BOOST_CHECK(out.is_span());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
}

BOOST_AUTO_TEST_CASE(pop_back_as_vector)
{
  vector_type<int> input {1, 2, 3};
  auto exp = input;

  test_type<int> out {input};

  out.pop_back();
  exp.pop_back();

  BOOST_CHECK(out.is_vector());
  BOOST_CHECK_EQUAL(out.capacity(), exp.capacity());
  BOOST_CHECK_EQUAL_COLLECTIONS(out.begin(), out.end(), exp.begin(), exp.end());
  BOOST_CHECK_EQUAL(out.get_allocator().n_allocations(),
                    exp.get_allocator().n_allocations());
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace modifiers

// NOLINTEND(bugprone-use-after-move, hicpp-invalid-access-moved,
// clang-diagnostic-self-assign-overloaded,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

}  // namespace span_or_vector
