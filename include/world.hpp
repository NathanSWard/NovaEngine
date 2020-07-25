#pragma once

#include <chrono>
#include <concepts>
#include <ranges>
#include <vector>

#include "system.hpp"
#include "util.hpp"

namespace nova {

using Time = std::chrono::milliseconds;

struct SystemHandle {
    SystemId id;
    bool hasDependency;
};

class World {
    entt::registry reg_;

    struct dependent_system {
        std::unique_ptr<ISystem> system;
        SystemDependencyView deps;
    };

    std::vector<std::unique_ptr<ISystem>> independent_systems_;
    std::vector<dependent_system> depdendent_systems_;

public:
    template<class S>
    requires std::derived_from<S, ISystem>
    SystemHandle addSystem(std::unique_ptr<S> system) {
        auto const id = system->id();
        if constexpr (S::numDependencies() > 0) {
            NOVA_ASSERT(std::none_of(std::begin(depdendent_systems_), std::end(depdendent_systems_), 
                [id](auto const& sys) { return sys.system->id() == id; }));
            depdendent_systems_.emplace_back(std::move(system), S::getDependencies());
        }
        else { // no dependenceis
            NOVA_ASSERT(std::none_of(std::begin(independent_systems_), std::end(independent_systems_), 
                [id](auto const& sys) { return sys->id() == id; }));
            independent_systems_.push_back(std::move(system));
        }
        return {id, S::numDependencies() > 0};
    } 

    template<class S>
    std::unique_ptr<S> removeSystem() {
        auto const id = S::staticId();
        if constexpr (S::numDependencies() > 0) {
            auto const found = std::find_if(std::begin(depdendent_systems_), std::end(depdendent_systems_), 
                [id](auto const& sys) { return sys.system->id() == id; });
            NOVA_ASSERT(found != std::end(depdendent_systems_));
            auto ptr = found->system.release();
            depdendent_systems_.erase(found);
            return std::unique_ptr<S>{static_cast<S*>(ptr)};
        }
        else {
            auto const found = std::find_if(std::begin(independent_systems_), std::end(independent_systems_), 
                [id](auto const& sys) { return sys->id() == id; });
            NOVA_ASSERT(found != std::end(independent_systems_));
            auto ptr = found->release();
            independent_systems_.erase(found);
            return std::unique_ptr<S>{static_cast<S*>(ptr)};
        }
    }

    std::size_t numSystems() const noexcept {
        return independent_systems_.size() + depdendent_systems_.size();
    }
    
};

} // namespace nova