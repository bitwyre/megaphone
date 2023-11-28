cmake_minimum_required(VERSION 3.22)

# Fetching all the deps
include(FetchContent)
# Fetch dependancy: Catch2
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.4.0
)

# Fetch dependancy: Spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG        v1.12.0
)

# Fetch dependancy: Zenoh
FetchContent_Declare(
    zenoh-cpp
    GIT_REPOSITORY https://github.com/eclipse-zenoh/zenoh-cpp.git
    GIT_TAG        c9f4d0e
)

FetchContent_Declare(
    uWebSockets_content
    GIT_REPOSITORY https://github.com/uNetworking/uWebSockets
    GIT_TAG v20.48.0
    GIT_SHALLOW ON
    GIT_SUBMODULES ""
)

FetchContent_Declare(
    uSockets_content
    GIT_REPOSITORY https://github.com/uNetworking/uSockets
    GIT_TAG v0.8.6
    GIT_SHALLOW ON
    GIT_SUBMODULES ""
)

FetchContent_MakeAvailable(Catch2)
FetchContent_MakeAvailable(spdlog)
FetchContent_MakeAvailable(zenoh-cpp)
FetchContent_MakeAvailable(uWebSockets_content)
FetchContent_MakeAvailable(uSockets_content)

find_package(ZLIB REQUIRED)

file(GLOB_RECURSE USOCKETS_SOURCES ${usockets_content_SOURCE_DIR}/src/*.c)
add_library(uSockets ${USOCKETS_SOURCES})
target_include_directories(uSockets PUBLIC ${usockets_content_SOURCE_DIR}/src)
target_compile_definitions(uSockets PRIVATE LIBUS_NO_SSL)

add_library(uWebSockets INTERFACE)
target_include_directories(uWebSockets INTERFACE ${uwebsockets_content_SOURCE_DIR}/src/)
target_link_libraries(uWebSockets INTERFACE uSockets ${ZLIB_LIBRARIES})