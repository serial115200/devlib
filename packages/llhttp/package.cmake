# llhttp 包配置
# 使用 CPM 管理 llhttp，从 Git 仓库 clone

# 使用 CMake option 控制是否启用
if(ENABLE_LLHTTP)
    # 默认值，兼容旧版 CMake
    set(LLHTTP_VERSION "8.1.0")
    set(LLHTTP_GIT_REPOSITORY "https://github.com/nodejs/llhttp.git")
    set(LLHTTP_GIT_TAG "release/v8.1.0")

    set(PACKAGE_CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/config.json")
    if(EXISTS "${PACKAGE_CONFIG_FILE}")
        file(READ "${PACKAGE_CONFIG_FILE}" PACKAGE_CONFIG_JSON)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
            string(JSON LLHTTP_VERSION ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "version")
            string(JSON LLHTTP_GIT_REPOSITORY ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "repo")
            string(JSON LLHTTP_GIT_TAG ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "hash")
        endif()
    endif()

    set(LLHTTP_OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "BUILD_STATIC_LIBS ON"
    )

    CPMAddPackage(
        NAME llhttp
        VERSION ${LLHTTP_VERSION}
        GIT_REPOSITORY ${LLHTTP_GIT_REPOSITORY}
        GIT_TAG ${LLHTTP_GIT_TAG}
        OPTIONS ${LLHTTP_OPTIONS}
    )

    if(llhttp_ADDED)
        message(STATUS "llhttp package added: ${llhttp_SOURCE_DIR}")
        message(STATUS "llhttp will be built using CMake")
        message(STATUS "llhttp will be installed to ${CMAKE_INSTALL_PREFIX}")
    endif()
endif()
