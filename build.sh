#!/bin/bash

# CPM 构建脚本
set -e

# 颜色输出
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Devlib 构建脚本 (CPM) ===${NC}"

# 处理代理设置
if [ -n "$all_proxy" ] || [ -n "$ALL_PROXY" ]; then
    PROXY="${all_proxy:-$ALL_PROXY}"
    echo -e "${BLUE}检测到代理: ${PROXY}${NC}"
    
    # 如果是 SOCKS5 代理，CMake 可能不支持，需要转换为 HTTP 或使用其他方法
    if [[ "$PROXY" =~ ^socks5 ]]; then
        echo -e "${YELLOW}注意: SOCKS5 代理，CMake 可能无法直接使用${NC}"
        echo -e "${YELLOW}建议设置 HTTP_PROXY 和 HTTPS_PROXY 环境变量${NC}"
        # 尝试从 SOCKS5 代理提取地址，假设有 HTTP 代理在同一地址
        if [[ "$PROXY" =~ socks5h?://([^:]+):([0-9]+) ]]; then
            PROXY_HOST="${BASH_REMATCH[1]}"
            PROXY_PORT="${BASH_REMATCH[2]}"
            # 如果 SOCKS5 代理服务器也提供 HTTP 代理，可以尝试
            # 否则需要配置代理转换工具
            echo -e "${BLUE}提示: 如果代理服务器支持 HTTP，可以设置:${NC}"
            echo -e "${BLUE}  export HTTP_PROXY=\"http://${PROXY_HOST}:${PROXY_PORT}\"${NC}"
            echo -e "${BLUE}  export HTTPS_PROXY=\"http://${PROXY_HOST}:${PROXY_PORT}\"${NC}"
        fi
    else
        # HTTP/HTTPS 代理，直接导出
        export HTTP_PROXY="$PROXY"
        export HTTPS_PROXY="$PROXY"
        export http_proxy="$PROXY"
        export https_proxy="$PROXY"
        export ALL_PROXY="$PROXY"
        echo -e "${GREEN}已设置 HTTP/HTTPS 代理${NC}"
    fi
fi

# 检查 CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${YELLOW}错误: 未找到 CMake，请先安装 CMake${NC}"
    echo "安装命令: sudo apt install cmake"
    exit 1
fi

# 创建构建目录和 rootfs 目录
BUILD_DIR="build"
ROOTFS_DIR="rootfs"
mkdir -p ${BUILD_DIR}
mkdir -p ${ROOTFS_DIR}/usr

# 配置 CMake
# 包的启用状态通过 CMake 选项控制（config/packages.cmake）
# 可以通过 -D 参数覆盖，例如: -DENABLE_JSON_C=OFF
echo -e "${GREEN}配置 CMake...${NC}"
cd ${BUILD_DIR}
cmake .. -DCMAKE_INSTALL_PREFIX=../${ROOTFS_DIR}/usr

# 构建
echo -e "${GREEN}构建项目...${NC}"
cmake --build . --config Release

# 安装到 rootfs
echo -e "${GREEN}安装包到 rootfs 目录...${NC}"
cmake --install .

cd ..

echo -e "${GREEN}构建完成！包已安装到 ${ROOTFS_DIR}/usr 目录${NC}"
echo -e "${GREEN}rootfs 目录结构:${NC}"
if command -v tree &> /dev/null; then
    tree -L 3 ${ROOTFS_DIR} 2>/dev/null || ls -R ${ROOTFS_DIR}
else
    find ${ROOTFS_DIR} -type f 2>/dev/null | head -20 || ls -R ${ROOTFS_DIR}
fi

