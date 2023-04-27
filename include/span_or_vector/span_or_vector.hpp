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

#include <boost/core/span.hpp>

namespace span_or_vector
{
namespace detail
{

template<class T>
using span = boost::span<T>;

/**
 * @brief Base class that encapsulates operations with memory and enforces
 * object's invariant.
 */
template<class T, class Allocator>
class span_or_vector_base
    : private std::vector<T, Allocator>
    , private span<T>

{
  using span_type = span<T>;

public:
  using vector_type = std::vector<T, Allocator>;

  // Element access implemented via span, so use its type members instead of
  // std::vector's
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

  // Span methods that can be used without overriding
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

  // Vector methods that can be used without overriding
  using vector_type::get_allocator;
  using vector_type::max_size;

protected:  // Prohibit object creation and assignment
  span_or_vector_base() = default;

  /**
   * @brief Copy constructor that always copies data to comply with std::vector
   * behavior
   */
  span_or_vector_base(const span_or_vector_base& other)
      : vector_type()
      , span_type()
  {
    *this = other;
  }

  /**
   * @brief If `other` is span, then moves only span and makes `other` empty. If
   * `other` is vector, then moves it as std::vector
   */
  span_or_vector_base(span_or_vector_base&& other) noexcept
      : vector_type()
      , span_type()
  {
    *this = std::move(other);
  }
  ~span_or_vector_base() = default;

  /**
   * @brief Copy assignment that always copies data to comply with std::vector
   * behavior
   */
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

  /**
   * @brief If `other` is span, then moves only span and makes `other` empty. If
   * `other` is vector, then moves it as std::vector
   */
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

public:
  /**
   * @brief The only constructor different from std::vector. Initializes an
   * object as a span
   * @param first pointer to data
   * @param count number of elements used as capacity
   * @param alloc allocator used for memory allocation when switching to vector
   * state
   */
  span_or_vector_base(T* first, size_type count, const Allocator& alloc = {})
      : vector_type(alloc)
      , span_type(first, count)
      , is_span_(true)
      , span_capacity_(count)
  {
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  explicit span_or_vector_base(const Allocator& alloc)
      : vector_type(alloc)
  {
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  explicit span_or_vector_base(size_type count,
                               const T& value = T(),
                               const Allocator& alloc = Allocator())
      : vector_type(count, value, alloc)
      , span_type(vector_type::data(), vector_type::size())
  {
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  template<class InputIt,
           typename = std::enable_if<std::is_same<
               typename std::iterator_traits<InputIt>::iterator_category,
               std::input_iterator_tag>::value>>
  span_or_vector_base(InputIt first,
                      InputIt last,
                      const Allocator& alloc = Allocator())
      : vector_type(first, last, alloc)
      , span_type(vector_type::data(), vector_type::size())
  {
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  explicit span_or_vector_base(const vector_type& other)
      : vector_type(other)
      , span_type(vector_type::data(), vector_type::size())
  {
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  span_or_vector_base(const vector_type& other, const Allocator& alloc)
      : vector_type(other, alloc)
      , span_type(vector_type::data(), vector_type::size())
  {
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  explicit span_or_vector_base(vector_type&& other)
      : vector_type(std::move(other))
      , span_type(vector_type::data(), vector_type::size())
  {
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  span_or_vector_base(vector_type&& other, const Allocator& alloc)
      : vector_type(std::move(other), alloc)
      , span_type(vector_type::data(), vector_type::size())
  {
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  span_or_vector_base(std::initializer_list<T> init,
                      const Allocator& alloc = Allocator())
      : vector_type(init, alloc)
      , span_type(vector_type::data(), vector_type::size())
  {
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
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

  /**
   * @brief Replicates corresponding std::vector method
   */
  auto capacity() const noexcept -> size_type
  {
    return is_span() ? span_capacity_ : vector_type::capacity();
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  void shrink_to_fit()
  {
    if (is_vector()) {
      modify_as_vector([&](vector_type& vec) { vec.shrink_to_fit(); });
    } else {
      span_capacity_ = size();
    }
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
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

  /**
   * @brief Replicates corresponding std::vector method
   */
  void resize(size_type count, const value_type& value)
  {
    if (is_vector()) {
      modify_as_vector([&](vector_type& vec) { vec.resize(count, value); });
      return;
    }

    if (count <= size()) {
      span_type::operator=(span_type::first(count));
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

  /**
   * @brief Replicates corresponding std::vector method
   */
  void swap(span_or_vector_base& other) noexcept
  {
    vector_type::swap(static_cast<vector_type&>(other));
    std::swap(static_cast<span_type&>(*this), static_cast<span_type&>(other));
    std::swap(span_capacity_, other.span_capacity_);
    std::swap(is_span_, other.is_span_);
  }

  /**
   * @brief Indicates if object doesn't have its own memory
   */
  auto is_span() const noexcept -> bool { return is_span_; }

  /**
   * @brief Indicates if object has its own memory
   */
  auto is_vector() const noexcept -> bool { return !is_span_; }

  /**
   * @brief Converts object to std::vector with data copied
   */
  auto to_vector() const -> vector_type
  {
    if (is_vector()) {
      return static_cast<const vector_type&>(*this);
    }
    return {begin(), end()};
  }

  /**
   * @brief Converts object to std::vector with moving data to returned vector
   * is is_vector() is true, otherwise copies data from span.
   */
  auto move_to_vector() -> vector_type
  {
    vector_type result;

    if (is_vector()) {
      return modify_as_vector_with_return([&](vector_type& vec) -> vector_type
                                          { return std::move(vec); });
    }
    return {begin(), end()};
  }

protected:
  /**
   * @brief Apply an operation to the object and update span after it. Switches
   * the object to vector state. Should be called for all operations that change
   * object size or can cause reallocation.
   */
  template<class F>
  void modify_as_vector(F&& operation)
  {
    operation(*this);
    update_span();
  }

  /**
   * @brief Alternative to `modify_as_vector` for operations with output
   */
  template<class F>
  auto modify_as_vector_with_return(F&& operation) ->
      typename std::result_of<F(vector_type&)>::type
  {
    auto result = operation(*this);
    update_span();
    return result;
  }

  /**
   * @brief Alternative to `modify_as_vector_with_return` for operations that
   * return iterator pointing to vector
   */
  template<class F>
  auto modify_as_vector_with_iterator_return(F&& operation) -> iterator
  {
    const auto iter = modify_as_vector_with_return(std::forward<F>(operation));
    return to_span_iterator(iter);
  }

  /**
   * @brief Switch the object to vector state with specified capacity and copy
   * current data to new memory location
   */
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

  /**
   * @brief Switch the object to vector state with current capacity and current
   * data to new memory location
   */
  void switch_to_vector() { switch_to_vector(span_capacity_); }

  /**
   * @brief Switch the object to vector state with specified capacity without
   * copying data
   */
  void switch_to_empty_vector(size_type capacity)
  {
    assert(is_span());
    modify_as_vector([&](vector_type& vec) { vec.reserve(capacity); });
  }

  /**
   * @brief Switch the object to vector state without copying data
   */
  void switch_to_empty_vector() { switch_to_empty_vector(span_capacity_); }

  /**
   * @brief Convert span iterator to vector iterator
   */
  auto to_vector_iterator(iterator iter) noexcept ->
      typename vector_type::iterator
  {
    assert(is_vector());
    return vector_type::begin() + (iter - begin());
  }

  /**
   * @brief Convert span iterator to vector iterator
   */
  auto to_vector_iterator(const_iterator iter) const noexcept ->
      typename vector_type::const_iterator
  {
    assert(is_vector());
    return vector_type::begin() + (iter - begin());
  }

  /**
   * @brief Convert vector iterator to span iterator
   */
  auto to_span_iterator(typename vector_type::iterator iter) const noexcept
      -> iterator
  {
    assert(is_vector());
    return begin() + (iter - vector_type::begin());
  }

  /**
   * @brief Convert vector iterator to span iterator
   */
  auto to_span_iterator(typename vector_type::const_iterator iter)
      const noexcept -> const_iterator
  {
    assert(is_vector());
    return begin() + (iter - vector_type::begin());
  }

private:
  /**
   * @brief Synchronize span state with vector
   */
  void update_span()
  {
    is_span_ = false;
    span_type::operator=({vector_type::data(), vector_type::size()});
  }

  bool is_span_ = false;
  size_type span_capacity_ = 0;
};

/**
 * @brief Replicates corresponding function for std::vector
 */
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

/**
 * @brief Replicates corresponding function for std::vector
 */
template<class T, class Allocator>
auto operator!=(const span_or_vector_base<T, Allocator>& lhs,
                const span_or_vector_base<T, Allocator>& rhs) -> bool
{
  return !(lhs == rhs);
}

/**
 * @brief Replicates corresponding function for std::vector
 */
template<class T, class Allocator>
auto operator<(const span_or_vector_base<T, Allocator>& lhs,
               const span_or_vector_base<T, Allocator>& rhs) -> bool
{
  return std::lexicographical_compare(
      lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

/**
 * @brief Replicates corresponding function for std::vector
 */
template<class T, class Allocator>
auto operator>(const span_or_vector_base<T, Allocator>& lhs,
               const span_or_vector_base<T, Allocator>& rhs) -> bool
{
  return std::lexicographical_compare(
      rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

/**
 * @brief Replicates corresponding function for std::vector
 */
template<class T, class Allocator>
auto operator>=(const span_or_vector_base<T, Allocator>& lhs,
                const span_or_vector_base<T, Allocator>& rhs) -> bool
{
  return !(lhs < rhs);
}

/**
 * @brief Replicates corresponding function for std::vector
 */
template<class T, class Allocator>
auto operator<=(const span_or_vector_base<T, Allocator>& lhs,
                const span_or_vector_base<T, Allocator>& rhs) -> bool
{
  return !(lhs > rhs);
}

/**
 * @brief Base class with assignment method definitions
 */
template<class T, class Allocator>
class span_or_vector_assignments
    : virtual public span_or_vector_base<T, Allocator>
{
  using base = span_or_vector_base<T, Allocator>;

public:
  using vector_type = typename base::vector_type;
  using size_type = typename base::size_type;

protected:  // Prohibit object creation
  span_or_vector_assignments() = default;
  ~span_or_vector_assignments() = default;
  span_or_vector_assignments(const span_or_vector_assignments&) = default;
  span_or_vector_assignments(span_or_vector_assignments&&) noexcept = default;
  auto operator=(const span_or_vector_assignments&)
      -> span_or_vector_assignments& = default;

public:
  // Delete move assignment operator to avoid problems with virtual inheritance
  auto operator=(span_or_vector_assignments&&) noexcept
      -> span_or_vector_assignments& = delete;

  /**
   * @brief Replicates corresponding std::vector method
   */
  auto operator=(const vector_type& other) -> span_or_vector_assignments&
  {
    this->modify_as_vector([&](vector_type& vec) { vec = other; });
    return *this;
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  auto operator=(vector_type&& other) -> span_or_vector_assignments&
  {
    this->modify_as_vector([&](vector_type& vec) { vec = std::move(other); });
    return *this;
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  auto operator=(std::initializer_list<T> ilist) -> span_or_vector_assignments&
  {
    assign(ilist);
    return *this;
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  void assign(size_type count, const T& value)
  {
    if (this->is_vector()) {
      this->modify_as_vector([&](vector_type& vec)
                             { vec.assign(count, value); });
    } else {
      this->resize(count, value);
      std::fill_n(this->begin(), count, value);
    }
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  template<class InputIt,
           typename = std::enable_if<std::is_same<
               typename std::iterator_traits<InputIt>::iterator_category,
               std::input_iterator_tag>::value>>
  void assign(InputIt first, InputIt last)
  {
    assert(first <= last);
    if (this->is_vector()) {
      this->modify_as_vector([&](vector_type& vec)
                             { vec.assign(first, last); });
    } else {
      this->resize(static_cast<size_type>(std::distance(first, last)));
      std::copy(first, last, this->begin());
    }
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  void assign(std::initializer_list<T> ilist)
  {
    assign(ilist.begin(), ilist.end());
  }
};

/**
 * @brief Base class with element access method definitions
 */
template<class T, class Allocator>
class span_or_vector_element_access
    : virtual public span_or_vector_base<T, Allocator>
{
  using base = span_or_vector_base<T, Allocator>;

public:
  using size_type = typename base::size_type;
  using reference = typename base::reference;
  using const_reference = typename base::const_reference;

protected:  // Prohibit object creation
  span_or_vector_element_access() = default;
  ~span_or_vector_element_access() = default;
  span_or_vector_element_access(const span_or_vector_element_access&) = default;
  span_or_vector_element_access(span_or_vector_element_access&&) noexcept =
      default;
  auto operator=(const span_or_vector_element_access&)
      -> span_or_vector_element_access& = default;

public:
  // Delete move assignment operator to avoid problems with virtual inheritance
  auto operator=(span_or_vector_element_access&&) noexcept
      -> span_or_vector_element_access& = delete;

  /**
   * @brief Replicates corresponding std::vector method
   */
  auto at(size_type pos) const -> const_reference
  {
    check_out_of_range(pos);
    return this->operator[](pos);
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  auto at(size_type pos) -> reference
  {
    check_out_of_range(pos);
    return this->operator[](pos);
  }

private:
  void check_out_of_range(size_type pos) const
  {
    if (pos >= this->size()) {
      throw std::out_of_range(std::string("span_or_vector::at : Position ")
                              + std::to_string(pos) + " is out of range [0, "
                              + std::to_string(this->size()) + ")");
    }
  }
};

/**
 * @brief Base class with modifier method definitions
 */
template<class T, class Allocator>
class span_or_vector_modifiers
    : virtual public span_or_vector_base<T, Allocator>
{
  using base = span_or_vector_base<T, Allocator>;

public:
  using vector_type = typename base::vector_type;
  using size_type = typename base::size_type;
  using iterator = typename base::iterator;
  using const_iterator = typename base::const_iterator;

protected:  // Prohibit object creation
  span_or_vector_modifiers() = default;
  ~span_or_vector_modifiers() = default;
  span_or_vector_modifiers(const span_or_vector_modifiers&) = default;
  span_or_vector_modifiers(span_or_vector_modifiers&&) noexcept = default;
  auto operator=(const span_or_vector_modifiers&)
      -> span_or_vector_modifiers& = default;

public:
  // Delete move assignment operator to avoid problems with virtual inheritance
  auto operator=(span_or_vector_modifiers&&) noexcept
      -> span_or_vector_modifiers& = delete;

  /**
   * @brief Replicates corresponding std::vector method
   */
  auto insert(const_iterator pos, const T& value) -> iterator
  {
    return insert(pos, 1, value);
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  auto insert(const_iterator pos, T&& value) -> iterator
  {
    return emplace(pos, std::move(value));
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  auto insert(const_iterator pos, size_type count, const T& value) -> iterator
  {
    if (this->is_vector()) {
      return this->modify_as_vector_with_iterator_return(
          [&](vector_type& vec) -> typename vector_type::iterator
          { return vec.insert(this->to_vector_iterator(pos), count, value); });
    }

    return insert_into_span(pos,
                            count,
                            [&](iterator iter) -> iterator
                            { return std::fill_n(iter, count, value); });
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  template<class InputIt,
           typename = std::enable_if<std::is_same<
               typename std::iterator_traits<InputIt>::iterator_category,
               std::input_iterator_tag>::value>>
  auto insert(const_iterator pos, InputIt first, InputIt last) -> iterator
  {
    if (this->is_vector()) {
      return this->modify_as_vector_with_iterator_return(
          [&](vector_type& vec) -> typename vector_type::iterator
          { return vec.insert(this->to_vector_iterator(pos), first, last); });
    }

    assert(first <= last);
    return insert_into_span(pos,
                            static_cast<std::size_t>(last - first),
                            [&](iterator iter) -> iterator
                            { return std::copy(first, last, iter); });
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  auto insert(const_iterator pos, std::initializer_list<T> ilist) -> iterator
  {
    return insert(pos, ilist.begin(), ilist.end());
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  template<class... Args>
  auto emplace(const_iterator pos, Args&&... args) -> iterator
  {
    if (this->is_vector()) {
      return this->modify_as_vector_with_iterator_return(
          [&](vector_type& vec) -> typename vector_type::iterator
          {
            return vec.emplace(this->to_vector_iterator(pos),
                               std::forward<Args>(args)...);
          });
    }

    return insert_into_span(pos,
                            1,
                            [&](iterator iter) -> iterator
                            {
                              // False positive because in general case we call
                              // T's constructor here
                              // NOLINTNEXTLINE(google-readability-casting)
                              *iter++ = T(std::forward<Args>(args)...);
                              return iter;
                            });
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  auto erase(const_iterator pos) -> iterator { return erase(pos, pos + 1); }

  /**
   * @brief Replicates corresponding std::vector method
   */
  auto erase(const_iterator first, const_iterator last) -> iterator
  {
    if (this->is_vector()) {
      return this->modify_as_vector_with_iterator_return(
          [&](vector_type& vec) -> typename vector_type::iterator
          {
            return vec.erase(this->to_vector_iterator(first),
                             this->to_vector_iterator(last));
          });
    }

    const auto first_as_index = first - this->begin();
    assert(first_as_index >= 0);
    assert(static_cast<size_type>(first_as_index) < this->size());

    const auto n_erased = std::distance(first, last);

    std::move(this->begin() + first_as_index + n_erased,
              this->end(),
              this->begin() + first_as_index);
    this->resize(this->size() - static_cast<size_type>(n_erased));
    return this->begin() + first_as_index;
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  void push_back(const T& value) { insert(this->end(), value); }

  /**
   * @brief Replicates corresponding std::vector method
   */
  void push_back(T&& value) { insert(this->end(), std::move(value)); }

  /**
   * @brief Replicates corresponding std::vector method
   */
  template<class... Args>
  void emplace_back(Args&&... args)
  {
    emplace(this->end(), std::forward<Args>(args)...);
  }

  /**
   * @brief Replicates corresponding std::vector method
   */
  void pop_back() { this->resize(this->size() - 1); }

  /**
   * @brief Replicates corresponding std::vector method
   */
  void clear() { this->resize(0); }

private:
  template<class FillInsertedValues>
  auto insert_into_span(const_iterator pos,
                        size_type count,
                        FillInsertedValues&& fill) -> iterator
  {
    assert(this->is_span());
    assert(pos >= this->begin());
    assert(pos <= this->end());

    const auto pos_as_index = pos - this->begin();
    auto non_const_pos = this->begin() + pos_as_index;

    // We use pointers as random access iterators here
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto new_size = this->size() + count;
    if (new_size <= this->capacity()) {
      const auto old_end = this->cend();
      this->resize(new_size);
      std::move_backward(pos, old_end, this->end());
    } else {
      this->modify_as_vector(
          [&](vector_type& vec)
          {
            vec.resize(new_size);
            auto vec_it = std::copy(this->cbegin(), pos, vec.begin());
            vec_it +=
                static_cast<typename vector_type::iterator::difference_type>(
                    count);
            std::copy(pos, this->cend(), vec_it);
          });
      non_const_pos = this->begin() + pos_as_index;
    }

    const auto out_it = fill(non_const_pos);

    assert(out_it == non_const_pos + count);
    (void)(out_it);

    return non_const_pos;

    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  }
};

}  // namespace detail

/**
 * @brief Object of this class behaves like std::vector but can be initialized
 * as span and use existing data as its content. In this case if called method
 * can't do its work with the object initialized as span, the objects allocates
 * its own memory and copies its data to a new location.
 * @tparam T contained value type
 * @tparam Allocator allocator type
 */
template<class T, class Allocator = std::allocator<T>>
class span_or_vector
    : virtual public detail::span_or_vector_base<T, Allocator>
    , public detail::span_or_vector_element_access<T, Allocator>
    , public detail::span_or_vector_modifiers<T, Allocator>
    , public detail::span_or_vector_assignments<T, Allocator>
{
  using base = detail::span_or_vector_base<T, Allocator>;

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

  using detail::span_or_vector_assignments<T, Allocator>::operator=;
};
}  // namespace span_or_vector
