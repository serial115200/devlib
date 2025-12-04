#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libubox/utils.h>
#include <libubox/ustream-ssl.h>
#include "http.h"

/* 全局 body 处理器 */
static http_body_handler_t *g_body_handler = NULL;

/* 设置/获取全局 body 处理器 */
void http_set_body_handler(http_body_handler_t *handler) {
    g_body_handler = handler;
}

http_body_handler_t *http_get_body_handler(void) {
    return g_body_handler;
}

/* HTTP 头部处理 */
int http_on_header_field(llhttp_t *parser, const char *at, size_t length) 
{
    struct http_conn *conn = (struct http_conn *)parser->data;
    
    /* 检测 Content-Type 头部 */
    if (length == 12 && strncasecmp(at, "Content-Type", 12) == 0) {
        /* 标记下一个 header_value 是 content_type */
        conn->content_type = (char *)1; // 临时标记
    }
    
    return 0;
}

int http_on_header_value(llhttp_t *parser, const char *at, size_t length) 
{
    struct http_conn *conn = (struct http_conn *)parser->data;
    
    /* 如果是 Content-Type 的值 */
    if (conn->content_type == (char *)1) {
        conn->content_type = strndup(at, length);
    }
    
    return 0;
}

int http_on_headers_complete(llhttp_t *parser) 
{
    struct http_conn *conn = (struct http_conn *)parser->data;
    
    /* 初始化 body 处理器 */
    if (g_body_handler && g_body_handler->on_init) {
        return g_body_handler->on_init(conn, conn->content_type);
    }
    
    return 0;
}

int http_on_body(llhttp_t *parser, const char *at, size_t length) 
{
    struct http_conn *conn = (struct http_conn *)parser->data;
    
    /* 调用 body 处理器 */
    if (g_body_handler && g_body_handler->on_data) {
        return g_body_handler->on_data(conn, at, length);
    }
    
    return 0;
}

int http_on_message_complete(llhttp_t *parser) 
{
    struct http_conn *conn = (struct http_conn *)parser->data;
    
    /* 调用 body 处理器完成回调 */
    if (g_body_handler && g_body_handler->on_complete) {
        if (g_body_handler->on_complete(conn) < 0) {
            conn->status_code = 400;
            conn->response_body = strdup("{\"error\":\"Bad Request\"}");
            conn->response_body_len = strlen(conn->response_body);
            conn->response_content_type = "application/json";
        }
    }
    
    /* 发送响应 */
    http_send_response(conn);
    
    return 0;
}

/* HTTP 响应发送 */
void http_send_response(struct http_conn *conn)
{
    char header[4096];
    const char *content_type = conn->response_content_type ? 
                                conn->response_content_type : "text/plain";
    
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        conn->status_code,
        conn->status_code == 200 ? "OK" : "Bad Request",
        content_type,
        conn->response_body_len);
    
    ustream_write(conn->stream, header, header_len, false);
    
    if (conn->response_body && conn->response_body_len > 0) {
        ustream_write(conn->stream, conn->response_body, 
                     conn->response_body_len, false);
    }
}

/* SSL 连接通知回调 */
static void ssl_notify_connected(struct ustream_ssl *ssl)
{
    fprintf(stderr, "SSL connection established\n");
}

static void ssl_notify_error(struct ustream_ssl *ssl, int error, const char *str)
{
    fprintf(stderr, "SSL error(%d): %s\n", error, str);
}

/* ustream 读取回调（HTTP 和 HTTPS 统一） */
static void stream_notify_read(struct ustream *s, int bytes) 
{
    struct http_conn *conn = container_of(s, struct http_conn, stream);
    if (!conn) return;
    
    char *data;
    int len;
    
    while ((data = ustream_get_read_buf(s, &len)) != NULL && len > 0) 
    {
        enum llhttp_errno err = llhttp_execute(&conn->parser, data, len);
        if (err != HPE_OK) {
            fprintf(stderr, "HTTP parse error: %s\n", llhttp_errno_name(err));
            ustream_consume(s, len);
            break;
        }
        ustream_consume(s, len);
    }
}

/* ustream 状态回调（HTTP 和 HTTPS 统一） */
static void stream_notify_state(struct ustream *s) 
{
    struct http_conn *conn = container_of(s, struct http_conn, stream);
    if (!conn) return;
    
    if (!s->eof && !s->write_error)
        return;
    
    /* 清理 body 处理器 */
    http_body_handler_t *handler = http_get_body_handler();
    if (handler && handler->on_cleanup) {
        handler->on_cleanup(conn);
    }
    
    /* 清理连接资源 */
    if (conn->content_type && conn->content_type != (char *)1) {
        free(conn->content_type);
    }
    if (conn->response_body) {
        free(conn->response_body);
    }
    
    /* 清理 stream */
    if (conn->ssl) {
        /* HTTPS: 需要清理 SSL 层 */
        ustream_free(conn->stream);
        ustream_free(&conn->fd.stream);
        close(conn->fd.fd.fd);
    } else {
        /* HTTP: 只清理 fd stream */
        uloop_fd_delete(&conn->fd.fd);
        ustream_free(&conn->fd.stream);
        close(conn->fd.fd.fd);
    }
    
    free(conn);
}

