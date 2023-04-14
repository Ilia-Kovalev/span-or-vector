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

template<class T, class Allocator = std::allocator<T>>
class span_or_vector_base
    : private std::vector<T, Allocator>
    , private span<T>

{
  using span_type = span<T>;

protected:
  using vector_type = std::vector<T, Allocator>;

public:
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

  auto operator=(const span_or_vector_base& other) -> span_or_vector_base&
  {
    if (this == &other) {
      return *this;
    }

    is_span_ = false;
    if (other.is_span()) {
      modify_as_vector(
          [&](vector_type& vec)
          {
            vec = static_cast<const vector_type&>(other);
            vec.reserve(other.capacity());
            vec.assign(other.begin(), other.end());
          });
    } else {
      modify_as_vector([&](vector_type& vec)
                       { vec = static_cast<const vector_type&>(other); });
    }

    return *this;
  }

  auto operator=(span_or_vector_base&& other) noexcept -> span_or_vector_base&
  {
    if (this == &other) {
      return *this;
    }

    if (other.is_span()) {
      vector_type::operator=(std::move(static_cast<vector_type&&>(other)));
      span_type::operator=(std::move(static_cast<span_type&&>(other)));
      is_span_ = true;
      span_capacity_ = other.span_capacity_;
    } else {
      is_span_ = false;
      modify_as_vector([&](vector_type& vec)
                       { vec = std::move(static_cast<vector_type&&>(other)); });
    }

    return *this;
  }

  auto operator=(const vector_type& other) -> span_or_vector_base&
  {
    modify_as_vector([&](vector_type& vec) { vec = other; });
    return *this;
  }

  auto operator=(vector_type&& other) -> span_or_vector_base&
  {
    modify_as_vector([&](vector_type& vec) { vec = std::move(other); });
    return *this;
  }

  auto operator=(std::initializer_list<T> ilist) -> span_or_vector_base&
  {
    assign(ilist);
    return *this;
  }

  void reserve(size_type new_cap)
  {
    if (is_vector()) {
      modify_as_vector([&](vector_type& vec) { vec.reserve(new_cap); });
      return;
    }

    if (new_cap <= span_capacity_) {
      span_capacity_ = new_cap;
      return;
    }

    is_span_ = false;
    modify_as_vector(
        [&](vector_type& vec)
        {
          vec.reserve(new_cap);
          vec.assign(begin(), end());
        });
  }

  auto capacity() const noexcept -> size_type
  {
    return is_span() ? span_capacity_ : vector_type::capacity();
  }

  void shrink_to_fit()
  {
    if (is_span()) {
      span_capacity_ = size();
    } else {
      modify_as_vector([&](vector_type& vec) { vec.shrink_to_fit(); });
    }
  }

  void clear()
  {
    if (is_span()) {
      span_type::operator=(span_type::first(0));
    } else {
      modify_as_vector([&](vector_type& vec) { vec.clear(); });
    }
  }

  void resize(size_type count)
  {
    if (is_vector()) {
      modify_as_vector([&](vector_type& vec) { vec.resize(count); });
      return;
    }

    if (count <= span_capacity_) {
      span_type::operator=({data(), count});
      return;
    }

    switch_to_vector(count);
    resize(count);
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
    resize(count, value);
  }

  void swap(span_or_vector_base& other) { std::swap(*this, other); }

  auto is_span() const noexcept -> bool { return is_span_; }

  auto is_vector() const noexcept -> bool { return !is_span_; }

  span_or_vector_base() = default;

  span_or_vector_base(const span_or_vector_base& other) { *this = other; }
  span_or_vector_base(span_or_vector_base&& other) noexcept
  {
    *this = std::move(other);
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

protected:
  ~span_or_vector_base() = default;

  void update_span()
  {
    assert(is_vector());
    span_type::operator=({vector_type::data(), vector_type::size()});
  }

  template<class F,
           typename = typename std::enable_if<std::is_void<
               typename std::result_of<F(vector_type&)>::type>::value>::type>
  void modify_as_vector(F&& operation)
  {
    assert(is_vector());

    operation(static_cast<vector_type&>(*this));
    update_span();
  }

  template<class F,
           typename = typename std::enable_if<!std::is_void<
               typename std::result_of<F(vector_type&)>::type>::value>::type>
  auto modify_as_vector(F&& operation) ->
      typename std::result_of<F(vector_type&)>::type
  {
    assert(is_vector());

    const auto result = operation(static_cast<vector_type&>(*this));
    update_span();
    return result;
  }

  void switch_to_vector(size_type capacity = 0)
  {
    switch_to_empty_vector(capacity);
    modify_as_vector([&](vector_type& vec) { vec.assign(begin(), end()); });
  }

  void switch_to_empty_vector(size_type capacity = 0)
  {
    is_span_ = false;
    modify_as_vector([&](vector_type& vec) { vec.reserve(capacity); });
  }

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
      return modify_as_vector([&](vector_type& vec) -> vector_type
                              { return std::move(vec); });
    }
    return {begin(), end()};
  }

private:
  bool is_span_ = false;
  size_type span_capacity_ = 0;
};

}  // namespace detail

template<class T, class Allocator = std::allocator<T>>
class span_or_vector : public detail::span_or_vector_base<T, Allocator>
{
  using base = detail::span_or_vector_base<T, Allocator>;

  using vector_type = typename base::vector_type;

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

  using base::base;

  auto at(size_type pos) const -> const_reference
  {
    check_out_of_range(pos);
    return this->operator[](pos);
  }

