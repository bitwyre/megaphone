cmake_minimum_required(VERSION 3.22)

# Fetching all the deps
include(FetchContent)
if (MEGAPHONE_ENABLE_TESTS)
# Fetch dependancy: Catch2
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.4.0
)
endif()

# Fetch dependancy: Spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG        v1.12.0
)

# Fetch dependancy: Zenoh -> Zenoh-pico
FetchContent_declare(
    zenohpico_backend
    GIT_REPOSITORY https://github.com/eclipse-zenoh/zenoh-pico
    GIT_TAG 7ea1bcc
)

# Fetch dependancy: Zenoh -> Zenoh-C
FetchContent_declare(
    zenohc_backend
    GIT_REPOSITORY https://github.com/eclipse-zenoh/zenoh-c
    GIT_TAG 14133431b7a82743c0fd05ae51f5ff87c858cd02
)

# Fetch dependancy: Zenoh
FetchContent_Declare(
    cpp_wrapper
    GIT_REPOSITORY https://github.com/eclipse-zenoh/zenoh-cpp.git
    GIT_TAG        770aa2f
    GIT_SUBMODULES ""
)

# Fetch dependancy: uWebSockets
FetchContent_Declare(
    uWebSockets_content
    GIT_REPOSITORY https://github.com/uNetworking/uWebSockets.git
    GIT_TAG v20.48.0
    GIT_SHALLOW ON
    GIT_SUBMODULES ""
)

# Fetch dependancy: uSockets -> uWebSockets
FetchContent_Declare(
    uSockets_content
    GIT_REPOSITORY https://github.com/uNetworking/uSockets.git
    GIT_TAG v0.8.6
    GIT_SHALLOW ON
    GIT_SUBMODULES ""
)

# Fetch dependancy: flatbuffers
set(FLATBUFFERS_BUILD_FLATC OFF)
FetchContent_Declare(
    flatbuffers
    GIT_REPOSITORY https://github.com/google/flatbuffers
    GIT_TAG        v23.5.26
)

# Fetch dependancy: rapidjson
set(RAPIDJSON_BUILD_DOC OFF)
set(RAPIDJSON_BUILD_TESTS OFF)
set(RAPIDJSON_BUILD_EXAMPLES OFF)
FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG        f9d5341
)

if (MEGAPHONE_ENABLE_TESTS)
    FetchContent_MakeAvailable(Catch2)
endif()
FetchContent_MakeAvailable(spdlog)
FetchContent_MakeAvailable(zenohc_backend)
FetchContent_MakeAvailable(zenohpico_backend)
FetchContent_MakeAvailable(cpp_wrapper)
FetchContent_MakeAvailable(uWebSockets_content)
FetchContent_MakeAvailable(uSockets_content)
FetchContent_MakeAvailable(flatbuffers)
FetchContent_GetProperties(rapidjson)

set_target_properties(zenohcxx_zenohpico PROPERTIES
  CXX_STANDARD 11  # Set the desired C++ standard
  CXX_STANDARD_REQUIRED YES
)

set_target_properties(zenohcxx_zenohc_lib PROPERTIES
  CXX_STANDARD 11  # Set the desired C++ standard
  CXX_STANDARD_REQUIRED YES
)

find_package(ZLIB REQUIRED)

if(NOT rapidjson_POPULATED)
    # Populate RapidJSON
    FetchContent_Populate(rapidjson)

    # And add the library
    add_library(rapidjson INTERFACE)
    target_include_directories(rapidjson INTERFACE "$<BUILD_INTERFACE:${rapidjson_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
endif()

file(GLOB_RECURSE USOCKETS_SOURCES ${usockets_content_SOURCE_DIR}/src/*.c)
add_library(uSockets ${USOCKETS_SOURCES})
target_include_directories(uSockets PUBLIC ${usockets_content_SOURCE_DIR}/src)
target_compile_definitions(uSockets PRIVATE LIBUS_NO_SSL)

add_library(uWebSockets INTERFACE)
target_include_directories(uWebSockets INTERFACE ${uwebsockets_content_SOURCE_DIR}/src/)
target_link_libraries(uWebSockets INTERFACE uSockets ${ZLIB_LIBRARIES})