# linenoise 包配置
# 使用 CPM 管理 linenoise，从 Git 仓库 clone
# linenoise 是单文件库，没有 CMakeLists.txt，使用 CPM 下载后手动创建目标

# 使用 CMake option 控制是否启用
if(ENABLE_LINENOISE)
    # 默认值，兼容旧版 CMake
    set(LINENOISE_VERSION "latest")
    set(LINENOISE_GIT_REPOSITORY "https://github.com/antirez/linenoise.git")
    set(LINENOISE_GIT_TAG "master")

    set(PACKAGE_CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/config.json")
    if(EXISTS "${PACKAGE_CONFIG_FILE}")
        file(READ "${PACKAGE_CONFIG_FILE}" PACKAGE_CONFIG_JSON)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
            string(JSON LINENOISE_VERSION ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "version")
            string(JSON LINENOISE_GIT_REPOSITORY ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "repo")
            string(JSON LINENOISE_GIT_TAG ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "hash")
        endif()
    endif()

    CPMAddPackage(
        NAME linenoise
        VERSION ${LINENOISE_VERSION}
        GIT_REPOSITORY ${LINENOISE_GIT_REPOSITORY}
        GIT_TAG ${LINENOISE_GIT_TAG}
        DOWNLOAD_ONLY YES
    )

    if(linenoise_ADDED AND NOT TARGET linenoise)
        add_library(linenoise STATIC ${linenoise_SOURCE_DIR}/linenoise.c)
        
        target_include_directories(linenoise PUBLIC
            ${linenoise_SOURCE_DIR}
        )
        
        set_target_properties(linenoise PROPERTIES
            C_STANDARD 99
            C_STANDARD_REQUIRED ON
        )
        
        install(TARGETS linenoise
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin
        )
        
        install(FILES ${linenoise_SOURCE_DIR}/linenoise.h
            DESTINATION include
        )
        
        message(STATUS "linenoise target created: ${linenoise_SOURCE_DIR}")
        message(STATUS "linenoise will be installed to ${CMAKE_INSTALL_PREFIX}")
    endif()
endif()
