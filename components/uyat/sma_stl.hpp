#pragma once

#include <cstddef>
#include <cstdlib>
#include <memory>

#include "sma.hpp"

namespace sma
{

template <typename T, typename X>
class STLAllocator {
public:
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    template<typename U>
    struct rebind {
        typedef STLAllocator<U, X> other;
    };

    inline STLAllocator():sma_(X::get_sma()) {}
    inline ~STLAllocator() = default;
    inline STLAllocator(const STLAllocator&) = default;
    template<typename U>
    inline STLAllocator(const STLAllocator<U, X>& other):sma_(other.sma_){}

    inline pointer address(reference r) {
        return &r;
    }

    inline const_pointer address(const_reference r) {
        return &r;
    }

    inline pointer allocate(size_type cnt, const void* hint = 0) {
        T *ptr = reinterpret_cast<T*>(sma_.allocate(cnt * sizeof(T)));
        return ptr;
    }

    inline void deallocate(pointer p, size_type) {
        sma_.free(reinterpret_cast<uint8_t*>(p));
    }

    inline size_type max_size() const {
        return sma_.max_size() / sizeof(T);
    }

    inline void construct(pointer p, const T& t) {
        new(p) T(t);
    }

    inline void destroy(pointer p) {
        p->~T();
    }

    inline bool operator==(const STLAllocator& a) { return &this->sma_ == &a.sma_; }
    inline bool operator!=(const STLAllocator& a) { return !operator==(a); }

private:
   StaticMemoryAllocator& sma_;

   template <typename U, typename Y> friend class STLAllocator;
};

}
