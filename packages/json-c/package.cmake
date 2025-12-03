# json-c 包配置
# 使用 CPM 管理 json-c，从 Git 仓库 clone

# 使用 CMake option 控制是否启用
if(ENABLE_JSON_C)
    # 先设置默认值，确保旧版 CMake (<3.19) 也有有效参数
    set(JSON_C_VERSION "0.18")
    set(JSON_C_GIT_REPOSITORY "https://github.com/json-c/json-c.git")
    set(JSON_C_GIT_TAG "json-c-0.18")

    set(PACKAGE_CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/config.json")
    if(EXISTS "${PACKAGE_CONFIG_FILE}")
        file(READ "${PACKAGE_CONFIG_FILE}" PACKAGE_CONFIG_JSON)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
            string(JSON JSON_C_VERSION ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "version")
            string(JSON JSON_C_GIT_REPOSITORY ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "repo")
            string(JSON JSON_C_GIT_TAG ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "hash")
        endif()
    endif()

    CPMAddPackage(
        NAME json-c
        VERSION ${JSON_C_VERSION}
        GIT_REPOSITORY ${JSON_C_GIT_REPOSITORY}
        GIT_TAG ${JSON_C_GIT_TAG}
        OPTIONS
            "BUILD_SHARED_LIBS ON"
            "BUILD_TESTING OFF"
            "DISABLE_WERROR ON"
    )
endif()

if(json-c_ADDED)
    message(STATUS "json-c package added: ${json-c_SOURCE_DIR}")
    if(TARGET json-c)
        set_target_properties(json-c PROPERTIES
            INSTALL_RPATH_USE_LINK_PATH TRUE
        )
    endif()
endif()
