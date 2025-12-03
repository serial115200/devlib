# libubox 包配置
# 使用 CPM 管理 libubox，从 Git 仓库 clone

# 使用 CMake option 控制是否启用
if(ENABLE_LIBUBOX)
    # 读取包详细配置文件（repo、hash、version等）
    set(PACKAGE_CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/config.json")
    if(EXISTS "${PACKAGE_CONFIG_FILE}")
        file(READ "${PACKAGE_CONFIG_FILE}" PACKAGE_CONFIG_JSON)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
            string(JSON LIBUBOX_VERSION ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "version")
            string(JSON LIBUBOX_GIT_REPOSITORY ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "repo")
            string(JSON LIBUBOX_GIT_TAG ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "hash")
        endif()
    else()
        # 默认值
        set(LIBUBOX_VERSION "latest")
        set(LIBUBOX_GIT_REPOSITORY "https://git.openwrt.org/project/libubox.git")
        set(LIBUBOX_GIT_TAG "master")
    endif()
    
    # 使用公共下载目录
    set(LIBUBOX_SOURCE_DIR "${DEVLIB_DOWNLOAD_DIR}/libubox")
    # 确保下载目录存在
    file(MAKE_DIRECTORY "${DEVLIB_DOWNLOAD_DIR}")
    
    message(STATUS "libubox will be cloned to: ${LIBUBOX_SOURCE_DIR}")
    
    # libubox 使用 CMake 构建
    # 设置构建选项：禁用 lua 和 examples
    set(LIBUBOX_OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "BUILD_LUA OFF"
        "BUILD_EXAMPLES OFF"
    )
    
    # 在配置 libubox 之前，如果 json-c 目标存在，设置变量让 pkg-config 能找到它
    # 这样 libubox 的 CMakeLists.txt 中的 PKG_SEARCH_MODULE 就能找到 json-c
    if(TARGET json-c)
        # json-c 的构建目录（pkg-config 文件在这里生成）
        set(JSON_C_BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/json-c-build")
        
        # 设置 PKG_CONFIG_PATH 包含 json-c 的构建目录
        if(EXISTS "${JSON_C_BINARY_DIR}")
            set(ENV{PKG_CONFIG_PATH} "${JSON_C_BINARY_DIR}:$ENV{PKG_CONFIG_PATH}")
        endif()
        
        # 也添加到安装目录（如果已安装）
        if(EXISTS "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
            set(ENV{PKG_CONFIG_PATH} "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
        endif()
        
        message(STATUS "Set PKG_CONFIG_PATH to find json-c: ${JSON_C_BINARY_DIR}")
    endif()
    
    # 使用 CPM 添加 libubox
    CPMAddPackage(
        NAME libubox
        VERSION ${LIBUBOX_VERSION}
        GIT_REPOSITORY ${LIBUBOX_GIT_REPOSITORY}
        GIT_TAG ${LIBUBOX_GIT_TAG}
        SOURCE_DIR ${LIBUBOX_SOURCE_DIR}
        OPTIONS ${LIBUBOX_OPTIONS}
    )
    
    if(libubox_ADDED)
        message(STATUS "libubox package added: ${libubox_SOURCE_DIR}")
        message(STATUS "libubox will be built using CMake (LUA and EXAMPLES disabled)")
        message(STATUS "libubox will be installed to ${CMAKE_INSTALL_PREFIX}")
    endif()
endif()

