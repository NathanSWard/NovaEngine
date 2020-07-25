#pragma once

#include <algorithm>
#include <functional>
#include <vector>

namespace detail {

template<class T>
struct is_transparent {
    template<class U, class Transparent = typename U::is_transparent>
    static auto test(int) -> decltype(std::true_type{});
    template<class>
    static auto test(...) -> std::false_type;

    static constexpr bool value = decltype(test<T>(0))::value;
};

template<class T>
inline static constexpr bool is_transparent_v = is_transparent<T>::value;

template<bool IsUnique, class T, class Container = std::vector<T>, class Cmp = std::less<>, class Eq = std::equal_to<>>
class adapter_base {
    Container c_;
public:
    using value_type = T;
    using container_type = Container;
    using size_type = typename container_type::size_type;
    using const_iterator = typename container_type::const_iterator;

    constexpr adapter_base() noexcept = default;

    template<class... Args, std::enable_if_t<std::is_constructible_v<container_type, Args...>, int> = 0>
    constexpr explicit adapter_base(Args&&... args) noexcept(std::is_nothrow_constructible_v<container_type, Args...>)
        : c_(std::forward<Args>(args)...) 
    {
        if constexpr (sizeof...(Args) > 0) {
            std::sort(c_.begin(), c_.end(), Cmp{});
            if constexpr (IsUnique)
                c_.erase(std::unique(c_.begin(), c_.end(), Eq{}), c_.end());
        }
    }

    constexpr adapter_base(adapter_base const&) = default;
    constexpr adapter_base(adapter_base&&) = default;
    constexpr adapter_base& operator=(adapter_base const&) = default;
    constexpr adapter_base& operator=(adapter_base&&) = default;

    // access
    constexpr container_type const& get_container() const noexcept {
        return c_;
    }

    // iterators
    constexpr auto begin() const noexcept {
        return c_.begin();
    }

    constexpr auto cbegin() const noexcept {
        return c_.cbegin();
    }

    constexpr auto end() const noexcept {
        return c_.end();
    }

    constexpr auto cend() const noexcept {
        return c_.cend();
    }

    constexpr auto rbegin() const noexcept {
        return c_.rbegin();
    }

    constexpr auto crbegin() const noexcept {
        return c_.crbegin();
    }

    constexpr auto rend() const noexcept {
        return c_.rend();
    }

    constexpr auto crend() const noexcept {
        return c_.crend();
    }

    // query
    constexpr bool contains(value_type const& val) const noexcept {
        return std::binary_search(c_.cbegin(), c_.cend(), val, Cmp{});
    }

    template<class U, class C = Cmp, std::enable_if_t<is_transparent_v<C>, int> = 0>
    constexpr bool contains(U const& val) const noexcept {
        return std::binary_search(c_.cbegin(), c_.cend(), val, Cmp{});
    }

    constexpr const_iterator find(value_type const& val) const noexcept {
        if (auto const it = lower_bound(val); it == end() || Eq{}(*it, val))
            return it;
        return cend();
    }

    template<class U, class C = Cmp, class E = Eq, std::enable_if_t<is_transparent_v<C> && is_transparent_v<E>, int> = 0>
    constexpr const_iterator find(U const& val) const noexcept {
        if (auto const it = lower_bound(val); it == end() || Eq{}(*it, val))
            return it;
        return cend();
    }

    constexpr size_type count(value_type const& val) const noexcept {
        auto const [first, last] = equal_range(val);
        return std::distance(first, last);
    }

    template<class U, class C = Cmp, std::enable_if_t<is_transparent_v<C>, int> = 0>
    constexpr size_type count(U const& val) const noexcept {
        auto const [first, last] = equal_range(val);
        return std::distance(first, last);
    }

    constexpr auto lower_bound(value_type const& val) const noexcept {
        return std::lower_bound(c_.cbegin(), c_.cend(), val, Cmp{});
    }

    template<class U, class C = Cmp, std::enable_if_t<is_transparent_v<C>, int> = 0>
    constexpr auto lower_bound(U const& val) const noexcept {
        return std::lower_bound(c_.cbegin(), c_.cend(), val, Cmp{});
    }

