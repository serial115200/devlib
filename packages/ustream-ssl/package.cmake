# ustream-ssl 包配置
# OpenWrt SSL 库包装器，支持 OpenSSL/mbedTLS/wolfSSL

if(ENABLE_USTREAM_SSL)
    set(USTREAM_SSL_VERSION "latest")
    set(USTREAM_SSL_GIT_REPOSITORY "https://git.openwrt.org/project/ustream-ssl.git")
    set(USTREAM_SSL_GIT_TAG "master")

    # 读取配置文件
    set(PACKAGE_CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/config.json")
    if(EXISTS "${PACKAGE_CONFIG_FILE}")
        file(READ "${PACKAGE_CONFIG_FILE}" PACKAGE_CONFIG_JSON)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
            string(JSON USTREAM_SSL_VERSION ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "version")
            string(JSON USTREAM_SSL_GIT_REPOSITORY ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "repo")
            string(JSON USTREAM_SSL_GIT_TAG ERROR_VARIABLE json_error GET "${PACKAGE_CONFIG_JSON}" "hash")
        endif()
    endif()

    # SSL 后端选择：openssl, mbedtls, wolfssl
    option(MBEDTLS "Use mbedTLS instead of OpenSSL" OFF)
    option(WOLFSSL "Use wolfSSL instead of OpenSSL" OFF)
    
    # 查找 OpenSSL（默认后端）
    if(NOT MBEDTLS AND NOT WOLFSSL)
        find_package(OpenSSL REQUIRED)
        if(OpenSSL_FOUND)
            message(STATUS "Found OpenSSL: ${OPENSSL_VERSION}")
            message(STATUS "  Include: ${OPENSSL_INCLUDE_DIR}")
            message(STATUS "  Libraries: ${OPENSSL_LIBRARIES}")
        endif()
    endif()

    # 确保 libubox 已经添加
    if(NOT TARGET ubox)
        message(FATAL_ERROR "ustream-ssl requires libubox, but libubox target not found")
    endif()

    # 获取 libubox 源码目录（CPM 会设置 libubox_SOURCE_DIR）
    if(NOT libubox_SOURCE_DIR)
        message(FATAL_ERROR "libubox_SOURCE_DIR not found")
    endif()
    
    # ustream-ssl 的源码中 #include <libubox/ustream.h>
    # 所以需要设置父目录作为包含路径，让 libubox/ 子目录可以被找到
    # 我们需要创建一个符号链接或者设置正确的包含目录
    get_filename_component(LIBUBOX_PARENT_DIR "${libubox_SOURCE_DIR}" DIRECTORY)
    
    # 设置 ubox_include_dir 为 libubox 源码的父目录
    # 这样 #include <libubox/xxx.h> 就能找到 ${LIBUBOX_PARENT_DIR}/libubox/xxx.h
    set(ubox_include_dir "${LIBUBOX_PARENT_DIR}" CACHE PATH "libubox include directory" FORCE)
    set(ubox_library "ubox" CACHE STRING "libubox library target" FORCE)
    
    message(STATUS "Setting ubox_include_dir to: ${ubox_include_dir}")
    message(STATUS "Setting ubox_library to: ${ubox_library}")

    # ustream-ssl 使用自己的 CMakeLists.txt
    CPMAddPackage(
        NAME ustream-ssl
        VERSION ${USTREAM_SSL_VERSION}
        GIT_REPOSITORY ${USTREAM_SSL_GIT_REPOSITORY}
        GIT_TAG ${USTREAM_SSL_GIT_TAG}
    )

    if(ustream-ssl_ADDED)
        message(STATUS "ustream-ssl package added: ${ustream-ssl_SOURCE_DIR}")
        if(MBEDTLS)
            message(STATUS "ustream-ssl backend: mbedTLS")
        elseif(WOLFSSL)
            message(STATUS "ustream-ssl backend: wolfSSL")
        else()
            message(STATUS "ustream-ssl backend: OpenSSL")
        endif()
        message(STATUS "ustream-ssl will be installed to ${CMAKE_INSTALL_PREFIX}")
    endif()
endif()

