#!/bin/bash

# HTTP JSON Server 测试脚本
# 使用 curl 测试 userver 的功能

PORT=${1:-8080}
SERVER_URL="http://localhost:${PORT}"

# 颜色输出
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== HTTP JSON Server 测试脚本 ===${NC}"
echo -e "${YELLOW}服务器地址: ${SERVER_URL}${NC}"
echo ""

# 检查服务器是否运行
check_server() {
    if ! curl -s --connect-timeout 1 "${SERVER_URL}" > /dev/null 2>&1; then
        echo -e "${RED}错误: 无法连接到服务器 ${SERVER_URL}${NC}"
        echo -e "${YELLOW}请先启动服务器:${NC}"
        echo -e "  ${GREEN}./rootfs/usr/bin/userver ${PORT}${NC}"
        echo ""
        exit 1
    fi
}

# 测试函数
test_case() {
    local name="$1"
    local method="$2"
    local url="$3"
    local data="$4"
    local expected_status="$5"
    
    echo -e "${BLUE}测试: ${name}${NC}"
    
    if [ -n "$data" ]; then
        response=$(curl -s -w "\n%{http_code}" -X "${method}" \
            -H "Content-Type: application/json" \
            -d "${data}" \
            "${url}")
    else
        response=$(curl -s -w "\n%{http_code}" -X "${method}" "${url}")
    fi
    
    http_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | sed '$d')
    
    if [ "$http_code" = "$expected_status" ]; then
        echo -e "${GREEN}✓ HTTP 状态码: ${http_code} (期望: ${expected_status})${NC}"
    else
        echo -e "${RED}✗ HTTP 状态码: ${http_code} (期望: ${expected_status})${NC}"
    fi
    
    echo -e "${YELLOW}响应内容:${NC}"
    echo "$body" | python3 -m json.tool 2>/dev/null || echo "$body"
    echo ""
}

# 检查服务器
check_server

# 测试 1: GET 请求（无请求体）
test_case "GET 请求 - 无请求体" \
    "GET" \
    "${SERVER_URL}" \
    "" \
    "200"

# 测试 2: POST 请求 - 简单 JSON
test_case "POST 请求 - 简单 JSON" \
    "POST" \
    "${SERVER_URL}" \
    '{"message": "Hello, Server!"}' \
    "200"

# 测试 3: POST 请求 - 带 data 字段（应该回显）
test_case "POST 请求 - 带 data 字段（回显测试）" \
    "POST" \
    "${SERVER_URL}" \
    '{"data": {"name": "test", "value": 123}}' \
    "200"

# 测试 4: POST 请求 - 复杂 JSON
test_case "POST 请求 - 复杂 JSON" \
    "POST" \
    "${SERVER_URL}" \
    '{"data": {"users": [{"id": 1, "name": "Alice"}, {"id": 2, "name": "Bob"}]}}' \
    "200"

# 测试 5: 无效 JSON（应该返回 400）
test_case "POST 请求 - 无效 JSON" \
    "POST" \
    "${SERVER_URL}" \
    '{"invalid": json}' \
    "400"

# 测试 6: 空请求体
test_case "POST 请求 - 空请求体" \
    "POST" \
    "${SERVER_URL}" \
    "" \
    "200"

# 测试 7: 大 JSON 数据
test_case "POST 请求 - 大 JSON 数据" \
    "POST" \
    "${SERVER_URL}" \
    "{\"data\": {\"array\": [$(seq -s ',' 1 100)]}}" \
    "200"

echo -e "${GREEN}=== 所有测试完成 ===${NC}"

