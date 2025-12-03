#!/bin/bash

# 简单的 curl 测试脚本
# 快速测试 HTTP JSON Server

PORT=${1:-8080}
URL="http://localhost:${PORT}"

echo "=== 快速测试 HTTP JSON Server ==="
echo "服务器: ${URL}"
echo ""

# 测试 1: 基本 GET
echo "1. GET 请求:"
curl -s "${URL}" | python3 -m json.tool 2>/dev/null || curl -s "${URL}"
echo -e "\n"

# 测试 2: POST with JSON
echo "2. POST 请求 (带 JSON):"
curl -s -X POST \
    -H "Content-Type: application/json" \
    -d '{"data": {"test": "value"}}' \
    "${URL}" | python3 -m json.tool 2>/dev/null || curl -s -X POST -H "Content-Type: application/json" -d '{"data": {"test": "value"}}' "${URL}"
echo -e "\n"

# 测试 3: 无效 JSON
echo "3. POST 请求 (无效 JSON):"
curl -s -X POST \
    -H "Content-Type: application/json" \
    -d '{"invalid": json}' \
    "${URL}" | python3 -m json.tool 2>/dev/null || curl -s -X POST -H "Content-Type: application/json" -d '{"invalid": json}' "${URL}"
echo -e "\n"

echo "测试完成！"

