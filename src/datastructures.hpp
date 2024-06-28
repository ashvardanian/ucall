#pragma once
#include <cstring>     // `std::memcpy`
#include <memory>      // `std::malloc`
#include <type_traits> // `std::is_nothrow_default_constructible`

namespace unum::ucall {

/**
 *  @brief  A lightweight non-owning span template for contiguous sequences.
 *          It supports basic access and iteration operations.
 *
 *  @tparam element_at Type of the elements in the span.
 */
template <typename element_at> class span_gt {
    element_at* begin_{};
    element_at* end_{};

  public:
    span_gt(element_at* b, element_at* e) noexcept : begin_(b), end_(e) {}

    span_gt(span_gt&&) = delete;
    span_gt(span_gt const&) = delete;
    span_gt& operator=(span_gt const&) = delete;

    [[nodiscard]] element_at const* data() const noexcept { return begin_; }
    [[nodiscard]] element_at* data() noexcept { return begin_; }
    [[nodiscard]] element_at* begin() noexcept { return begin_; }
    [[nodiscard]] element_at* end() noexcept { return end_; }
    [[nodiscard]] std::size_t size() const noexcept { return end_ - begin_; }
    [[nodiscard]] element_at& operator[](std::size_t i) noexcept { return begin_[i]; }
    [[nodiscard]] element_at const& operator[](std::size_t i) const noexcept { return begin_[i]; }
    operator std::basic_string_view<element_at>() const noexcept { return {data(), size()}; }
};

/**
 *  @brief  A buffer class that reallocates on every resize.
 *  @tparam element_at Type of the elements in the buffer.
 */
template <typename element_at> class buffer_gt {
    element_at* elements_{};
    std::size_t capacity_{};
    static_assert(std::is_nothrow_default_constructible<element_at>());

  public:
    buffer_gt() noexcept = default;

    buffer_gt(buffer_gt&&) = delete;
    buffer_gt(buffer_gt const&) = delete;
    buffer_gt& operator=(buffer_gt const&) = delete;

    buffer_gt& operator=(buffer_gt&& other) noexcept {
        std::swap(elements_, other.elements_);
        std::swap(capacity_, other.capacity_);
        return *this;
    }

    [[nodiscard]] bool resize(std::size_t n) noexcept {
        elements_ = (element_at*)std::malloc(sizeof(element_at) * n);
        if (!elements_)
            return false;
        capacity_ = n;
        std::uninitialized_default_construct(elements_, elements_ + capacity_);
        return true;
    }

    ~buffer_gt() noexcept {
        if constexpr (!std::is_trivially_destructible<element_at>())
            std::destroy_n(elements_, capacity_);
        std::free(elements_);
        elements_ = nullptr;
    }

    [[nodiscard]] element_at const* data() const noexcept { return elements_; }
    [[nodiscard]] element_at* data() noexcept { return elements_; }
    [[nodiscard]] element_at* begin() noexcept { return elements_; }
    [[nodiscard]] element_at* end() noexcept { return elements_ + capacity_; }
    [[nodiscard]] std::size_t size() const noexcept { return capacity_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] element_at& operator[](std::size_t i) noexcept { return elements_[i]; }
    [[nodiscard]] element_at const& operator[](std::size_t i) const noexcept { return elements_[i]; }
};

/**
 *  @brief  A dynamic array class template for managing a resizable array of elements.
 *  @tparam element_at Type of the elements in the array.
 */
template <typename element_at> class array_gt {
    element_at* elements_{};
    std::size_t count_{};
    std::size_t capacity_{};
    static_assert(std::is_nothrow_default_constructible<element_at>());
    static_assert(std::is_trivially_copy_constructible<element_at>(), "Can't use realloc and memcpy");

  public:
    array_gt() = default;

    array_gt(array_gt&&) = delete;
    array_gt(array_gt const&) = delete;
    array_gt& operator=(array_gt const&) = delete;

    array_gt& operator=(array_gt&& other) noexcept {
        std::swap(elements_, other.elements_);
        std::swap(count_, other.count_);
        std::swap(capacity_, other.capacity_);
        return *this;
    }

    /**
     *  @brief  Reserves capacity for the specified number of elements,
     *          re-allocating the previous memory region, if needed.
     *  @param  n New capacity of the array.
     *  @return True if reservation is successful, otherwise false.
     */
    [[nodiscard]] bool reserve(std::size_t n) noexcept {
        if (n <= capacity_)
            return true;
        if (!elements_) {
            auto ptr = (element_at*)std::malloc(sizeof(element_at) * n);
            if (!ptr)
                return false;
            elements_ = ptr;
        } else {
            auto ptr = (element_at*)std::realloc(elements_, sizeof(element_at) * n);
            if (!ptr)
                return false;
            elements_ = ptr;
        }
        std::uninitialized_default_construct(elements_ + capacity_, elements_ + n);
        capacity_ = n;
        return true;
    }

    ~array_gt() noexcept { reset(); }

    /**
     *  @brief  Resets the array, destroying all elements and freeing memory.
     */
    void reset() noexcept {
        if constexpr (!std::is_trivially_destructible<element_at>())
            std::destroy_n(elements_, count_);
        std::free(elements_);
        elements_ = nullptr;
        capacity_ = count_ = 0;
    }

    [[nodiscard]] element_at const* data() const noexcept { return elements_; }
    [[nodiscard]] element_at* data() noexcept { return elements_; }
    [[nodiscard]] element_at* begin() noexcept { return elements_; }
    [[nodiscard]] element_at* end() noexcept { return elements_ + count_; }
    [[nodiscard]] std::size_t size() const noexcept { return count_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] element_at& operator[](std::size_t i) noexcept { return elements_[i]; }

    /**
     *  @brief  Adds an element to the end of the array, assuming enough memory is available.
     *  @param  element The element to add.
     */
    void push_back_reserved(element_at element) noexcept { new (elements_ + count_++) element_at(std::move(element)); }

    /**
     *  @brief  Removes one or more elements from the end of the array.
     *  @param  n Number of elements to remove.
     */
    void pop_back(std::size_t n = 1) noexcept { count_ -= n; }

    /**
     *  @brief  Appends multiple elements to the end of the array.
     *  @param  elements Pointer to the elements to append.
     *  @param  n Number of elements to append.
     *  @return True if append is successful, otherwise false.
     */
    [[nodiscard]] bool append_n(element_at const* elements, std::size_t n) noexcept {
        if (!reserve(size() + n))
            return false;
        std::memcpy(end(), elements, n * sizeof(element_at));
        count_ += n;
        return true;
    }
};

/**
 *  @brief  A "pool" allocator-like class to store reusable objects.
 *  @tparam element_at Type of the elements in the pool.
 */
template <typename element_at> class pool_gt {
    std::size_t capacity_{};
    std::size_t free_count_{};
    element_at* elements_{};
    std::size_t* free_offsets_{};
    static_assert(std::is_nothrow_default_constructible<element_at>());

  public:
    pool_gt() = default;

    pool_gt(pool_gt&&) = delete;
    pool_gt(pool_gt const&) = delete;
    pool_gt& operator=(pool_gt const&) = delete;

    pool_gt& operator=(pool_gt&& other) noexcept {
        std::swap(capacity_, other.capacity_);
        std::swap(free_count_, other.free_count_);
        std::swap(elements_, other.elements_);
        std::swap(free_offsets_, other.free_offsets_);
        return *this;
    }

    /**
     *  @brief  Reserves capacity for the specified number of elements.
     *  @param  n New capacity of the pool.
     *  @return True if reservation is successful, otherwise false.
     */
    [[nodiscard]] bool reserve(std::size_t n) noexcept {
        auto mem = std::malloc((sizeof(element_at) + sizeof(std::size_t)) * n);
        if (!mem)
            return false;
        elements_ = (element_at*)mem;
        free_offsets_ = (std::size_t*)(elements_ + n);
        free_count_ = capacity_ = n;
        std::uninitialized_default_construct(elements_, elements_ + capacity_);
        std::iota(free_offsets_, free_offsets_ + n, 0ul);
        return true;
    }

    ~pool_gt() noexcept {
        if constexpr (!std::is_trivially_destructible<element_at>())
            std::destroy_n(elements_, capacity_);
        std::free(elements_);
        elements_ = nullptr;
    }

    /**
     *  @brief  Allocates an element from the pool.
     *  @return Pointer to the allocated element, or nullptr if the pool is empty.
     */
    [[nodiscard]] element_at* alloc() noexcept {
        return free_count_ ? elements_ + free_offsets_[--free_count_] : nullptr;
    }

    /**
     *  @brief  Releases an element back to the pool.
     *  @param  released Pointer to the element to be released.
     */
    void release(element_at* released) noexcept { free_offsets_[free_count_++] = released - elements_; }

    /**
     *  @brief  Gets the offset of the given element in the pool.
     *  @param  element Reference to the element.
     *  @return Offset of the element in the pool.
     */
    [[nodiscard]] std::size_t offset_of(element_at& element) const noexcept { return &element - elements_; }

    /**
     *  @brief  Gets the element at the specified offset in the pool.
     *  @param  i Offset of the element.
     *  @return Reference to the element at the specified offset.
     */
    [[nodiscard]] element_at& at_offset(std::size_t i) const noexcept { return elements_[i]; }
};

/**
 *  @brief  Round-Robin construct for reusable connection states.
 *  @tparam element_at Type of the elements in the round-robin buffer.
 */
template <typename element_at> class round_robin_gt {
    element_at* circle_{};
    std::size_t count_{};
    std::size_t capacity_{};
    std::size_t idx_newest_{};
    std::size_t idx_oldest_{};
    std::size_t idx_to_poll_{};

  public:
    inline round_robin_gt& operator=(round_robin_gt&& other) noexcept {
        std::swap(circle_, other.circle_);
        std::swap(count_, other.count_);
        std::swap(capacity_, other.capacity_);
        std::swap(idx_newest_, other.idx_newest_);
        std::swap(idx_oldest_, other.idx_oldest_);
        std::swap(idx_to_poll_, other.idx_to_poll_);
        return *this;
    }

    /**
     *  @brief  Allocates memory for the specified number of elements in the round-robin buffer.
     *  @param  n Number of elements to allocate.
     *  @return True if allocation is successful, otherwise false.
     */
    inline bool alloc(std::size_t n) noexcept {
        auto cons = (element_at*)std::malloc(sizeof(element_at) * n);
        if (!cons)
            return false;
        circle_ = cons;
        capacity_ = n;
        return true;
    }

    /**
     *  @brief  Drops the oldest element from the round-robin buffer.
     *  @return The descriptor of the dropped element.
     */
    inline descriptor_t drop_tail() noexcept {
        descriptor_t old = std::exchange(circle_[idx_oldest_].descriptor, bad_descriptor_k);
        idx_to_poll_ = idx_to_poll_ == idx_oldest_ ? (idx_to_poll_ + 1) % capacity_ : idx_to_poll_;
        idx_oldest_ = (idx_oldest_ + 1) % capacity_;
        count_--;
        return old;
    }

    /**
     *  @brief  Adds a new element to the round-robin buffer.
     *  @param  new_ The new element to add.
     */
    inline void push_ahead(descriptor_t new_) noexcept {
        circle_[idx_newest_].descriptor = new_;
        circle_[idx_newest_].skipped_cycles = 0;
        circle_[idx_newest_].response.copies_count = 0;
        circle_[idx_newest_].response.iovecs_count = 0;
        idx_newest_ = (idx_newest_ + 1) % capacity_;
        count_++;
    }

    /**
     *  @brief  Polls the next element in the round-robin buffer.
     *  @return Reference to the polled element.
     */
    inline element_at& poll() noexcept {
        auto connection_ptr = &circle_[idx_to_poll_];
        auto idx_to_poll_following = (idx_to_poll_ + 1) % count_;
        idx_to_poll_ = idx_to_poll_following == idx_newest_ ? idx_oldest_ : idx_to_poll_following;
        return circle_[idx_to_poll_];
    }

    /**
     *  @brief  Gets the oldest element in the round-robin buffer.
     *  @return Reference to the oldest element.
     */
    inline element_at& tail() noexcept { return circle_[idx_oldest_]; }

    /**
     *  @brief  Gets the newest element in the round-robin buffer.
     *  @return Reference to the newest element.
     */
    inline element_at& head() noexcept { return circle_[idx_newest_ - 1]; }

    /**
     *  @brief  Gets the number of elements in the round-robin buffer.
     *  @return Number of elements.
     */
    inline std::size_t size() const noexcept { return count_; }

    /**
     *  @brief  Gets the capacity of the round-robin buffer.
     *  @return Capacity of the buffer.
     */
    inline std::size_t capacity() const noexcept { return capacity_; }
};

} // namespace unum::ucall
