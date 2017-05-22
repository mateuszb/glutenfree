#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <iterator>

namespace common {
  using std::size_t;
  using std::to_string;

  template<typename T>
  class DisplacedArray {
  public:

    class iterator : public std::iterator<std::random_access_iterator_tag, T, size_t, const T*, const T&> {
    public:
      explicit iterator(T array[], size_t start, size_t end)
	: base(array), idx(start), nelements(end - start)
      {
      }

      iterator& operator++()
      {
	if (idx < (idx + nelements)) {
	  ++idx;
	}
	return *this;
      }

      iterator operator++(int)
      {
	auto ret = *this;
	++(*this);
	return ret;
      }

      auto operator==(iterator rhs) const
      {
	return idx == rhs.idx;
      }

      auto operator!=(iterator rhs) const
      {
	return !(*this == rhs);
      }

      T& operator*() const { return base[idx]; }

    private:
      size_t idx;
      size_t nelements;
      T* base;
    };
    
    using const_iterator = const iterator;    
    
    DisplacedArray(T* base, size_t disp, size_t nelmts)
      : ptr(base), displacement(disp), nelements(nelmts)
    {
    }

    DisplacedArray(DisplacedArray& rhs) = delete;
    DisplacedArray() = delete;

    T& operator[](size_t idx)
    {
      if (idx < displacement + nelements)
	return ptr[displacement + idx];
      else
	throw std::out_of_range("Index " + to_string(idx)
				+ " is out of range on displaced array with displacement "
				+ to_string(displacement)
				+ " and size of " + to_string(nelements));
    }
    
    const T& operator[](size_t idx) const
    {
      if (idx < displacement + nelements)
	return ptr[displacement + idx];
      else
	throw std::out_of_range("Index " + to_string(idx)
				+ " is out of range on displaced array with displacement "
				+ to_string(displacement)
				+ " and size of " + to_string(nelements));
    }

    constexpr auto cbegin() const
    {
      return const_iterator(ptr, displacement + 0, nelements);
    }

    constexpr auto cend() const
    {
      return const_iterator(ptr, displacement + nelements, nelements);
    }

    auto begin() const
    {
      return iterator(ptr, displacement, nelements);
    }

    auto end() const
    {
      return iterator(ptr, displacement + nelements, nelements);
    }
    
  private:
    T* ptr;
    const size_t displacement;
    const size_t nelements;
  };
}