    constexpr auto upper_bound(value_type const& val) const noexcept {
        return std::upper_bound(c_.cbegin(), c_.cend(), val, Cmp{});
    }

    template<class U, class C = Cmp, std::enable_if_t<is_transparent_v<C>, int> = 0>
    constexpr auto upper_bound(U const& val) const noexcept {
        return std::upper_bound(c_.cbegin(), c_.cend(), val, Cmp{});
    }

    constexpr auto equal_range(value_type const& val) const noexcept {
        return std::equal_range(c_.cbegin(), c_.cend(), val, Cmp{});
    }

    template<class U, class C = Cmp, std::enable_if_t<is_transparent_v<C>, int> = 0>
    constexpr auto equal_range(U const& val) const noexcept {
        return std::equal_range(c_.cbegin(), c_.cend(), val, Cmp{});
    }

    // capacity
    constexpr bool empty() const noexcept {
        return c_.empty();
    }

    constexpr auto size() const noexcept {
        return c_.size();
    }

    constexpr auto max_size() const noexcept {
        return c_.max_size();
    }

    // modifiers
    constexpr void clear() {
        c_.clear();
    }

    template<class U, std::enable_if_t<!std::is_convertible_v<U, const_iterator>, int> = 0>
    constexpr size_type erase(U const& val) {
        if constexpr (IsUnique) {
            if (auto const it = find(val); it != end()) {
                erase(it);
                return size_type(1);
            }
        }
        else {
            if (auto const [first, last] = equal_range(val); first != end()) {
                erase(first, last);
                return std::distance(first, last);
            }
        }
        return size_type(0);
    }

    constexpr auto erase(const_iterator const it) {
        return c_.erase(it);
    }

    constexpr auto erase(const_iterator const first, const_iterator const last) {
        return c_.erase(first, last);
    }

    template<class U>
    constexpr auto insert(U&& val) {
        auto const it = lower_bound(val);
        if constexpr (!IsUnique) {
            using R = std::pair<const_iterator, bool>;
            if (it == c_.end() || !Eq{}(*it, val))
                return R(c_.emplace(it, std::forward<U>(val)), true);
            return R(it, false);
        }
        else
            return const_iterator(c_.emplace(it, std::forward<U>(val)));
    }
    
    constexpr void swap(adapter_base& other) noexcept {
        c_.swap(other.c_);
    }
};

} // namespace detail

template<bool IsUnique, class T, class C, class Cmp, class Eq, class U>
constexpr auto erase(detail::adapter_base<IsUnique, T, C, Cmp, Eq>& c, U const& val) {
    auto const [first, last] = c.equal_range(val);
    auto const num_erased = std::distance(first, last);
    c.erase(first, last);
    return num_erased;
}

template<bool IsUnique, class T, class C, class Cmp, class Eq, class Pred>
constexpr auto erase_if(detail::adapter_base<IsUnique, T, C, Cmp, Eq>& c, Pred&& pred) {
    auto const it = std::remove_if(c.begin(), c.end(), pred);
    auto const num_erased = std::distance(it, c.end());
    c.erase(it, c.end());
    return num_erased;
}

template<class T, class Container = std::vector<T>, class Cmp = std::less<>>
class sorted_adapter : public detail::adapter_base<false, T, Container, Cmp> {
    using base_t = detail::adapter_base<false, T, Container, Cmp>;
public:
    template<class... Args>
    constexpr sorted_adapter(Args&&... args) : base_t(std::forward<Args>(args)...) {}
};

template<class T, class Container = std::vector<T>, class Eq = std::equal_to<>, class Cmp = std::less<>>
class unique_sorted_adapter : public detail::adapter_base<true, T, Container, Cmp, Eq> {
    using base_t = detail::adapter_base<true, T, Container, Cmp, Eq>;
public:
    template<class... Args>
    constexpr unique_sorted_adapter(Args&&... args) : base_t(std::forward<Args>(args)...) {}
};