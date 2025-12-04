cmake_minimum_required(VERSION 3.15)
project(devlib VERSION 1.0.0 LANGUAGES C CXX)

# 设置 C 标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置安装路径为 rootfs
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/rootfs/usr" CACHE PATH "Installation directory")
# 显示安装路径
message(STATUS "Packages will be installed to: ${CMAKE_INSTALL_PREFIX}")

# 设置公共下载目录（所有包共享）
set(DEVLIB_DOWNLOAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/download")
get_filename_component(DEVLIB_DOWNLOAD_DIR "${DEVLIB_DOWNLOAD_DIR}" ABSOLUTE)

# 统一 CPM 源缓存目录，避免每个包单独指定 SOURCE_DIR
set(CPM_SOURCE_CACHE "${DEVLIB_DOWNLOAD_DIR}")
set(CPM_LOCAL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake")
if(EXISTS "${CPM_LOCAL_FILE}")
    include(${CPM_LOCAL_FILE})
else()
    message(FATAL_ERROR "未找到 CPM.cmake，请确保 cmake/CPM.cmake 文件存在")
endif()

include(config/packages.cmake)

# 包含包配置
include(packages/json-c/package.cmake)
include(packages/libubox/package.cmake)
include(packages/llhttp/package.cmake)
include(packages/linenoise/package.cmake)
include(packages/ustream-ssl/package.cmake)

option(BUILD_USERVER "Build userver service" ON)
if(BUILD_USERVER)
    add_subdirectory(userver)
endif()