/* 服务器接受连接回调（统一处理 HTTP 和 HTTPS） */
static void server_cb(struct uloop_fd *fd, unsigned int events) 
{
    struct http_server *server = container_of(fd, struct http_server, server_fd);
    struct sockaddr_storage client_addr; 
    socklen_t client_len = sizeof(client_addr);
    int client_fd;
    
    client_fd = accept(fd->fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept");
        return;
    }
    
    struct http_conn *conn = calloc(1, sizeof(*conn));
    if (!conn) {
        close(client_fd);
        return;
    }
    
    /* 初始化 llhttp */
    llhttp_settings_init(&conn->settings);
    conn->settings.on_header_field = http_on_header_field;
    conn->settings.on_header_value = http_on_header_value;
    conn->settings.on_headers_complete = http_on_headers_complete;
    conn->settings.on_body = http_on_body;
    conn->settings.on_message_complete = http_on_message_complete;
    
    llhttp_init(&conn->parser, HTTP_REQUEST, &conn->settings);
    conn->parser.data = conn;
    
    /* 根据配置初始化 HTTP 或 HTTPS stream */
    if (server->use_ssl && server->ssl_ctx) {
        /* HTTPS: 初始化 SSL 层 */
        struct ustream_ssl *ssl = calloc(1, sizeof(*ssl));
        if (!ssl) {
            close(client_fd);
            free(conn);
            return;
        }
        
        conn->ssl = ssl;
        ssl->stream.string_data = true;
        ssl->stream.notify_read = stream_notify_read;
        ssl->stream.notify_state = stream_notify_state;
        ssl->notify_connected = ssl_notify_connected;
        ssl->notify_error = ssl_notify_error;
        
        ustream_fd_init(&conn->fd, client_fd);
        ustream_ssl_init(ssl, &conn->fd.stream, server->ssl_ctx, true);
        
        conn->stream = &ssl->stream;
    } else {
        /* HTTP: 直接使用 fd stream */
        ustream_fd_init(&conn->fd, client_fd);
        conn->fd.stream.notify_read = stream_notify_read;
        conn->fd.stream.notify_state = stream_notify_state;
        
        conn->stream = &conn->fd.stream;
    }
}

/* HTTP/HTTPS 服务器初始化（统一接口） */
int http_init(struct http_server *server, http_body_handler_t *handler)
{
    http_set_body_handler(handler);
    
    /* 如果启用 SSL，初始化 SSL 上下文 */
    if (server->use_ssl) {
        server->ssl_ctx = ustream_ssl_context_new(true);
        if (!server->ssl_ctx) {
            fprintf(stderr, "Failed to create SSL context\n");
            return -1;
        }
        
        /* 加载证书和私钥 */
        if (server->ssl_config.cert_file && server->ssl_config.key_file) {
            if (ustream_ssl_context_set_crt_file(server->ssl_ctx, 
                                                  server->ssl_config.cert_file) < 0) {
                fprintf(stderr, "Failed to load certificate: %s\n", 
                        server->ssl_config.cert_file);
                ustream_ssl_context_free(server->ssl_ctx);
                return -1;
            }
            
            if (ustream_ssl_context_set_key_file(server->ssl_ctx, 
                                                  server->ssl_config.key_file) < 0) {
                fprintf(stderr, "Failed to load private key: %s\n", 
                        server->ssl_config.key_file);
                ustream_ssl_context_free(server->ssl_ctx);
                return -1;
            }
        } else {
            fprintf(stderr, "Warning: SSL enabled but no certificate/key configured\n");
        }
        
        /* 加载 CA 证书（可选） */
        if (server->ssl_config.ca_file) {
            if (ustream_ssl_context_add_ca_crt_file(server->ssl_ctx, 
                                                      server->ssl_config.ca_file) < 0) {
                fprintf(stderr, "Warning: Failed to load CA file: %s\n", 
                        server->ssl_config.ca_file);
            }
        }
    }
    
    /* 创建监听 socket */
    int fd = usock_inet(server->type, server->host, server->service, &server->addr);
    if (fd < 0) {
        perror("usock_inet");
        if (server->ssl_ctx) {
            ustream_ssl_context_free(server->ssl_ctx);
        }
        return -1;
    }
    
    server->server_fd.fd = fd;
    server->server_fd.cb = server_cb;
    uloop_fd_add(&server->server_fd, ULOOP_READ);
    return 0;
}

/* HTTP/HTTPS 服务器清理（统一接口） */
void http_cleanup(struct http_server *server) 
{
    if (server->server_fd.fd >= 0) {
        uloop_fd_delete(&server->server_fd);
        close(server->server_fd.fd);
        server->server_fd.fd = -1;
    }
    
    /* 清理 SSL 上下文 */
    if (server->ssl_ctx) {
        ustream_ssl_context_free(server->ssl_ctx);
        server->ssl_ctx = NULL;
    }
}
