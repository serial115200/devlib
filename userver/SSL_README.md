# userver SSL/TLS 支持

基于 OpenWrt `ustream-ssl` 库的 HTTPS 服务器实现。

## 架构

```
┌─────────────────────────────────────────────────────────────┐
│                      main.c                                  │
│                 (应用入口，SSL 配置)                          │
└─────────────────────────────────────────────────────────────┘
                              │
                ┌─────────────┼─────────────┐
                ▼             ▼             ▼
    ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
    │   http.c     │ │  http_ssl.c  │ │ http_json.c  │
    │  (HTTP/1.1)  │ │   (HTTPS)    │ │   (数据层)   │
    └──────────────┘ └──────────────┘ └──────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │  ustream-ssl     │
                    │  (OpenSSL 封装)  │
                    └──────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │    OpenSSL       │
                    │  (TLS 1.2/1.3)   │
                    └──────────────────┘
```

## 已集成的库

### 1. **ustream-ssl**
- **来源**: OpenWrt 项目
- **版本**: master (最新)
- **后端**: OpenSSL (默认)
- **位置**: `/home/chen/devlib/packages/ustream-ssl/`
- **安装**: `rootfs/usr/lib/libustream-ssl.so`

### 2. **OpenSSL**
- **版本**: 3.0.13 (系统库)
- **支持**: TLS 1.2, TLS 1.3
- **包**: `libssl3t64`, `libssl-dev`

## 文件说明

### 新增文件

```
userver/src/
├── http_ssl.h          # HTTPS 服务器接口
└── http_ssl.c          # HTTPS 服务器实现

packages/ustream-ssl/
├── config.json         # 包配置
└── package.cmake       # CMake 集成

download/libubox/
└── libubox -> 348b     # 符号链接（解决 #include <libubox/xxx.h>）
```

### 修改文件

```
userver/
├── CMakeLists.txt      # 添加 http_ssl.c 和 SSL 链接库
└── src/
    ├── http.h          # 导出回调函数供 SSL 模块使用
    └── http.c          # 回调函数改为非 static

config/
└── packages.cmake      # 添加 ENABLE_USTREAM_SSL 选项

CMakeLists.txt          # 包含 ustream-ssl 包
```

## 编译

```bash
# 完整编译（包含 ustream-ssl）
make all

# 清理重新编译
make clean && make all

# 检查 SSL 库
ls -la rootfs/usr/lib/libustream-ssl.so
ldd rootfs/usr/bin/userver | grep ssl
```

## 使用示例

### 1. **生成自签名证书**（测试用）

```bash
# 生成私钥
openssl genrsa -out server.key 2048

# 生成证书
openssl req -new -x509 -key server.key -out server.crt -days 365 \
    -subj "/C=CN/ST=Beijing/L=Beijing/O=Test/CN=localhost"
```

### 2. **启动 HTTPS 服务器**

```c
#include "http_ssl.h"
#include "http_json.h"

int main() {
    uloop_init();
    
    struct https_server server = {
        .base = {
            .type = USOCK_TCP | USOCK_SERVER | USOCK_NONBLOCK,
            .host = NULL,
            .service = "8443",  // HTTPS 默认端口
        },
        .ssl_config = {
            .cert_file = "server.crt",
            .key_file = "server.key",
            .ca_file = NULL,
            .verify_client = 0,
        },
    };
    
    https_server_init(&server, http_json_handler_stream());
    
    printf("HTTPS Server listening on port 8443\n");
    uloop_run();
    
    https_server_cleanup(&server);
    uloop_done();
    return 0;
}
```

### 3. **测试 HTTPS 连接**

```bash
# 使用 curl 测试（忽略证书验证）
curl -k -X POST -H 'Content-Type: application/json' \
     -d '{"data":"hello"}' \
     https://localhost:8443

# 使用 openssl s_client 测试
openssl s_client -connect localhost:8443 -showcerts
```

## SSL 配置选项

### `struct http_ssl_config`

| 字段 | 类型 | 说明 |
|------|------|------|
| `cert_file` | `const char*` | 服务器证书文件路径（PEM 格式） |
| `key_file` | `const char*` | 服务器私钥文件路径（PEM 格式） |
| `ca_file` | `const char*` | CA 证书文件（用于验证客户端证书） |
| `verify_client` | `int` | 是否验证客户端证书（0=否，1=是） |

