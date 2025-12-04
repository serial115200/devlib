cmake_minimum_required(VERSION 3.15)
project(userver VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/DevlibRootfs.cmake)

add_executable(userver
    src/main.c
    src/http.c
    src/http_json.c
    src/http_form.c
)

set(ROOTFS_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rootfs/usr/include")
set(ROOTFS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rootfs/usr/lib")

target_include_directories(userver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ROOTFS_INC_DIR}
)

target_link_libraries(userver
    ${ROOTFS_LIB_DIR}/libubox.a
    ${ROOTFS_LIB_DIR}/libllhttp.a
    ${ROOTFS_LIB_DIR}/libjson-c.a
    ${ROOTFS_LIB_DIR}/libustream-ssl.so
    ssl
    crypto
)

if(UNIX)
    target_link_libraries(userver rt)
endif()

install(TARGETS userver RUNTIME DESTINATION bin)
