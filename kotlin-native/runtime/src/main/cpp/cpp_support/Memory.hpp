/*
 * Copyright 2010-2022 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>

#include "Alloc.h"

namespace kotlin::std_support {

// Default allocator for Kotlin.
// TODO: Consider overriding global operator new and operator delete instead. However, make sure this does
//       not extend over to interop.
template <typename T>
struct allocator {
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    allocator() noexcept = default;

    allocator(const allocator&) noexcept = default;

    template <typename U>
    allocator(const allocator<U>&) noexcept {}

    // TODO: maybe malloc, actually?
    T* allocate(std::size_t n) noexcept { return static_cast<T*>(konan::calloc(n, sizeof(T))); }

    void deallocate(T* p, std::size_t n) noexcept { konan::free(p); }
};

template <typename T, typename U>
bool operator==(const allocator<T>&, const allocator<U>&) noexcept {
    return true;
}

template <typename T, typename U>
bool operator!=(const allocator<T>&, const allocator<U>&) noexcept {
    return false;
}

template <typename T, typename Allocator, typename... Args>
T* allocator_new(const Allocator& allocator, Args&&... args) {
    using TAllocatorTraits = typename std::allocator_traits<Allocator>::template rebind_traits<T>;
    using TAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;

    auto a = TAllocator(allocator);
    T* ptr = TAllocatorTraits::allocate(a, 1);

#if KONAN_NO_EXCEPTIONS
    TAllocatorTraits::construct(a, ptr, std::forward<Args>(args)...);
    return ptr;
#else
    try {
        TAllocatorTraits::construct(a, ptr, std::forward<Args>(args)...);
        return ptr;
    } catch (...) {
        TAllocatorTraits::deallocate(a, ptr, 1);
        throw;
    }
#endif
}

template <typename T, typename Allocator>
void allocator_delete(const Allocator& allocator, T* ptr) noexcept {
    using TAllocatorTraits = typename std::allocator_traits<Allocator>::template rebind_traits<T>;
    using TAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;

    auto a = TAllocator(allocator);
    TAllocatorTraits::destroy(a, ptr);
    TAllocatorTraits::deallocate(a, ptr, 1);
}

template <typename Allocator>
class allocator_deleter {
    template <typename Other>
    using RebindAllocator =
            typename std::allocator_traits<Other>::template rebind_alloc<typename std::allocator_traits<Allocator>::value_type>;

public:
    explicit allocator_deleter(const Allocator& allocator = Allocator()) noexcept : allocator(allocator) {}

    template <typename Other, typename = std::enable_if_t<std::is_same_v<Allocator, RebindAllocator<Other>>>>
    allocator_deleter(allocator_deleter<Other> deleter) noexcept : allocator(RebindAllocator<Other>(deleter.allocator)) {}

    void operator()(typename std::allocator_traits<Allocator>::pointer ptr) noexcept { allocator_delete(allocator, ptr); }

    [[no_unique_address]] Allocator allocator;
};

template <typename T, typename Allocator, typename... Args>
auto allocate_unique(const Allocator& allocator, Args&&... args) {
    static_assert(!std::is_array_v<T>, "T cannot be an array");
    using TAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
    using TDeleter = allocator_deleter<TAllocator>;
    return std::unique_ptr<T, TDeleter>(allocator_new<T>(allocator, std::forward<Args>(args)...), TDeleter(allocator));
}

template <typename T>
using default_delete = allocator_deleter<allocator<T>>;

template <typename T, typename Deleter = default_delete<T>>
using unique_ptr = std::unique_ptr<T, Deleter>;

template <typename T, typename... Args>
auto make_unique(Args&&... args) {
    static_assert(!std::is_array_v<T>, "T cannot be an array");
    return allocate_unique<T>(allocator<T>(), std::forward<Args>(args)...);
}

template <typename T, typename... Args>
auto make_shared(Args&&... args) {
    static_assert(!std::is_array_v<T>, "T cannot be an array");
    return std::allocate_shared<T>(allocator<T>(), std::forward<Args>(args)...);
}

template <typename T, typename... Args>
T* knew(Args&&... args) {
    return allocator_new<T>(allocator<T>(), std::forward<Args>(args)...);
}

template <typename T>
void kdelete(T* ptr) noexcept {
    allocator_delete(allocator<T>(), ptr);
}

} // namespace kotlin::std_support
