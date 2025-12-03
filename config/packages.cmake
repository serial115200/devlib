# 包启用配置
# 可以通过 cmake -D 参数或 ccmake 修改

# 聚合开关：使用已安装的 rootfs 依赖，而不在本工程内构建
option(USE_ROOTFS "Use dependencies from installed rootfs" OFF)

# 根据 USE_ROOTFS 一次性设置包启用状态
if(USE_ROOTFS)
    set(ENABLE_JSON_C OFF)
    set(ENABLE_LIBUBOX OFF)
    set(ENABLE_LLHTTP OFF)
    set(ENABLE_LINENOISE OFF)
    message(STATUS "Using rootfs dependencies, package building disabled")
else()
    # 仅在不使用 rootfs 时允许用户配置各个包
    option(ENABLE_JSON_C        "Enable json-c package"         ON)
    option(ENABLE_LIBUBOX       "Enable libubox package"        ON)
    option(ENABLE_LLHTTP        "Enable llhttp package"         ON)
    option(ENABLE_LINENOISE     "Enable linenoise package"      ON)
endif()