## 技术细节

### 1. **SSL 连接结构**

```c
struct https_conn {
    struct http_conn base;     /* HTTP 连接基类 */
    struct ustream_fd fd;      /* 底层 fd stream */
    struct ustream_ssl ssl;    /* SSL stream（加密层） */
};
```

### 2. **数据流**

```
客户端 → TCP socket → ustream_fd → ustream_ssl → llhttp → body_handler
                      ┗━ 明文 ━┛   ┗━ 加密 ━┛
```

### 3. **回调链**

```
accept() → https_server_cb()
           ├─ ustream_fd_init()        # 初始化 fd stream
           ├─ ustream_ssl_init()       # 初始化 SSL
           ├─ ssl_notify_connected()   # SSL 握手完成
           ├─ ssl_stream_notify_read() # 读取解密数据
           │   └─ llhttp_execute()     # HTTP 解析
           │       └─ http_on_body()   # Body 处理
           └─ ssl_stream_notify_state() # 连接状态变化
```

### 4. **SSL 后端支持**

ustream-ssl 支持多种 SSL 后端：

| 后端 | CMake 选项 | 说明 |
|------|-----------|------|
| **OpenSSL** | 默认 | 功能最全，性能好 |
| mbedTLS | `-DMBEDTLS=ON` | 轻量级，适合嵌入式 |
| wolfSSL | `-DWOLFSSL=ON` | 高性能，商业支持 |

切换后端：
```bash
cd build
cmake .. -DMBEDTLS=ON
make
```

## 安全建议

### 生产环境

1. **使用正式证书**
   - 从 CA 机构购买证书
   - 使用 Let's Encrypt 免费证书

2. **证书权限**
   ```bash
   chmod 600 server.key    # 私钥只读
   chmod 644 server.crt    # 证书可读
   ```

3. **启用客户端验证**（双向 TLS）
   ```c
   .ssl_config = {
       .cert_file = "server.crt",
       .key_file = "server.key",
       .ca_file = "ca.crt",
       .verify_client = 1,  // 启用
   }
   ```

4. **配置密码套件**
   ```c
   ustream_ssl_context_set_ciphers(ctx, 
       "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256");
   ```

5. **定期更新证书**
   - 证书有效期通常为 90 天（Let's Encrypt）或 1 年
   - 使用自动化工具（如 certbot）

## 性能优化

### 1. **SSL 会话复用**

ustream-ssl 自动支持 SSL 会话复用，减少握手开销。

### 2. **TLS 1.3**

OpenSSL 3.x 默认启用 TLS 1.3，握手更快（1-RTT）。

### 3. **零拷贝**

SSL 解密后的数据直接传递给 llhttp，无额外拷贝。

```
socket → SSL 解密 → llhttp → json_tokener_parse_ex
         ┗━━━━━━━━━━━━━━━━━━━━━ 零拷贝 ━━━━━━━━━━━━━━━━━━━┛
```

## 故障排查

### 1. **SSL 握手失败**

```bash
# 检查证书和私钥是否匹配
openssl x509 -noout -modulus -in server.crt | openssl md5
openssl rsa -noout -modulus -in server.key | openssl md5
# 两个 MD5 值应该相同
```

### 2. **证书验证失败**

```bash
# 验证证书链
openssl verify -CAfile ca.crt server.crt
```

### 3. **查看 SSL 错误**

服务器会输出详细的 SSL 错误信息：
```
SSL connection error(1): certificate verify failed
```

### 4. **调试 SSL 连接**

```bash
# 使用 openssl s_client 调试
openssl s_client -connect localhost:8443 -debug -msg
```

## 下一步

- [ ] 添加 main.c 中的 HTTPS 模式支持（`-S` 参数）
- [ ] 添加证书自动续期支持
- [ ] 添加 HTTP/2 支持（需要 nghttp2）
- [ ] 添加 ALPN 协商
- [ ] 性能基准测试

## 参考资料

- [OpenWrt ustream-ssl](https://git.openwrt.org/project/ustream-ssl.git)
- [OpenSSL 文档](https://www.openssl.org/docs/)
- [TLS 1.3 RFC 8446](https://tools.ietf.org/html/rfc8446)

