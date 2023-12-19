cmake_minimum_required(VERSION 3.22)
project("Megaphone")

# Project options
option(USE_CLANG_TIDY OFF "An option to turn off clang-tidy")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Set project options
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MEGAPHONE_ENABLE_TESTS OFF "Enable Catch2 tests")
option(MEGAPHONE_ENABLE_ASAN OFF "Enable libasan")

include(cmake/FetchPackages.cmake)

add_library(shared INTERFACE)

file(GLOB_RECURSE LIBPHONE_SOURCES
    source/libphone/*.cpp
)
add_library(phone ${LIBPHONE_SOURCES})

file(GLOB_RECURSE MEGAPHONE_SOURCES
    source/megaphone/*.cpp
)
add_executable(megaphone ${MEGAPHONE_SOURCES})

# Set compiler flags
set(COMPILER_FLAGS
    # All the warnings
    -Wall -Wextra
    # Except for the ones that aren't mine
    -Wno-deprecated -Wno-unused-parameter -Wno-deprecated-declarations
    -Wno-macro-redefined
    # Stack pointer
    -fno-omit-frame-pointer
)

if(MEGAPHONE_ENABLE_ASAN)
    list(APPEND COMPILER_FLAGS
        # Add asan options
        -fsanitize=address -fsanitize=pointer-compare
        -fsanitize=pointer-subtract -fsanitize=float-divide-by-zero
        -fsanitize=float-cast-overflow
    )
endif()

target_compile_options(megaphone PUBLIC ${COMPILER_FLAGS})
target_link_options(megaphone PUBLIC ${COMPILER_FLAGS})

target_compile_options(phone PUBLIC ${COMPILER_FLAGS})
target_link_options(phone PUBLIC ${COMPILER_FLAGS})

# Add include dirs
target_include_directories(shared 
    INTERFACE source/flatbuffers/generated
    INTERFACE source/spscqueue
)
target_include_directories(megaphone PUBLIC source/megaphone)
target_include_directories(phone PUBLIC source/libphone)

# Link all the libs to each other
target_link_libraries(phone
    PUBLIC shared
    PUBLIC uWebSockets
    PUBLIC spdlog::spdlog
    PUBLIC rapidjson
    PUBLIC zenohcxx::zenohc::lib
    PUBLIC zenohcxx::zenohpico
    PUBLIC flatbuffers
)
target_link_libraries(megaphone PUBLIC phone)

# Turn clang-tidy off/on
if (USE_CLANG_TIDY)
    message("Using clang tidy")
    # Find clang tidy
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}" "-warnings-as-errors=true -checks=*,-llvmlibc-*,-google-*,-fuchsia-*,-android-*,-altera-*,-abseil-*,-boost-*,-objc-*,-openmp-*,-zircon-*,-misc-include-cleaner,-readability-convert-member-functions-to-static")

    set_target_properties(megaphone PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}"
    )
    set_target_properties(phone PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}"
    )
endif()

# Tests
if (MEGAPHONE_ENABLE_TESTS)
    file(GLOB_RECURSE TESTS_SOURCES
        tests/*.cpp
    )
    add_executable(phonetests ${TESTS_SOURCES})
    target_link_libraries(phonetests
        PRIVATE Catch2::Catch2WithMain
        PRIVATE phone
    )
endif()
