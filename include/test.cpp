#include "system.hpp"
#include "world.hpp"

#include <iostream>

using namespace nova;

struct pos : component_base { float x, y; };
struct vel : component_base { float dx, dy; };

entt::registry reg;

struct sysA : SystemBase<sysA, Read<vel>, Write<pos>> {
    void process(entt::entity, vel const&) const noexcept {

    }
};

struct sysB : SystemBase<sysB, Read<pos>, Write<>, Exclude<>, Dependency<sysA>> {
    void process(entities_view const& view) const noexcept {

    }
};

int main() {
    World world;   
    world.addSystem(std::make_unique<sysA>());
    world.addSystem(std::make_unique<sysB>());
    auto ptrA = world.removeSystem<sysA>();
    auto ptrB = world.removeSystem<sysB>();
}