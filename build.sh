#!/bin/bash

# CPM 构建脚本
set -e

# 颜色输出
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Devlib 构建脚本 (CPM) ===${NC}"

# 检查 CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${YELLOW}错误: 未找到 CMake，请先安装 CMake${NC}"
    echo "安装命令: sudo apt install cmake"
    exit 1
fi

# 创建目录变量（由 CMake 负责实际创建）
BUILD_SYS_DIR="build/sys"
BUILD_APP_DIR="build/app"
ROOTFS_DIR="rootfs"

# 阶段 1：仅构建并安装 packages 到 rootfs
echo -e "${GREEN}阶段 1：构建并安装 packages 到 rootfs...${NC}"
cmake -S . -B ${BUILD_SYS_DIR} -DCMAKE_INSTALL_PREFIX=${ROOTFS_DIR}/usr -DBUILD_USERVER=OFF
cmake --build ${BUILD_SYS_DIR} --config Release
cmake --install ${BUILD_SYS_DIR}

# 清理运行时不需要的 CMake 配置文件
rm -rf ${ROOTFS_DIR}/usr/lib/cmake ${ROOTFS_DIR}/usr/lib64/cmake ${ROOTFS_DIR}/usr/share/cmake 2>/dev/null || true

# 阶段 2：构建应用（示例：userver），可选择禁用 packages，直接使用 rootfs
echo -e "${GREEN}阶段 2：构建应用，使用已安装的 rootfs 依赖...${NC}"
cmake -S . -B ${BUILD_APP_DIR} -DCMAKE_INSTALL_PREFIX=${ROOTFS_DIR}/usr -DBUILD_USERVER=ON -DUSE_ROOTFS=ON
cmake --build ${BUILD_APP_DIR} --config Release
cmake --install ${BUILD_APP_DIR}

echo -e "${GREEN}构建完成！包与应用已安装到 ${ROOTFS_DIR}/usr 目录${NC}"
echo -e "${GREEN}rootfs 目录结构:${NC}"
if command -v tree &> /dev/null; then
    tree -L 3 ${ROOTFS_DIR} 2>/dev/null || ls -R ${ROOTFS_DIR}
else
    find ${ROOTFS_DIR} -type f 2>/dev/null | head -20 || ls -R ${ROOTFS_DIR}
fi
