# DevlibRootfs.cmake
# 自动将 rootfs 目录加入 CMake 的搜索路径
# 用于不在 packages 目录下管理的项目（如 userver、example 等）
# 让这些项目能够找到并链接到已安装到 rootfs 中的库

# 获取 rootfs 目录的绝对路径
get_filename_component(DEVLIB_ROOTFS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rootfs" ABSOLUTE)

if(EXISTS "${DEVLIB_ROOTFS_DIR}")
    message(STATUS "找到 rootfs 目录: ${DEVLIB_ROOTFS_DIR}")
    
    # 添加 include 目录
    if(EXISTS "${DEVLIB_ROOTFS_DIR}/usr/include")
        include_directories("${DEVLIB_ROOTFS_DIR}/usr/include")
        message(STATUS "添加 include 路径: ${DEVLIB_ROOTFS_DIR}/usr/include")
    elseif(EXISTS "${DEVLIB_ROOTFS_DIR}/include")
        include_directories("${DEVLIB_ROOTFS_DIR}/include")
        message(STATUS "添加 include 路径: ${DEVLIB_ROOTFS_DIR}/include")
    endif()
    
    # 添加库目录
    if(EXISTS "${DEVLIB_ROOTFS_DIR}/usr/lib")
        link_directories("${DEVLIB_ROOTFS_DIR}/usr/lib")
        message(STATUS "添加库路径: ${DEVLIB_ROOTFS_DIR}/usr/lib")
    elseif(EXISTS "${DEVLIB_ROOTFS_DIR}/lib")
        link_directories("${DEVLIB_ROOTFS_DIR}/lib")
        message(STATUS "添加库路径: ${DEVLIB_ROOTFS_DIR}/lib")
    endif()
    
    # 设置 CMAKE_PREFIX_PATH，让 find_package 能找到 rootfs 中的包
    if(EXISTS "${DEVLIB_ROOTFS_DIR}/usr")
        list(APPEND CMAKE_PREFIX_PATH "${DEVLIB_ROOTFS_DIR}/usr")
    else()
        list(APPEND CMAKE_PREFIX_PATH "${DEVLIB_ROOTFS_DIR}")
    endif()
    
    # 设置 PKG_CONFIG_PATH（如果使用 pkg-config）
    if(EXISTS "${DEVLIB_ROOTFS_DIR}/usr/lib/pkgconfig")
        set(ENV{PKG_CONFIG_PATH} "${DEVLIB_ROOTFS_DIR}/usr/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    elseif(EXISTS "${DEVLIB_ROOTFS_DIR}/lib/pkgconfig")
        set(ENV{PKG_CONFIG_PATH} "${DEVLIB_ROOTFS_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    endif()
else()
    message(WARNING "rootfs 目录不存在: ${DEVLIB_ROOTFS_DIR}")
endif()

