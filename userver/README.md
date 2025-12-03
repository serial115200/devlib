# userver - HTTP JSON 服务

这是 devlib 项目的第一个功能：基于 libubox 和 llhttp 的 HTTP JSON 服务。

## 功能特性

- 使用 libubox 的 uloop 事件循环和 ustream 流处理
- 使用 llhttp 进行高性能 HTTP 解析
- 支持 JSON 请求和响应
- 异步非阻塞 I/O

## 构建

在项目根目录运行：

```bash
./build.sh
```

服务可执行文件会安装到 `rootfs/usr/bin/userver`

## 运行

```bash
# 使用默认端口 8080
./rootfs/usr/bin/userver

# 或指定端口
./rootfs/usr/bin/userver 9000
```

## 使用示例

### 发送 GET 请求

```bash
curl http://localhost:8080/
```

响应：
```json
{"status":"ok","message":"HTTP JSON Server"}
```

### 发送 POST JSON 请求

```bash
curl -X POST http://localhost:8080/ \
  -H "Content-Type: application/json" \
  -d '{"data": {"name": "test", "value": 123}}'
```

响应：
```json
{"status":"ok","message":"Request received","echo":{"name":"test","value":123}}
```

## 测试

项目提供了两个测试脚本：

### 完整测试脚本

运行完整的测试套件（包含多种测试场景）：

```bash
# 在项目根目录
cd userver

# 先启动服务器（在另一个终端）
./rootfs/usr/bin/userver 8080

# 运行测试脚本
./test_curl.sh 8080
```

### 简单测试脚本

快速测试基本功能：

```bash
# 在项目根目录
cd userver

# 先启动服务器（在另一个终端）
./rootfs/usr/bin/userver 8080

# 运行简单测试
./test_simple.sh 8080
```

### 手动测试

也可以使用 curl 手动测试：

```bash
# GET 请求
curl http://localhost:8080/

# POST 请求（带 JSON）
curl -X POST http://localhost:8080/ \
  -H "Content-Type: application/json" \
  -d '{"data": {"test": "value"}}'

# POST 请求（无效 JSON，应该返回 400）
curl -X POST http://localhost:8080/ \
  -H "Content-Type: application/json" \
  -d '{"invalid": json}'
```

## 代码结构

```
userver/
├── src/
│   ├── main.c          # 主程序入口
│   ├── http_server.c   # HTTP 服务器实现
│   └── http_server.h   # HTTP 服务器头文件
├── test_curl.sh        # 完整测试脚本
├── test_simple.sh      # 简单测试脚本
└── CMakeLists.txt      # 构建配置
```

## 依赖

- libubox: 提供事件循环和流处理
- llhttp: HTTP 解析器
- json-c: JSON 处理

