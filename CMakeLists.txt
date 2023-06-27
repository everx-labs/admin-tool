cmake_minimum_required(VERSION 3.16)
project(admin-tool)

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --recommend-shallow
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMOD_RESULT
        )
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

find_package(Python COMPONENTS Interpreter REQUIRED)
execute_process(
    COMMAND ${Python_EXECUTABLE} ${PROJECT_SOURCE_DIR}/generate.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(JSON_BLOCK_BUILD_FLAG)
    set(JSON_BLOCK_TARGET_PATH debug)
else ()
    set(JSON_BLOCK_BUILD_FLAG --release)
    set(JSON_BLOCK_TARGET_PATH release)
endif ()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (NOT MSVC)
  set(CMAKE_CXX_FLAGS "-Os -Wall -Wextra")
endif()

set(TON_USE_ROCKSDB "OFF")

add_subdirectory(third-party/fmt)
add_subdirectory(third-party/ton)
add_subdirectory(third-party/json)
add_subdirectory(third-party/curlpp)

set(TON_ARCH "${TON_ARCH}")

include_directories(third-party/json/include)

include_directories(json-block/include)
link_directories(json-block/target/${JSON_BLOCK_TARGET_PATH})
add_custom_target(json_block_target
    COMMAND cargo build ${JSON_BLOCK_BUILD_FLAG}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/json-block
)

file(GLOB SOURCES
    gql-client/*.hpp
    gql-client/*.cpp
)

add_executable(admin-tool main.cpp ${SOURCES})
add_dependencies(admin-tool json_block_target)

target_include_directories(admin-tool PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

target_link_libraries(
    admin-tool PRIVATE
    fmt::fmt libjson_block.a curlpp
    tdutils ton_crypto ton_block fift-lib
)
