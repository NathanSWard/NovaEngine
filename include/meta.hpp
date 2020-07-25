#pragma once

#include <type_traits>

#include "../deps/entt/single_include/entt/entt.hpp"

namespace nova::meta {

template<class...>
struct sink {};

template<class Sink>
struct sink_remove_cv;

template<class... Ts>
struct sink_remove_cv<sink<Ts...>> {
    using type = sink<std::remove_cv_t<Ts>...>;
};

template<class Sink>
using sink_remove_cv_t = typename sink_remove_cv<Sink>::type;

template<class Sink>
struct sink_remove_reference;

template<class... Ts>
struct sink_remove_reference<sink<Ts...>> {
    using type = sink<std::remove_reference_t<Ts>...>;
};

template<class Sink>
using sink_remove_reference_t = typename sink_remove_reference<Sink>::type;

template<class...>
struct always_false : std::false_type {};

template<class T>
struct is_member_func : std::false_type {};
template<class C, class R, class... Args>
struct is_member_func<R(C::*)(Args...)> : std::true_type {};
template<class C, class R, class... Args>
struct is_member_func<R(C::*)(Args...) const> : std::true_type {};
template<class C, class R, class... Args>
struct is_member_func<R(C::*)(Args...) noexcept> : std::true_type {};
template<class C, class R, class... Args>
struct is_member_func<R(C::*)(Args...) const noexcept> : std::true_type {};
template<class T>
inline constexpr bool is_member_func_v = is_member_func<T>::value;

#define HAS_MEM_FN(func) \
template<class T, class = void> \
struct has_##func##_mem_fn : std::false_type {}; \
template<class T> \
struct has_##func##_mem_fn<T, std::enable_if_t<is_member_func_v<decltype(&T::func)>>> \
    : std::true_type {}; \
template<class T> \
inline constexpr bool has_##func##_mem_fn_v = has_##func##_mem_fn<T>::value;

HAS_MEM_FN(process)

#undef HAS_MEM_FN

template<class, template<class...> class>
inline constexpr bool is_specialization_v = false;
template<class... Args, template<class...> class T>
inline constexpr bool is_specialization_v<T<Args...>, T> = true;

template<class Needle, class... Haystack>
struct is_in;

template<class Needle>
struct is_in<Needle> : std::false_type {};

template<class Needle, class Head, class... Tail>
struct is_in<Needle, Head, Tail...> {
    static constexpr bool value = std::is_same_v<Needle, Head> ? true : is_in<Needle, Tail...>::value;
};

template<class Needle, class... Haystack>
inline constexpr bool is_in_v = is_in<Needle, Haystack...>::value;

// concat a sink and a list of type into a sink of the unique types between them.
template<class Out, class In, class... List>
struct unique_concat_impl;

template<class... Out, class... In>
struct unique_concat_impl<sink<Out...>, sink<In...>> {
    using type = sink<Out...>;
};

template<class... Out, class... In, class Head, class... Tail>
struct unique_concat_impl<sink<Out...>, sink<In...>, Head, Tail...> {
    using type = std::conditional_t<is_in_v<Head, In...>, 
                    typename unique_concat_impl<sink<Out...>, sink<In...>, Tail...>::type,
                    typename unique_concat_impl<sink<Out..., Head>, sink<In...>, Tail...>::type>; 
};

template<class InSink, class... List>
struct unique_concat;

template<class... In, class... List>
struct unique_concat<sink<In...>, List...> : unique_concat_impl<sink<In...>, sink<In...>, List...> {}; 

template<class InSink, class... List>
using unique_concat_t = typename unique_concat<InSink, List...>::type;

// 
template<class Out, class In, class... List>
struct missing_types_impl;

template<class... Out, class... In>
struct missing_types_impl<sink<Out...>, sink<In...>> {
    using type = sink<Out...>;
};

template<class... Out, class... In, class Head, class... Tail>
struct missing_types_impl<sink<Out...>, sink<In...>, Head, Tail...> {
    using type = std::conditional_t<is_in_v<Head, In...>, 
                    typename missing_types_impl<sink<Out...>, sink<In...>, Tail...>::type,
                    typename missing_types_impl<sink<Out..., Head>, sink<In...>, Tail...>::type>;
};

template<class InSink, class... List>
struct missing_types;

template<class... In, class... List>
struct missing_types<sink<In...>, List...> : missing_types_impl<sink<>, sink<In...>, List...> {};

template<class InSink, class... List>
using missing_types_t = typename missing_types<InSink, List...>::type;

template<class MemFn>
struct mem_fn_traits;

template<class R, class C, class... Args>
struct mem_fn_traits<R(C::*)(Args...)> {
    using args_t = sink<Args...>;
    using ret_t = R;
    using class_t = C;
    static constexpr bool is_const = false;
    static constexpr bool is_noexcept = false;
};

template<class R, class C, class... Args>
struct mem_fn_traits<R(C::*)(Args...) noexcept> {
    using args_t = sink<Args...>;
    using ret_t = R;
    using class_t = C;
    static constexpr bool is_const = false;
    static constexpr bool is_noexcept = true;
};

template<class R, class C, class... Args>
struct mem_fn_traits<R(C::*)(Args...) const> {
    using args_t = sink<Args...>;
    using ret_t = R;
    using class_t = C;
    static constexpr bool is_const = true;
    static constexpr bool is_noexcept = false;
};

template<class R, class C, class... Args>
struct mem_fn_traits<R(C::*)(Args...) const noexcept> {
    using args_t = sink<Args...>;
    using ret_t = R;
    using class_t = C;
    static constexpr bool is_const = true;
    static constexpr bool is_noexcept = true;
};

} // namespace nova::meta