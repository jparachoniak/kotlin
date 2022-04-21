/*
 * Copyright 2010-2022 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#pragma once

#include <cstddef>
#include <new>
#include <utility>

#include "cpp_support/CStdlib.hpp"

namespace konan {

inline void* calloc(size_t count, size_t size) {
    return kotlin::std_support::calloc(count, size);
}

inline void* calloc_aligned(size_t count, size_t size, size_t alignment) {
    return kotlin::std_support::aligned_calloc(alignment, count, size);
}

inline void free(void* ptr) {
    kotlin::std_support::free(ptr);
}

} // namespace konan

inline void* konanAllocMemory(size_t size) {
  return konan::calloc(1, size);
}

inline void* konanAllocAlignedMemory(size_t size, size_t alignment) {
    return konan::calloc_aligned(1, size, alignment);
}

inline void konanFreeMemory(void* memory) {
  konan::free(memory);
}

template<typename T>
inline T* konanAllocArray(size_t length) {
  return reinterpret_cast<T*>(konanAllocMemory(length * sizeof(T)));
}

template <typename T, typename ...A>
inline T* konanConstructInstance(A&& ...args) {
  return new (konanAllocMemory(sizeof(T))) T(::std::forward<A>(args)...);
}

template <typename T, typename ...A>
inline T* konanConstructSizedInstance(size_t size, A&& ...args) {
  return new (konanAllocMemory(size)) T(::std::forward<A>(args)...);
}

template <typename T>
inline void konanDestructInstance(T* instance) {
  instance->~T();
  konanFreeMemory(instance);
}

// Force a class to be heap-allocated using `konanAllocMemory`. Does not prevent stack allocation, or
// allocation as part of another object.
// Usage:
// class A : public KonanAllocatorAware {
//     ...
// };
class KonanAllocatorAware {
public:
    static void* operator new(size_t count) noexcept { return konanAllocMemory(count); }
    static void* operator new[](size_t count) noexcept { return konanAllocMemory(count); }

    static void* operator new(size_t count, void* ptr) noexcept { return ptr; }
    static void* operator new[](size_t count, void* ptr) noexcept { return ptr; }

    static void operator delete(void* ptr) noexcept { konanFreeMemory(ptr); }
    static void operator delete[](void* ptr) noexcept { konanFreeMemory(ptr); }

protected:
    // Hide constructors, assignments and destructor to discourage operating on instance of `KonanAllocatorAware`
    KonanAllocatorAware() = default;

    KonanAllocatorAware(const KonanAllocatorAware&) = default;
    KonanAllocatorAware(KonanAllocatorAware&&) = default;

    KonanAllocatorAware& operator=(const KonanAllocatorAware&) = default;
    KonanAllocatorAware& operator=(KonanAllocatorAware&&) = default;

    // Not virtual by design. Since this class hides this destructor, no one can destroy an
    // instance of `KonanAllocatorAware` directly, so this destructor is never called in a virtual manner.
    ~KonanAllocatorAware() = default;
};
