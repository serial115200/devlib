# userver - 模块化 HTTP 服务器

基于 libubox + llhttp + json-c 的高性能、模块化 HTTP 服务器。

## 架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                        main.c                                │
│                   (应用入口，参数解析)                        │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                        http.c/h                              │
│              (HTTP 协议层：llhttp + ustream)                 │
│  • 连接管理 (uloop_fd, ustream)                              │
│  • HTTP 解析 (llhttp)                                        │
│  • Header 处理 (Content-Type 检测)                           │
│  • Body 处理器接口 (http_body_handler_t)                     │
└─────────────────────────────────────────────────────────────┘
                              │
                ┌─────────────┼─────────────┐
                ▼             ▼             ▼
    ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
    │ http_json.c  │ │ http_form.c  │ │ http_xxx.c   │
    │   (JSON)     │ │   (Form)     │ │  (扩展...)   │
    └──────────────┘ └──────────────┘ └──────────────┘
         │                  │                  │
         ├─ 流式解析        ├─ URL 解码        └─ ...
         └─ 缓冲解析        └─ 键值对解析
```

## 核心特性

### 1. **分层架构**
- **HTTP 层**：负责 HTTP 协议解析、连接管理
- **数据层**：可插拔的 body 处理器（JSON、Form、自定义）

### 2. **零拷贝 JSON 解析**
- 使用 `json_tokener_parse_ex()` 流式解析
- 直接在 ustream 缓冲区中解析，无需额外拷贝
- 支持分片数据增量解析

### 3. **多种数据格式支持**
- **JSON 流式模式**：零拷贝，推荐用于生产环境
- **JSON 缓冲模式**：传统方式，兼容性好
- **Form URL 编码**：支持表单提交

### 4. **健壮的错误处理**
- 解析错误不会导致连接卡住
- 返回标准 HTTP 错误响应（400 Bad Request）
- 支持大数据量保护（可配置最大 body 大小）

## 编译与安装

```bash
# 编译所有依赖和应用
make all

# 可执行文件位置
./rootfs/usr/bin/userver
```

## 使用方法

### 基本用法

```bash
# JSON 流式模式（默认，零拷贝）
./rootfs/usr/bin/userver -p 8080

# JSON 缓冲模式
./rootfs/usr/bin/userver -p 8080 -m json-buffer

# Form 解析模式
./rootfs/usr/bin/userver -p 8080 -m form

# 绑定到特定 IP
./rootfs/usr/bin/userver -h 127.0.0.1 -p 8080

# Unix Socket
./rootfs/usr/bin/userver -s /tmp/userver.sock
```

### 测试命令

#### JSON 测试
```bash
# 简单 JSON
curl -X POST -H 'Content-Type: application/json' \
     -d '{"data":{"name":"test","value":123}}' \
     http://localhost:8080

# 响应
{"status":"ok","mode":"stream","echo":{"name":"test","value":123}}

# 无效 JSON
curl -X POST -H 'Content-Type: application/json' \
     -d 'invalid json' \
     http://localhost:8080

# 响应
{"error":"Invalid JSON","status":"error"}
```

#### Form 测试
```bash
# URL 编码表单
curl -X POST -H 'Content-Type: application/x-www-form-urlencoded' \
     -d 'name=John&age=30&city=Beijing' \
     http://localhost:8080

# 响应
{"status":"ok","type":"form-urlencoded","fields":{"name":"John","age":"30","city":"Beijing"}}
```

#### 自动化测试
```bash
# 运行完整测试套件
./userver/test_curl.sh
```

## 性能对比

| 模式 | 内存拷贝 | 内存占用 | 适用场景 |
|------|----------|----------|----------|
| **JSON 流式** | 0 次 | 最低 | 生产环境（推荐） |
| **JSON 缓冲** | 1 次 | 中等 | 兼容性测试 |
| **Form** | 1 次 | 中等 | 表单提交 |

### 数据流对比

**流式模式（零拷贝）**：
```
socket → ustream缓冲区 → llhttp → json_tokener_parse_ex → JSON对象
         ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━ 零拷贝 ━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**缓冲模式**：
```
socket → ustream缓冲区 → llhttp → memcpy → buffer → json_tokener_parse → JSON对象
         ┗━━━━━━━━━━━━┛                  ┗━ 拷贝 ━┛
```

## 扩展开发

### 添加新的数据处理器

1. **创建头文件** `http_xxx.h`：

```c
#ifndef HTTP_XXX_H
#define HTTP_XXX_H

#include "http.h"

http_body_handler_t *http_xxx_handler(void);

#endif
```

2. **实现处理器** `http_xxx.c`：

```c
#include "http_xxx.h"

static int xxx_init(struct http_conn *conn, const char *content_type) {
    // 检查 Content-Type，分配上下文
    if (!content_type || strstr(content_type, "application/xxx") == NULL) {
        return 0; // 跳过
    }
    // 分配 conn->body_ctx
    return 0;
}

static int xxx_data(struct http_conn *conn, const char *data, size_t len) {
    // 处理数据片段（可能被多次调用）
    return 0;
}

static int xxx_complete(struct http_conn *conn) {
    // 构建响应
    conn->status_code = 200;
    conn->response_body = strdup("...");
    conn->response_body_len = strlen(conn->response_body);
    conn->response_content_type = "...";
    return 0;
}

static void xxx_cleanup(struct http_conn *conn) {
    // 清理 conn->body_ctx
}

static http_body_handler_t xxx_handler = {
    .on_init = xxx_init,
    .on_data = xxx_data,
    .on_complete = xxx_complete,
    .on_cleanup = xxx_cleanup,
};

http_body_handler_t *http_xxx_handler(void) {
    return &xxx_handler;
}
```

3. **在 main.c 中注册**：

```c
#include "http_xxx.h"

// 在 main() 中添加选项
if (strcmp(mode, "xxx") == 0) {
    handler = http_xxx_handler();
}
```

4. **更新 CMakeLists.txt**：

```cmake
add_executable(userver
    src/main.c
    src/http.c
    src/http_json.c
    src/http_form.c
    src/http_xxx.c  # 新增
)
```

## 文件说明

```
userver/
├── src/
│   ├── main.c           # 应用入口
│   ├── http.h           # HTTP 协议层接口
│   ├── http.c           # HTTP 协议层实现
│   ├── http_json.h      # JSON 处理器接口
│   ├── http_json.c      # JSON 处理器实现（流式+缓冲）
│   ├── http_form.h      # Form 处理器接口
│   └── http_form.c      # Form 处理器实现
├── CMakeLists.txt       # 构建配置
├── test_curl.sh         # 自动化测试脚本
└── README.md            # 本文档
```

## 依赖库

- **libubox**：事件循环 (uloop)、流处理 (ustream)
- **llhttp**：HTTP 解析器
- **json-c**：JSON 解析库

## 许可证

MIT License
