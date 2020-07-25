#pragma once

#include <concepts>
#include <ranges>
#include <type_traits>
#include <vector>

#include "component.hpp"
#include "meta.hpp"
#include "util.hpp"

#include <iostream>

namespace nova {

using SystemId = void(*)();

struct ISystem {
    virtual void processImpl(entt::registry& r) const noexcept = 0;
    virtual void processImpl(entt::registry& r) noexcept = 0;
    virtual SystemId id() const noexcept = 0;
    virtual ~ISystem() = default;
};

template<class... Components>
requires std::conjunction_v<std::is_base_of<component_base, Components>...> 
struct Read {};

template<class... Components> 
requires std::conjunction_v<std::is_base_of<component_base, Components>...> 
struct Write {};

template<class... Components>
requires std::conjunction_v<std::is_base_of<component_base, Components>...>
struct Exclude {};

template<class... Systems>
requires std::conjunction_v<std::is_base_of<ISystem, Systems>...>
struct Dependency {}; 

namespace detail {

template<class>
void _system_id_fn() {}

template<class Needles, class... Haystack>
struct check_components;

template<class... Ns, class... Haystack>
struct check_components<meta::sink<Ns...>, Haystack...> {
    static constexpr bool value = std::conjunction_v<meta::is_in<std::remove_reference_t<Ns>, Haystack...>...>;
};

template<class Sink>
struct strip_entity;

template<class E, class... Ts>
struct strip_entity<meta::sink<E, Ts...>> {
    using entity_t = std::conditional_t<std::is_same_v<entt::entity, std::remove_cvref_t<E>>, meta::sink<E>, meta::sink<>>;
    using args_t = std::conditional_t<std::is_same_v<entt::entity, std::remove_cvref_t<E>>, meta::sink<Ts...>, meta::sink<E, Ts...>>;
};

template<class T, class... Us>
inline constexpr bool check_components_v = check_components<T, Us...>::value;

template<class System, class View>
concept process_view = requires(System&& s, View const& v) {
    s.process(v);
};

template<class T>
struct InternalSystemId { 
    using id_type = void(*)();
    static constexpr id_type id = &detail::_system_id_fn<T>;
};

} // namespace detail

using SystemDependencyView = decltype(std::views::all(std::declval<std::array<SystemId, 1> const&>()));
 
template<class B, class R = Read<>, class W = Write<>, class E = Exclude<>, class D = Dependency<>>
struct SystemBase;

template<class Base, class... Rs, class... Ws, class... Es, class... Ds>
class SystemBase<Base, Read<Rs...>, Write<Ws...>, Exclude<Es...>, Dependency<Ds...>> : public ISystem {
public:
    using entities_view = decltype(std::declval<entt::registry>().view<std::add_const_t<Rs>..., Ws...>(entt::exclude<Es...>));
    // TODO: using entities_group

private:
    static constexpr detail::InternalSystemId<Base> id_{};
    static constexpr std::array<SystemId, sizeof...(Ds)> deps_ = {Ds::staticId()...};

public:
    static constexpr SystemId staticId() noexcept {
        return id_.id;
    }

    constexpr SystemId id() const noexcept final {
        return staticId();
    }

    static constexpr std::size_t numDependencies() noexcept {
        return sizeof...(Ds);
    }

    static constexpr auto getDependencies() noexcept {
        return std::views::all(deps_);
    }

    static constexpr auto getView(entt::registry& r) noexcept {
        return r.view<std::add_const_t<Rs>..., Ws...>(entt::exclude<Es...>);
    }

    template<class B = Base>
    requires (meta::has_process_mem_fn_v<B> && detail::process_view<B&, entities_view>)
    constexpr void crtpProcess(entt::registry& r) noexcept {
        static_cast<B&>(*this).process(getView(r));
    }

    template<class B = Base>
    requires (meta::has_process_mem_fn_v<B> && detail::process_view<B const&, entities_view>)
    constexpr void crtpProcess(entt::registry& r) const noexcept {
        static_cast<B const&>(*this).process(getView(r));
    }

    template<class B = Base, class... MaybeEntity, class... Args, class... Ignore>
    constexpr void crtpProcessComponents(entt::registry& r, meta::sink<MaybeEntity...>, meta::sink<Args...>, meta::sink<Ignore...>) noexcept {
        r.view<std::remove_reference_t<Args>..., Ignore...>(entt::exclude<Es...>).each([this](MaybeEntity... e, Args... args, auto&&...) {
            static_cast<B&>(*this).process(std::forward<decltype(e)>(e)..., std::forward<decltype(args)>(args)...);
        });
    }

    template<class B = Base, class... MaybeEntity, class... Args, class... Ignore>
    constexpr void crtpProcessComponents(entt::registry& r, meta::sink<MaybeEntity...>, meta::sink<Args...>, meta::sink<Ignore...>) const noexcept {
        r.view<std::remove_reference_t<Args>..., Ignore...>(entt::exclude<Es...>).each([this](MaybeEntity... e, Args... args, auto&&...) {
            static_cast<B const&>(*this).process(std::forward<decltype(e)>(e)..., std::forward<decltype(args)>(args)...);
        });
    }

    template<class B = Base>
    requires (meta::has_process_mem_fn_v<B> && !detail::process_view<B&, entities_view>)
    constexpr void crtpProcess(entt::registry& r) noexcept {
        
        using process_args = typename meta::mem_fn_traits<decltype(&B::process)>::args_t;
        using entity_arg = typename detail::strip_entity<process_args>::entity_t;
        using view_args = typename detail::strip_entity<process_args>::args_t;
        using ignore_args = meta::missing_types_t<meta::sink_remove_reference_t<view_args>, std::add_const_t<Rs>..., Ws...>;

        static_assert(detail::check_components_v<view_args, std::add_const_t<Rs>..., Ws...>, 
            "The components accessed within `process` must be present in the Read<> and Write<> template arguments. "
            "Read<> reference arguments must be const-quailfied. Write<> reference arguments cannot be const-qualified. "
            "If the entity id is desired, it must be the first argument.");

        crtpProcessComponents(r, entity_arg{}, view_args{}, ignore_args{});
    }

    template<class B = Base>
    requires (meta::has_process_mem_fn_v<B> && !detail::process_view<B const&, entities_view>)
    constexpr void crtpProcess(entt::registry& r) const noexcept {
        
        using process_args = typename meta::mem_fn_traits<decltype(&B::process)>::args_t;
        using entity_arg = typename detail::strip_entity<process_args>::entity_t;
        using view_args = typename detail::strip_entity<process_args>::args_t;
        using ignore_args = meta::missing_types_t<meta::sink_remove_reference_t<view_args>, std::add_const_t<Rs>..., Ws...>;

        static_assert(detail::check_components_v<view_args, std::add_const_t<Rs>..., Ws...>, 
            "The components accessed within `process` must be present in the Read<> and Write<> template arguments. "
            "Read<> reference arguments must be const-quailfied. Write<> reference arguments cannot be const-qualified. "
            "If the entity id is desired, it must be the first argument.");

        crtpProcessComponents(r, entity_arg{}, view_args{}, ignore_args{});
    }

    constexpr void processImpl(entt::registry& r) noexcept final {
        crtpProcess(r);
    }

    constexpr void processImpl(entt::registry& r) const noexcept final {
        crtpProcess(r);
    }
};

} // namespace nova