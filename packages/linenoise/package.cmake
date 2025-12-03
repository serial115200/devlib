# linenoise 包配置
# 使用 CPM 管理 linenoise，从 Git 仓库 clone
# linenoise 是单文件库，没有 CMakeLists.txt，使用 CPM 下载后手动创建目标

# 使用 CMake option 控制是否启用
if(ENABLE_LINENOISE)
    # 读取包详细配置文件（repo、hash、version等）
    set(PACKAGE_CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/config.json")
    if(EXISTS "${PACKAGE_CONFIG_FILE}")
        file(READ "${PACKAGE_CONFIG_FILE}" PACKAGE_CONFIG_JSON)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
            string(JSON LINENOISE_VERSION ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "version")
            string(JSON LINENOISE_GIT_REPOSITORY ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "repo")
            string(JSON LINENOISE_GIT_TAG ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "hash")
        endif()
    else()
        # 默认值
        set(LINENOISE_VERSION "latest")
        set(LINENOISE_GIT_REPOSITORY "https://github.com/antirez/linenoise.git")
        set(LINENOISE_GIT_TAG "master")
    endif()
    
    # 使用公共下载目录
    set(LINENOISE_SOURCE_DIR "${DEVLIB_DOWNLOAD_DIR}/linenoise")
    # 确保下载目录存在
    file(MAKE_DIRECTORY "${DEVLIB_DOWNLOAD_DIR}")
    
    message(STATUS "linenoise will be cloned to: ${LINENOISE_SOURCE_DIR}")
    
    # 使用 CPM 下载 linenoise（即使没有 CMakeLists.txt，CPM 也可以下载源码）
    CPMAddPackage(
        NAME linenoise
        VERSION ${LINENOISE_VERSION}
        GIT_REPOSITORY ${LINENOISE_GIT_REPOSITORY}
        GIT_TAG ${LINENOISE_GIT_TAG}
        SOURCE_DIR ${LINENOISE_SOURCE_DIR}
        DOWNLOAD_ONLY YES  # 只下载，不尝试配置/构建（因为没有 CMakeLists.txt）
    )
    
    # 如果下载成功，手动创建目标
    if(linenoise_ADDED AND NOT TARGET linenoise)
        # 创建静态库目标
        add_library(linenoise STATIC
            ${linenoise_SOURCE_DIR}/linenoise.c
        )
        
        # 设置包含目录
        target_include_directories(linenoise PUBLIC
            ${linenoise_SOURCE_DIR}
        )
        
        # 设置 C 标准
        set_target_properties(linenoise PROPERTIES
            C_STANDARD 99
            C_STANDARD_REQUIRED ON
        )
        
        # 安装规则
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

