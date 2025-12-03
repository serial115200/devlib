# libubox 包配置
# 使用 CPM 管理 libubox，从 Git 仓库 clone

# 使用 CMake option 控制是否启用
if(ENABLE_LIBUBOX)
    # 默认值，兼容旧版 CMake
    set(LIBUBOX_VERSION "latest")
    set(LIBUBOX_GIT_REPOSITORY "https://git.openwrt.org/project/libubox.git")
    set(LIBUBOX_GIT_TAG "master")

    set(PACKAGE_CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/config.json")
    if(EXISTS "${PACKAGE_CONFIG_FILE}")
        file(READ "${PACKAGE_CONFIG_FILE}" PACKAGE_CONFIG_JSON)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
            string(JSON LIBUBOX_VERSION ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "version")
            string(JSON LIBUBOX_GIT_REPOSITORY ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "repo")
            string(JSON LIBUBOX_GIT_TAG ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "hash")
        endif()
    endif()

    set(LIBUBOX_OPTIONS
        "BUILD_SHARED_LIBS ON"
        "BUILD_LUA OFF"
        "BUILD_EXAMPLES OFF"
    )

    if(TARGET json-c)
        if(DEFINED json-c_BINARY_DIR AND EXISTS "${json-c_BINARY_DIR}")
            set(ENV{PKG_CONFIG_PATH} "${json-c_BINARY_DIR}:$ENV{PKG_CONFIG_PATH}")
            message(STATUS "Set PKG_CONFIG_PATH to find json-c: ${json-c_BINARY_DIR}")
        endif()
        if(EXISTS "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
            set(ENV{PKG_CONFIG_PATH} "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
        endif()
    endif()

    CPMAddPackage(
        NAME libubox
        VERSION ${LIBUBOX_VERSION}
        GIT_REPOSITORY ${LIBUBOX_GIT_REPOSITORY}
        GIT_TAG ${LIBUBOX_GIT_TAG}
        OPTIONS ${LIBUBOX_OPTIONS}
    )

    if(libubox_ADDED)
        message(STATUS "libubox package added: ${libubox_SOURCE_DIR}")
        message(STATUS "libubox will be built using CMake (LUA and EXAMPLES disabled)")
        message(STATUS "libubox will be installed to ${CMAKE_INSTALL_PREFIX}")
    endif()
endif()
