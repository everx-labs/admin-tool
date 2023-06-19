cmake_minimum_required(VERSION 3.20)
project(admin-tool)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(JSON_BLOCK_BUILD_FLAG)
    set(JSON_BLOCK_TARGET_PATH debug)
else ()
    set(JSON_BLOCK_BUILD_FLAG --release)
    set(JSON_BLOCK_TARGET_PATH release)
endif ()

add_subdirectory(third-party/fmt)
add_subdirectory(third-party/ton)
add_subdirectory(third-party/json)
add_subdirectory(third-party/curlpp)

set(TON_ARCH "${TON_ARCH}")
set(TON_USE_ROCKSDB "${TON_USE_ROCKSDB}")

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