  auto at(size_type pos) -> reference
  {
    check_out_of_range(pos);
    return this->operator[](pos);
  }

  void assign(size_type count, const T& value)
  {
    this->resize(count);
    std::fill_n(this->begin(), count, value);
  }

  template<class InputIt>
  void assign(InputIt first, InputIt last)
  {
    resize(std::distance(first, last));
    std::copy(first, last, this->begin());
  }

  void assign(std::initializer_list<T> ilist)
  {
    assign(ilist.begin(), ilist.end());
  }

  auto insert(const_iterator pos, const T& value) -> iterator
  {
    return insert_impl(pos, value);
  }

  auto insert(const_iterator pos, T&& value) -> iterator
  {
    return insert_impl(pos, std::move(value));
  }

  auto insert(const_iterator pos, size_type count, const T& value) -> iterator
  {
    if (count == 0) {
      return pos;
    }

    if (this->is_vector()) {
      return modify_as_vector([&](vector_type& vec) -> iterator
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
    const auto count = std::distance(first, last);

    if (count == 0) {
      return pos;
    }

    if (this->is_vector()) {
      return modify_as_vector([&](vector_type& vec) -> iterator
                              { return vec.insert(pos, first, last); });
    }

    return insert_into_span(pos,
                            count,
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
    const auto count = sizeof...(args);

    if (count == 0) {
      return pos;
    }

    if (this->is_vector()) {
      return modify_as_vector(
          [&](vector_type& vec) -> iterator
          { return vec.emplace(pos, std::forward<Args>(args)...); });
    }

    return insert_into_span(
        pos,
        count,
        [&](iterator iter) -> iterator
        { return detail::forward_into(iter, std::forward<Args>(args)...); });
  }

  auto erase(const_iterator pos) -> iterator
  {
    if (pos == this->end()) {
      return this->end();
    }

    return erase(pos, pos + 1);
  }

  auto erase(const_iterator first, const_iterator last) -> iterator
  {
    if (this->is_vector()) {
      return modify_as_vector([&](vector_type& vec) -> iterator
                              { return vec.erase(first, last); });
    }

    const auto out_it = std::move(last, this->end(), first);
    resize(this->size() - std::distance(first, last));
    return first;
  }

  void push_back(const T& value) { push_back_impl(value); }
  void push_back(T&& value) { push_back_impl(std::move(value)); }

  template<class... Args>
  void emplace_back(Args&&... args)
  {
    emplace(this->end(), std::forward<Args>(args)...);
  }

  void pop_back()
  {
    if (this->is_vector()) {
      modify_as_vector([](vector_type& vec) { vec.pop_back(); });
    } else {
      resize(this->size() - 1);
    }
  }

private:
  template<typename U>
  auto insert_impl(const_iterator pos, U&& value) -> iterator
  {
    if (this->is_vector()) {
      return modify_as_vector([&](vector_type& vec)
                              { vec.insert(pos, std::forward<U>(value)); });
    }

    return insert_into_span(pos,
                            1,
                            [&](iterator iter) -> iterator
                            { return *iter++ = std::forward<U>(value); });
  }

  template<typename U>
  void push_back_impl(U&& value)
  {
    if (this->is_vector()) {
      modify_as_vector([&](vector_type& vec)
                       { vec.push_back(std::forward<U>(vec)); });
      return;
    }

    if (this->size() < this->capacity()) {
      resize(this->size() + 1);
      this->back() = std::forward<U>(value);
      return;
    }

    switch_to_vector(this->size() + 1);
    push_back(std::forward<U>(value));
  }

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
    assert(count > 0);

    const auto new_size = this->size() + count;
    if (new_size <= this->capacity()) {
      this->resize(new_size);
      std::move_backward(pos, pos + count, pos + count);
      auto out_it = fill(pos);
      assert(out_it == pos + count);
      return pos;
    }

    switch_to_empty_vector(new_size);
    return modify_as_vector(
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

  void check_out_of_range(size_type pos) const
  {
    if (pos >= this->size()) {
      throw std::out_of_range(std::string("span_or_vector::at : Position ")
                              + std::to_string(pos) + " is out of range [0, "
                              + this->size() + ")");
    }
  }
};

template<class T, class Alloc>
auto operator==(const span_or_vector<T, Alloc>& lhs,
                const span_or_vector<T, Alloc>& rhs) -> bool
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

template<class T, class Alloc>
auto operator!=(const span_or_vector<T, Alloc>& lhs,
                const span_or_vector<T, Alloc>& rhs) -> bool
{
  return !(lhs == rhs);
}

template<class T, class Alloc>
auto operator<(const std::vector<T, Alloc>& lhs,
               const std::vector<T, Alloc>& rhs) -> bool
{
  return std::lexicographical_compare(
      lhs.begin(), lhs.end(), rhs.begin(), lhs.end());
}

template<class T, class Alloc>
auto operator>(const std::vector<T, Alloc>& lhs,
               const std::vector<T, Alloc>& rhs) -> bool
{
  return std::lexicographical_compare(
      rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

template<class T, class Alloc>
auto operator>=(const std::vector<T, Alloc>& lhs,
                const std::vector<T, Alloc>& rhs) -> bool
{
  return !(lhs < rhs);
}

template<class T, class Alloc>
auto operator<=(const std::vector<T, Alloc>& lhs,
                const std::vector<T, Alloc>& rhs) -> bool
{
  return !(lhs > rhs);
}

}  // namespace span_or_vector
