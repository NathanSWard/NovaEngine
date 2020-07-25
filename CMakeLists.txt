cmake_minimum_required(VERSION 3.2)
project(Nova CXX)

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

include_directories(deps/entt/include/)

add_subdirectory(deps/SDL2pp)
add_subdirectory(deps/spdlog)
add_subdirectory(deps/fmt)

find_package(sdl2pp REQUIRED)
find_package(fmt REQUIRED)

add_executable(nova src/main.cpp)

target_link_libraries(nova PRIVATE sdl2pp)
target_link_libraries(nova PRIVATE spdlog::spdlog)
target_link_libraries(nova PRIVATE fmt::fmt)