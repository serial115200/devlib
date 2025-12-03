#!/bin/bash

# 代理设置脚本
# 用于配置 CMake/CPM 下载时使用的代理

# 颜色输出
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== 代理设置 ===${NC}"

# 检查是否已设置代理
if [ -n "$all_proxy" ] || [ -n "$ALL_PROXY" ]; then
    PROXY="${all_proxy:-$ALL_PROXY}"
    echo -e "${GREEN}检测到代理设置: ${PROXY}${NC}"
    
    # 解析代理类型和地址
    if [[ "$PROXY" =~ ^(socks5h?|http|https)://(.+):([0-9]+)$ ]]; then
        PROXY_TYPE="${BASH_REMATCH[1]}"
        PROXY_HOST="${BASH_REMATCH[2]}"
        PROXY_PORT="${BASH_REMATCH[3]}"
        
        echo -e "${GREEN}代理类型: ${PROXY_TYPE}${NC}"
        echo -e "${GREEN}代理地址: ${PROXY_HOST}:${PROXY_PORT}${NC}"
        
        # CMake 和大多数工具只支持 HTTP/HTTPS 代理
        # 如果是 SOCKS5，需要转换为 HTTP 代理或使用其他方法
        if [[ "$PROXY_TYPE" =~ ^socks5 ]]; then
            echo -e "${YELLOW}警告: SOCKS5 代理可能不被 CMake 直接支持${NC}"
            echo -e "${YELLOW}建议: 使用 HTTP 代理或配置代理转换工具${NC}"
            echo ""
            echo -e "${BLUE}选项 1: 使用 HTTP 代理（如果可用）${NC}"
            echo -e "${BLUE}选项 2: 使用 proxychains 包装命令${NC}"
            echo -e "${BLUE}选项 3: 配置 Git 使用 SOCKS5 代理${NC}"
            echo ""
            
            # 尝试设置 Git 代理（Git 支持 SOCKS5）
            export GIT_PROXY_COMMAND="socat STDIO SOCKS5:${PROXY_HOST}:${PROXY_PORT}:%h:%p"
            echo -e "${GREEN}已设置 Git SOCKS5 代理${NC}"
        else
            # HTTP/HTTPS 代理，直接设置
            export HTTP_PROXY="$PROXY"
            export HTTPS_PROXY="$PROXY"
            export http_proxy="$PROXY"
            export https_proxy="$PROXY"
            export ALL_PROXY="$PROXY"
            echo -e "${GREEN}已设置 HTTP/HTTPS 代理环境变量${NC}"
        fi
    else
        echo -e "${YELLOW}警告: 无法解析代理格式: ${PROXY}${NC}"
        echo -e "${YELLOW}预期格式: http://host:port 或 socks5://host:port${NC}"
    fi
else
    echo -e "${YELLOW}未检测到代理设置${NC}"
    echo -e "${BLUE}使用方法:${NC}"
    echo -e "  export all_proxy=\"socks5h://172.16.8.170:7890\""
    echo -e "  source setup_proxy.sh"
    echo ""
    echo -e "${BLUE}或者直接设置 HTTP 代理:${NC}"
    echo -e "  export HTTP_PROXY=\"http://172.16.8.170:7890\""
    echo -e "  export HTTPS_PROXY=\"http://172.16.8.170:7890\""
fi

echo ""
echo -e "${BLUE}当前代理环境变量:${NC}"
env | grep -i proxy | sort || echo "  无"

