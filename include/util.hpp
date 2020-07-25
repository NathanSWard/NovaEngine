#pragma once

#include <cassert>
#include <cstring>

namespace nova {

#ifndef NOVA_ASSERT
    #define NOVA_ASSERT(condition) assert(condition)
#endif

namespace util {
#if __cpp_lib_bit_cast > 201806L
    template<class To, class From>
    [[nodiscard]] inline constexpr To bit_cast(From const& from) noexcept {
        return std::bit_cast<To>(from);
    }
#else
    template<class To, class From>
    concept bit_cast_compatible = sizeof(To) == sizeof(From) &&
        std::is_trivially_copyable_v<From> &&
        std::is_trivially_copyable_v<To>;

    template<class To, class From>
    requires bit_cast_compatible<To, From>
    [[nodiscard]] inline constexpr To bit_cast(From const& from) noexcept { 
        To to{};
        std::memcpy(&to, &from, sizeof(To));
        return to;
    }
#endif

template<class T>
class tagged_ptr {
public:
    using ptr_t = T;
    using tag_t = std::uint16_t;

private:
    union cast_unit {
        ptr_t value{0};
        std::array<tag_t, 4> tag;
    };

    static_assert(sizeof(ptr_t) == sizeof(std::array<tag_t, 4>));

    static constexpr std::size_t tag_index = 3;
    static constexpr ptr_t ptr_mask = 0xffffffffffffUL;

    static constexpr ptr_t extract_ptr(ptr_t const& i) noexcept {
        return (i & ptr_mask);
    }

    static constexpr tag_t extract_tag(ptr_t const& i) noexcept {
        cast_unit cu;
        cu.value = i;
        return cu.tag[tag_index];
    }

    static constexpr ptr_t pack_ptr(ptr_t const ptr, tag_t const tag) noexcept {
        cast_unit ret;
        ret.value = ptr;
        ret.tag[tag_index] = tag;
        return ret.value;
    }

public:
    constexpr tagged_ptr() noexcept = default;
    constexpr tagged_ptr(tagged_ptr const&) noexcept = default;

    constexpr explicit tagged_ptr(ptr_t const p, tag_t const t = 0) noexcept 
        : ptr(pack_ptr(p, t)) {}

    constexpr tagged_ptr& operator=(tagged_ptr const&) noexcept = default;

    constexpr void set(ptr_t const p, tag_t const t) noexcept {
        ptr = pack_ptr(p, t);
    }

    constexpr bool operator==(tagged_ptr const& p) const noexcept {
        return (ptr == p.ptr);
    }

    constexpr bool operator!=(tagged_ptr const& p) const {
        return !operator==(p);
    }

    constexpr ptr_t get_ptr() const noexcept {
        return extract_ptr(ptr);
    }

    constexpr void set_ptr(ptr_t const p) noexcept {
        auto const tag = get_tag();
        ptr = pack_ptr(p, tag);
    }

    constexpr tag_t get_tag() const noexcept {
        return extract_tag(ptr);
    }

    constexpr tag_t get_next_tag() const noexcept {
        tag_t const next = (get_tag() + 1) & (std::numeric_limits<tag_t>::max)();
        return next;
    }

    constexpr void set_tag(tag_t const t) noexcept {
        auto const p = get_ptr();
        ptr = pack_ptr(p, t);
    }

protected:
    ptr_t ptr;
};

} // namespace util

} // namespace nova