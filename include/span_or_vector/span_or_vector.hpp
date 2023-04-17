/*
 * MIT License
 *
 * Copyright (c) 2023 Ilia Kovalev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
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

#pragma once

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef __cpp_lib_span
#  include <span>
namespace span_or_vector
{
namespace detail
{
template<typename T>
using span_type = std::span<T>;
}
}  // namespace span_or_vector
#else
#  include <boost/core/span.hpp>
namespace span_or_vector
{
namespace detail
{
template<class T>
using span = boost::span<T>;
}  // namespace detail
}  // namespace span_or_vector
#endif

namespace span_or_vector
{
namespace detail
{
template<class Iter, class H, class... Tail>
auto forward_into(Iter iter, H&& head, Tail&&... tail) -> Iter
{
  *iter++ = std::forward<H>(head);
  return forward_into(iter, std::forward<Tail>(tail)...);
}

template<class Iter>
auto forward_into(Iter iter) -> Iter
{
  return iter;
}

template<class T, class Allocator>
class span_or_vector_base
    : private std::vector<T, Allocator>
    , private span<T>

{
  using span_type = span<T>;

public:
  using vector_type = std::vector<T, Allocator>;

  using value_type = typename span_type::value_type;
  using allocator_type = typename vector_type::allocator_type;
  using size_type = typename span_type::size_type;
  using difference_type = typename span_type::difference_type;
  using reference = typename span_type::reference;
  using const_reference = typename span_type::const_reference;
  using pointer = typename span_type::pointer;
  using const_pointer = typename span_type::const_pointer;
  using iterator = typename span_type::iterator;
  using const_iterator = typename span_type::const_iterator;
  using reverse_iterator = typename span_type::reverse_iterator;
  using const_reverse_iterator = typename span_type::const_reverse_iterator;

  using span_type::size;
  using span_type::operator[];
  using span_type::back;
  using span_type::begin;
  using span_type::cbegin;
  using span_type::cend;
  using span_type::crbegin;
  using span_type::crend;
  using span_type::data;
  using span_type::empty;
  using span_type::end;
  using span_type::front;
  using span_type::rbegin;
  using span_type::rend;

  using vector_type::get_allocator;
  using vector_type::max_size;

  span_or_vector_base() = default;
  span_or_vector_base(const span_or_vector_base& other)
      : vector_type()
      , span_type()
  {
    *this = other;
  }

  span_or_vector_base(span_or_vector_base&& other) noexcept
      : vector_type()
      , span_type()
  {
    *this = std::move(other);
  }
  ~span_or_vector_base() = default;

  auto operator=(const span_or_vector_base& other) -> span_or_vector_base&
  {
    if (this == &other) {
      return *this;
    }

    if (is_span()) {
      // copy allocator via vector assignment operator
      const vector_type vec_copy {other.get_allocator()};
      vector_type::operator=(vec_copy);

      resize(other.size());
      std::copy(other.begin(), other.end(), begin());
      return *this;
    }

    modify_as_vector(
        [&](vector_type& vec)
        {
          vec = static_cast<const vector_type&>(other);
          if (other.is_span()) {
            vec.assign(other.begin(), other.end());
          }
        });

    return *this;
  }

  auto operator=(span_or_vector_base&& other) noexcept -> span_or_vector_base&
  {
    vector_type::operator=(std::move(static_cast<vector_type&&>(other)));

    if (other.is_span()) {
      span_type::operator=(other);
      span_capacity_ = other.span_capacity_;
      is_span_ = true;
    } else {
      update_span();
    }

    other.update_span();

    return *this;
  }

  span_or_vector_base(T* first, size_type count, const Allocator& alloc = {})
      : vector_type(alloc)
      , span_type(first, count)
      , is_span_(true)
      , span_capacity_(count)
  {
  }

  explicit span_or_vector_base(const Allocator& alloc)
      : vector_type(alloc)
  {
  }

  explicit span_or_vector_base(size_type count,
                               const T& value = T(),
                               const Allocator& alloc = Allocator())
      : vector_type(count, value, alloc)
      , span_type(vector_type::data(), vector_type::size())
  {
  }

  template<class InputIt>
  span_or_vector_base(InputIt first,
                      InputIt last,
                      const Allocator& alloc = Allocator())
      : vector_type(first, last, alloc)
      , span_type(vector_type::data(), vector_type::size())
  {
  }

  explicit span_or_vector_base(const vector_type& other)
      : vector_type(other)
      , span_type(vector_type::data(), vector_type::size())
  {
  }

  span_or_vector_base(const vector_type& other, const Allocator& alloc)
      : vector_type(other, alloc)
      , span_type(vector_type::data(), vector_type::size())
  {
  }

  explicit span_or_vector_base(vector_type&& other)
      : vector_type(std::move(other))
      , span_type(vector_type::data(), vector_type::size())
  {
  }

  span_or_vector_base(vector_type&& other, const Allocator& alloc)
      : vector_type(std::move(other), alloc)
      , span_type(vector_type::data(), vector_type::size())
  {
  }

  span_or_vector_base(std::initializer_list<T> init,
                      const Allocator& alloc = Allocator())
      : vector_type(init, alloc)
      , span_type(vector_type::data(), vector_type::size())
  {
  }

  void reserve(size_type new_cap)
  {
    if (is_vector()) {
      modify_as_vector([&](vector_type& vec) { vec.reserve(new_cap); });
      return;
    }

    if (new_cap <= span_capacity_) {
      return;
    }

    switch_to_vector(new_cap);
  }

  auto capacity() const noexcept -> size_type
  {
    return is_span() ? span_capacity_ : vector_type::capacity();
  }

  void shrink_to_fit()
  {
    if (is_vector()) {
      modify_as_vector([&](vector_type& vec) { vec.shrink_to_fit(); });
    } else {
      span_capacity_ = size();
    }
  }

  void resize(size_type count)
  {
    if (is_vector()) {
      modify_as_vector([&](vector_type& vec) { vec.resize(count); });
      return;
    }

    if (count <= span_capacity_) {
      span_type::operator=(span_type::first(count));
      return;
    }

    switch_to_vector(count);
    modify_as_vector([&](vector_type& vec) { vec.resize(count); });
  }

  void resize(size_type count, const value_type& value)
  {
    if (is_vector()) {
      modify_as_vector([&](vector_type& vec) { vec.resize(count, value); });
      return;
    }

    if (count <= span_capacity_) {
      const auto old_end = end();
      span_type::operator=({data(), count});
      std::fill(old_end, end(), value);
      return;
    }

    switch_to_vector(count);
    modify_as_vector([&](vector_type& vec) { vec.resize(count, value); });
  }

  void swap(span_or_vector_base& other) { std::swap(*this, other); }

  auto is_span() const noexcept -> bool { return is_span_; }

  auto is_vector() const noexcept -> bool { return !is_span_; }

  explicit operator span_type() const& { return *this; }
  explicit operator vector_type() const&
  {
    if (is_vector()) {
      return *this;
    }
    return {begin(), end()};
  }

  explicit operator vector_type() &&
  {
    vector_type result;

    if (is_vector()) {
      return modify_as_vector_with_return([&](vector_type& vec) -> vector_type
                                          { return std::move(vec); });
    }
    return {begin(), end()};
  }

  template<class F>
  void modify_as_vector(F&& operation)
  {
    operation(*this);
    update_span();
  }

  template<class F>
  auto modify_as_vector_with_return(F&& operation) ->
      typename std::result_of<F(vector_type&)>::type
  {
    const auto result = operation(*this);
    update_span();
    return result;
  }

  void switch_to_vector(size_type capacity)
  {
    assert(is_span());
    modify_as_vector(
        [&](vector_type& vec)
        {
          vec.reserve(capacity);
          vec.assign(begin(), end());
        });
  }

  void switch_to_vector() { switch_to_vector(span_capacity_); }

  void switch_to_empty_vector(size_type capacity)
  {
    assert(is_span());
    modify_as_vector([&](vector_type& vec) { vec.reserve(capacity); });
  }

  void switch_to_empty_vector() { switch_to_empty_vector(span_capacity_); }

private:
  void update_span()
  {
    is_span_ = false;
    span_type::operator=({vector_type::data(), vector_type::size()});
  }

  bool is_span_ = false;
  size_type span_capacity_ = 0;
};

template<class T, class Allocator>
auto operator==(const span_or_vector_base<T, Allocator>& lhs,
                const span_or_vector_base<T, Allocator>& rhs) -> bool
{
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }
  return true;
}

template<class T, class Allocator>
auto operator!=(const span_or_vector_base<T, Allocator>& lhs,
                const span_or_vector_base<T, Allocator>& rhs) -> bool
{
  return !(lhs == rhs);
}

template<class T, class Allocator>
auto operator<(const span_or_vector_base<T, Allocator>& lhs,
               const span_or_vector_base<T, Allocator>& rhs) -> bool
{
  return std::lexicographical_compare(
      lhs.begin(), lhs.end(), rhs.begin(), lhs.end());
}

template<class T, class Allocator>
auto operator>(const span_or_vector_base<T, Allocator>& lhs,
               const span_or_vector_base<T, Allocator>& rhs) -> bool
{
  return std::lexicographical_compare(
      rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

template<class T, class Allocator>
auto operator>=(const span_or_vector_base<T, Allocator>& lhs,
                const span_or_vector_base<T, Allocator>& rhs) -> bool
{
  return !(lhs < rhs);
}

template<class T, class Allocator>
auto operator<=(const span_or_vector_base<T, Allocator>& lhs,
                const span_or_vector_base<T, Allocator>& rhs) -> bool
{
  return !(lhs > rhs);
}

template<class T, class Allocator>
class span_or_vector_assignments
    : virtual protected span_or_vector_base<T, Allocator>
{
  using base = span_or_vector_base<T, Allocator>;

public:
  using vector_type = typename base::vector_type;
  using size_type = typename base::size_type;

  span_or_vector_assignments() = default;
  ~span_or_vector_assignments() = default;
  span_or_vector_assignments(const span_or_vector_assignments&) = default;
  span_or_vector_assignments(span_or_vector_assignments&&) noexcept = default;
  auto operator=(const span_or_vector_assignments&)
      -> span_or_vector_assignments& = default;
  auto operator=(span_or_vector_assignments&&) noexcept
      -> span_or_vector_assignments& = delete;

  auto operator=(const vector_type& other) -> span_or_vector_assignments&
  {
    modify_as_vector([&](vector_type& vec) { vec = other; });
    return *this;
  }

  auto operator=(vector_type&& other) -> span_or_vector_assignments&
  {
    modify_as_vector([&](vector_type& vec) { vec = std::move(other); });
    return *this;
  }

  auto operator=(std::initializer_list<T> ilist) -> span_or_vector_assignments&
  {
    assign(ilist);
    return *this;
  }

  void assign(size_type count, const T& value)
  {
    this->resize(count, value);
    std::fill_n(this->begin(), count, value);
  }

  template<class InputIt>
  void assign(InputIt first, InputIt last)
  {
    this->resize(std::distance(first, last));
    std::copy(first, last, this->begin());
  }

  void assign(std::initializer_list<T> ilist)
  {
    assign(ilist.begin(), ilist.end());
  }
};

template<class T, class Allocator>
class span_or_vector_element_access
    : virtual protected span_or_vector_base<T, Allocator>
{
  using base = span_or_vector_base<T, Allocator>;

public:
  using size_type = typename base::size_type;
  using reference = typename base::reference;
  using const_reference = typename base::const_reference;

  span_or_vector_element_access() = default;
  ~span_or_vector_element_access() = default;
  span_or_vector_element_access(const span_or_vector_element_access&) = default;
  span_or_vector_element_access(span_or_vector_element_access&&) noexcept =
      default;
  auto operator=(const span_or_vector_element_access&)
      -> span_or_vector_element_access& = default;
  auto operator=(span_or_vector_element_access&&) noexcept
      -> span_or_vector_element_access& = delete;

  auto at(size_type pos) const -> const_reference
  {
    check_out_of_range(pos);
    return operator[](pos);
  }

  auto at(size_type pos) -> reference
  {
    check_out_of_range(pos);
    return operator[](pos);
  }

  using base::begin;
  using base::end;
  using base::operator[];
  using base::back;
  using base::cbegin;
  using base::cend;
  using base::crbegin;
  using base::crend;
  using base::data;
  using base::front;
  using base::rbegin;
  using base::rend;

private:
  void check_out_of_range(size_type pos) const
  {
    if (pos >= this->size()) {
      throw std::out_of_range(std::string("span_or_vector::at : Position ")
                              + std::to_string(pos) + " is out of range [0, "
                              + this->size() + ")");
    }
  }
};

template<class T, class Allocator>
class span_or_vector_modifiers
    : virtual protected span_or_vector_base<T, Allocator>
{
  using base = span_or_vector_base<T, Allocator>;

public:
  using vector_type = typename base::vector_type;
  using size_type = typename base::size_type;
  using iterator = typename base::iterator;
  using const_iterator = typename base::const_iterator;

  span_or_vector_modifiers() = default;
  ~span_or_vector_modifiers() = default;
  span_or_vector_modifiers(const span_or_vector_modifiers&) = default;
  span_or_vector_modifiers(span_or_vector_modifiers&&) noexcept = default;
  auto operator=(const span_or_vector_modifiers&)
      -> span_or_vector_modifiers& = default;
  auto operator=(span_or_vector_modifiers&&) noexcept
      -> span_or_vector_modifiers& = delete;

  auto insert(const_iterator pos, const T& value) -> iterator
  {
    return insert(pos, 1, value);
  }

  auto insert(const_iterator pos, T&& value) -> iterator
  {
    return emplace(pos, std::move(value));
  }

  auto insert(const_iterator pos, size_type count, const T& value) -> iterator
  {
    if (this->is_vector()) {
      return modify_as_vector_with_return(
          [&](vector_type& vec) -> iterator
          { return vec.insert(pos, count, value); });
    }

    return insert_into_span(pos,
                            count,
                            [&](iterator iter) -> iterator
                            { return std::fill_n(iter, count, value); });
  }

  template<class InputIt>
  auto insert(const_iterator pos, InputIt first, InputIt last) -> iterator
  {
    if (this->is_vector()) {
      return modify_as_vector_with_return(
          [&](vector_type& vec) -> iterator
          { return vec.insert(pos, first, last); });
    }

    return insert_into_span(pos,
                            std::distance(first, last),
                            [&](iterator iter) -> iterator
                            { return std::copy(first, last, iter); });
  }

  auto insert(const_iterator pos, std::initializer_list<T> ilist) -> iterator
  {
    return insert(pos, ilist.begin(), ilist.end());
  }

  template<class... Args>
  auto emplace(const_iterator pos, Args&&... args) -> iterator
  {
    if (this->is_vector()) {
      return modify_as_vector_with_return(
          [&](vector_type& vec) -> iterator
          { return vec.emplace(pos, std::forward<Args>(args)...); });
    }

    return insert_into_span(
        pos,
        sizeof...(args),
        [&](iterator iter) -> iterator
        { return detail::forward_into(iter, std::forward<Args>(args)...); });
  }

  auto erase(const_iterator pos) -> iterator { return erase(pos, pos + 1); }

  auto erase(const_iterator first, const_iterator last) -> iterator
  {
    if (this->is_vector()) {
      return modify_as_vector_with_return([&](vector_type& vec) -> iterator
                                          { return vec.erase(first, last); });
    }

    std::move(last, this->end(), first);
    this->resize(this->size() - std::distance(first, last));
    return first;
  }

  void push_back(const T& value) { insert(this->end(), value); }
  void push_back(T&& value) { insert(this->end(), std::move(value)); }

  template<class... Args>
  void emplace_back(Args&&... args)
  {
    emplace(this->end(), std::forward<Args>(args)...);
  }

  void pop_back() { this->resize(this->size() - 1); }

  void clear() { this->resize(0); }

private:
  template<class FillInsertedValues>
  auto insert_into_span(const_iterator pos,
                        size_type count,
                        FillInsertedValues&& fill) -> iterator
  {
    static_assert(
        std::is_same<
            typename std::result_of<FillInsertedValues(const_iterator)>::type,
            iterator>::value,
        "Wrong callable type");

    assert(this->is_span());

    const auto new_size = this->size() + count;
    if (new_size <= this->capacity()) {
      this->resize(new_size);
      std::move_backward(pos, pos + count, pos + count);
      auto out_it = fill(pos);
      assert(out_it == pos + count);
      return pos;
    }

    return modify_as_vector_with_return(
        [&](vector_type& vec) -> iterator
        {
          vec.resize(new_size);
          auto out_it = std::copy(this->begin(), pos, vec.begin());
          auto const result = out_it;
          out_it = fill(out_it);
          assert(out_it == result + count);
          std::copy(pos, this->end(), out_it);
          return result;
        });
  }
};

}  // namespace detail

template<class T, class Allocator = std::allocator<T>>
class span_or_vector
    : virtual private detail::span_or_vector_base<T, Allocator>
    , private detail::span_or_vector_element_access<T, Allocator>
    , private detail::span_or_vector_modifiers<T, Allocator>
    , private detail::span_or_vector_assignments<T, Allocator>
{
  using base = detail::span_or_vector_base<T, Allocator>;
  using element_access = detail::span_or_vector_element_access<T, Allocator>;
  using modifiers = detail::span_or_vector_modifiers<T, Allocator>;
  using assignments = detail::span_or_vector_assignments<T, Allocator>;

public:
  using value_type = typename base::value_type;
  using allocator_type = typename base::allocator_type;
  using size_type = typename base::size_type;
  using difference_type = typename base::difference_type;
  using reference = typename base::reference;
  using const_reference = typename base::const_reference;
  using pointer = typename base::pointer;
  using const_pointer = typename base::const_pointer;
  using iterator = typename base::iterator;
  using const_iterator = typename base::const_iterator;
  using reverse_iterator = typename base::reverse_iterator;
  using const_reverse_iterator = typename base::const_reverse_iterator;

  using vector_type = typename base::vector_type;

  using base::base;

  span_or_vector() = default;
  ~span_or_vector() = default;
  span_or_vector(const span_or_vector& other) = default;
  span_or_vector(span_or_vector&& other) noexcept = default;

  auto operator=(const span_or_vector& other) -> span_or_vector&
  {
    if (this == &other) {
      return *this;
    }
    base::operator=(other);
    return *this;
  }

  auto operator=(span_or_vector&& other) noexcept -> span_or_vector&
  {
    base::operator=(std::move(other));
    return *this;
  }

  using base::is_span;
  using base::is_vector;
  using base::operator vector_type;

  using base::capacity;
  using base::empty;
  using base::get_allocator;
  using base::max_size;
  using base::reserve;
  using base::resize;
  using base::shrink_to_fit;
  using base::size;
  using base::swap;

  using assignments::assign;
  using assignments::operator=;

  using element_access::at;
  using element_access::begin;
  using element_access::end;
  using element_access::operator[];
  using element_access::back;
  using element_access::cbegin;
  using element_access::cend;
  using element_access::crbegin;
  using element_access::crend;
  using element_access::data;
  using element_access::front;
  using element_access::rbegin;
  using element_access::rend;

  using modifiers::clear;
  using modifiers::emplace;
  using modifiers::emplace_back;
  using modifiers::erase;
  using modifiers::insert;
  using modifiers::pop_back;
  using modifiers::push_back;
};
}  // namespace span_or_vector
