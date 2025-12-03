#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libubox/utils.h>
#include "http_server.h"

static int on_url(llhttp_t *parser, const char *at, size_t length) 
{
    struct http_conn *conn = (struct http_conn *)parser->data;
    conn->request_path = strndup(at, length);
    return 0;
}

static int on_body(llhttp_t *parser, const char *at, size_t length) 
{
    struct http_conn *conn = (struct http_conn *)parser->data;
    
    // 分配内存存储请求体
    if (!conn->request_body) {
        conn->request_body = malloc(length + 1);
        conn->request_body_len = 0;
    } else {
        conn->request_body = realloc(conn->request_body, conn->request_body_len + length + 1);
    }
    
    memcpy(conn->request_body + conn->request_body_len, at, length);
    conn->request_body_len += length;
    conn->request_body[conn->request_body_len] = '\0';
    
    return 0;
}

static int on_message_complete(llhttp_t *parser) {
    struct http_conn *conn = (struct http_conn *)parser->data;
    
    // 处理 JSON 请求
    if (conn->request_body && conn->request_body_len > 0) {
        json_object *json = json_tokener_parse(conn->request_body);
        if (json) {
            // 创建响应 JSON
            json_object *response = json_object_new_object();
            json_object *status = json_object_new_string("ok");
            json_object *message = json_object_new_string("Request received");
            json_object_object_add(response, "status", status);
            json_object_object_add(response, "message", message);
            
            // 如果有输入数据，回显
            json_object *data = json_object_object_get(json, "data");
            if (data) {
                json_object_object_add(response, "echo", json_object_get(data));
            }
            
            const char *response_str = json_object_to_json_string(response);
            conn->response_body = strdup(response_str);
            conn->response_body_len = strlen(response_str);
            conn->status_code = 200;
            
            json_object_put(json);
            json_object_put(response);
        } else {
            // JSON 解析失败
            conn->response_body = strdup("{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            conn->response_body_len = strlen(conn->response_body);
            conn->status_code = 400;
        }
    } else {
        // 没有请求体，返回默认响应
        conn->response_body = strdup("{\"status\":\"ok\",\"message\":\"HTTP JSON Server\"}");
        conn->response_body_len = strlen(conn->response_body);
        conn->status_code = 200;
    }
    
    // 发送响应
    char response[4096];
    snprintf(response, sizeof(response),
        "HTTP/1.1 %d OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        conn->status_code,
        conn->response_body_len,
        conn->response_body ? conn->response_body : "");
    
    ustream_write(&conn->s.stream, response, strlen(response), false);
    
    return 0;
}

static void stream_notify_read(struct ustream *s, int bytes) 
{
    struct http_conn *conn = container_of(s, struct http_conn, s.stream);
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

static void stream_notify_state(struct ustream *s) {
    struct http_conn *conn = container_of(s, struct http_conn, s.stream);
    
    if (!s->eof && !s->write_error)
        return;
    
    if (conn->request_path) {
        free(conn->request_path);
        conn->request_path = NULL;
    }
    if (conn->request_body) {
        free(conn->request_body);
        conn->request_body = NULL;
    }
    if (conn->response_body) {
        free(conn->response_body);
        conn->response_body = NULL;
    }
    
    uloop_fd_delete(&conn->s.fd);
    ustream_free(&conn->s.stream);
    close(conn->s.fd.fd);
    free(conn);
}

// 接受新连接
static void server_cb(struct uloop_fd *fd, unsigned int events) {
    struct http_server *server = container_of(fd, struct http_server, server_fd);
    struct sockaddr_storage client_addr; 
    socklen_t client_len = sizeof(client_addr);
    int client_fd;
    
    client_fd = accept(fd->fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept");
        return;
    }
    
    // 创建连接结构
    struct http_conn *conn = calloc(1, sizeof(*conn));
    if (!conn) {
        close(client_fd);
        return;
    }
    
    // 初始化 HTTP 解析器
    llhttp_settings_init(&conn->settings);
    conn->settings.on_url = on_url;
    conn->settings.on_body = on_body;
    conn->settings.on_message_complete = on_message_complete;
    
    llhttp_init(&conn->parser, HTTP_REQUEST, &conn->settings);
    conn->parser.data = conn;
    
    // 初始化流
    ustream_fd_init(&conn->s, client_fd);
    conn->s.stream.notify_read = stream_notify_read;
    conn->s.stream.notify_state = stream_notify_state;
}

int http_server_init(struct http_server *server) 
{
    int fd = usock_inet(server->type, server->host, server->service, &server->addr);
    if (fd < 0) {
        perror("usock_inet");
        return -1;
    }
    
    server->server_fd.fd = fd;
    server->server_fd.cb = server_cb;
    uloop_fd_add(&server->server_fd, ULOOP_READ);
    return 0;
}

void http_server_cleanup(struct http_server *server) {
    if (server->server_fd.fd >= 0) {
        uloop_fd_delete(&server->server_fd);
        close(server->server_fd.fd);
        server->server_fd.fd = -1;
    }
}
