cmake_minimum_required(VERSION 3.15)
project(example_project LANGUAGES C)

# 方法 1: 使用 DevlibRootfs.cmake（推荐）
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/DevlibRootfs.cmake)

# 设置 C 标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(example main.c)

# 使用 json-c 库
find_library(JSON_C_LIB json-c PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../rootfs/usr/lib)
if(JSON_C_LIB)
    target_link_libraries(example ${JSON_C_LIB})
    target_include_directories(example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../rootfs/usr/include)
    message(STATUS "找到 json-c 库: ${JSON_C_LIB}")
else()
    message(FATAL_ERROR "未找到 json-c 库，请先运行 ../build.sh")
endif()

