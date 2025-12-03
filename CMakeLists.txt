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

# 包含 CPM - 优先使用本地文件，如果不存在则自动下载
set(CPM_LOCAL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake")
if(EXISTS "${CPM_LOCAL_FILE}")
    message(STATUS "使用本地 CPM.cmake: ${CPM_LOCAL_FILE}")
    include(${CPM_LOCAL_FILE})
else()
    message(STATUS "本地 CPM.cmake 不存在，尝试自动下载...")
    # 如果本地文件不存在，可以在这里添加自动下载逻辑
    # 或者提示用户下载
    message(FATAL_ERROR "未找到 CPM.cmake，请确保 cmake/CPM.cmake 文件存在")
endif()

# 包含包启用配置（统一管理所有包的启用选项）
include(config/packages.cmake)

# 包含包配置
include(packages/json-c/package.cmake)
include(packages/libubox/package.cmake)
include(packages/llhttp/package.cmake)
include(packages/linenoise/package.cmake)

# 添加 userver 服务（第一个功能：HTTP JSON 服务）
add_subdirectory(userver)
