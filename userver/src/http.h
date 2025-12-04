#ifndef HTTP_H
#define HTTP_H

#include <sys/socket.h>
#include <libubox/uloop.h>
#include <libubox/ustream.h>
#include <libubox/usock.h>
#include <llhttp.h>

/* SSL 配置（可选） */
struct http_ssl_config {
    const char *cert_file;      /* 证书文件路径 */
    const char *key_file;       /* 私钥文件路径 */
    const char *ca_file;        /* CA 证书文件 */
    int verify_client;          /* 是否验证客户端证书 */
};

/* HTTP 服务器（统一支持 HTTP 和 HTTPS） */
struct http_server {
    int type;
    const char* host;
    const char* service;
    struct uloop_fd server_fd;
    struct sockaddr_storage addr;
    
    /* SSL 支持（可选） */
    int use_ssl;                        /* 是否启用 SSL */
    struct http_ssl_config ssl_config;  /* SSL 配置 */
    void *ssl_ctx;                      /* SSL 上下文（ustream_ssl_ctx*） */
};

/* HTTP 连接 - 统一支持 HTTP 和 HTTPS */
struct http_conn {
    /* 底层 stream（HTTP 或 HTTPS） */
    struct ustream *stream;         /* 统一的 stream 接口 */
    struct ustream_fd fd;           /* HTTP: 直接使用 */
    void *ssl;                      /* HTTPS: ustream_ssl* */
    
    /* HTTP 解析器 */
    llhttp_t parser;
    llhttp_settings_t settings;
    
    /* Content-Type 相关 */
    char *content_type;
    
    /* Body 处理器上下文（由具体处理器分配） */
    void *body_ctx;
    
    /* 错误标记 */
    int parse_error;
    
    /* 响应数据 */
    int status_code;
    char *response_body;
    size_t response_body_len;
    char *response_content_type;
};

/* Body 处理器接口 */
typedef struct {
    /* 初始化：解析开始前调用 */
    int (*on_init)(struct http_conn *conn, const char *content_type);
    
    /* 处理 body 数据片段（可能被调用多次） */
    int (*on_data)(struct http_conn *conn, const char *data, size_t len);
    
    /* 完成：解析完成后调用 */
    int (*on_complete)(struct http_conn *conn);
    
    /* 清理：连接关闭时调用 */
    void (*on_cleanup)(struct http_conn *conn);
} http_body_handler_t;

/* HTTP 服务器接口 */
int http_init(struct http_server *server, http_body_handler_t *handler);
void http_cleanup(struct http_server *server);

/* HTTP 响应辅助函数 */
void http_send_response(struct http_conn *conn);

/* HTTP 解析回调（供 SSL 模块使用） */
int http_on_header_field(llhttp_t *parser, const char *at, size_t length);
int http_on_header_value(llhttp_t *parser, const char *at, size_t length);
int http_on_headers_complete(llhttp_t *parser);
int http_on_body(llhttp_t *parser, const char *at, size_t length);
int http_on_message_complete(llhttp_t *parser);

/* 设置全局 body 处理器（供 SSL 模块使用） */
void http_set_body_handler(http_body_handler_t *handler);
http_body_handler_t *http_get_body_handler(void);

#endif // HTTP_H
