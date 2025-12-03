cmake_minimum_required(VERSION 3.15)
project(userver VERSION 1.0.0 LANGUAGES C)

# 设置 C 标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 包含 rootfs 路径配置
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/DevlibRootfs.cmake)

# 添加可执行文件
add_executable(userver
    src/main.c
    src/http_server.c
    src/http_server.h
)

# 链接库 - 使用 rootfs 中已安装的库
# libubox 提供 uloop, ustream, usock
set(ROOTFS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rootfs/usr/lib")

# 优先使用 CMake 目标（如果存在），否则使用库文件
if(TARGET ubox-static)
    target_link_libraries(userver ubox-static)
elseif(TARGET ubox)
    target_link_libraries(userver ubox)
else()
    target_link_libraries(userver ${ROOTFS_LIB_DIR}/libubox.a)
endif()

if(TARGET llhttp::llhttp)
    target_link_libraries(userver llhttp::llhttp)
elseif(TARGET llhttp_static)
    target_link_libraries(userver llhttp_static)
else()
    target_link_libraries(userver ${ROOTFS_LIB_DIR}/libllhttp.a)
endif()

if(TARGET json-c::json-c)
    target_link_libraries(userver json-c::json-c)
elseif(TARGET json-c)
    target_link_libraries(userver json-c)
else()
    target_link_libraries(userver ${ROOTFS_LIB_DIR}/libjson-c.a)
endif()

# 系统库
target_link_libraries(userver rt)

# 包含目录
target_include_directories(userver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../rootfs/usr/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../rootfs/usr/include/libubox
)

# 安装
install(TARGETS userver
    RUNTIME DESTINATION bin
)

