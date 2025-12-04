#include "http_json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUFFER_SIZE 4096
#define MAX_BUFFER_SIZE (10 * 1024 * 1024) /* 10MB */

/* ============ 流式 JSON 解析（零拷贝） ============ */

static int json_stream_init(struct http_conn *conn, const char *content_type)
{
    /* 只处理 JSON content type */
    if (!content_type || strstr(content_type, "application/json") == NULL) {
        return 0; /* 跳过，不是 JSON */
    }
    
    http_json_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return -1;
    
    ctx->mode = JSON_MODE_STREAM;
    ctx->tokener = json_tokener_new();
    if (!ctx->tokener) {
        free(ctx);
        return -1;
    }
    
    conn->body_ctx = ctx;
    return 0;
}

static int json_stream_data(struct http_conn *conn, const char *data, size_t len)
{
    http_json_ctx_t *ctx = (http_json_ctx_t *)conn->body_ctx;
    if (!ctx) return 0;
    
    /* 如果已经出错，跳过后续数据 */
    if (conn->parse_error) return 0;
    
    /* 流式解析：直接在 ustream 缓冲区中解析，零拷贝 */
    ctx->parsed = json_tokener_parse_ex(ctx->tokener, data, len);
    
    enum json_tokener_error jerr = json_tokener_get_error(ctx->tokener);
    if (jerr != json_tokener_continue && jerr != json_tokener_success) {
        fprintf(stderr, "JSON parse error: %s\n", json_tokener_error_desc(jerr));
        conn->parse_error = 1; /* 标记错误，但继续解析 HTTP */
    }
    
    return 0; /* 继续解析，让 HTTP 正常完成 */
}

static int json_stream_complete(struct http_conn *conn)
{
    http_json_ctx_t *ctx = (http_json_ctx_t *)conn->body_ctx;
    
    /* 检查是否有解析错误 */
    if (conn->parse_error) {
        conn->status_code = 400;
        conn->response_body = strdup("{\"error\":\"Invalid JSON\",\"status\":\"error\"}");
        conn->response_body_len = strlen(conn->response_body);
        conn->response_content_type = "application/json";
        return 0;
    }
    
    if (!ctx || !ctx->parsed) {
        /* 没有 JSON 数据，返回默认响应 */
        conn->status_code = 200;
        conn->response_body = strdup("{\"status\":\"ok\",\"message\":\"HTTP JSON Server (stream)\"}");
        conn->response_body_len = strlen(conn->response_body);
        conn->response_content_type = "application/json";
        return 0;
    }
    
    /* 构建响应 */
    json_object *response = json_object_new_object();
    json_object_object_add(response, "status", json_object_new_string("ok"));
    json_object_object_add(response, "mode", json_object_new_string("stream"));
    
    /* 回显接收到的数据 */
    json_object *data = json_object_object_get(ctx->parsed, "data");
    if (data) {
        json_object_object_add(response, "echo", json_object_get(data));
    }
    
    const char *response_str = json_object_to_json_string_ext(
        response, JSON_C_TO_STRING_PLAIN);
    conn->response_body = strdup(response_str);
    conn->response_body_len = strlen(response_str);
    conn->status_code = 200;
    conn->response_content_type = "application/json";
    
    json_object_put(response);
    return 0;
}

static void json_stream_cleanup(struct http_conn *conn)
{
    http_json_ctx_t *ctx = (http_json_ctx_t *)conn->body_ctx;
    if (!ctx) return;
    
    if (ctx->tokener) {
        json_tokener_free(ctx->tokener);
    }
    if (ctx->parsed) {
        json_object_put(ctx->parsed);
    }
    free(ctx);
    conn->body_ctx = NULL;
}

static http_body_handler_t json_stream_handler = {
    .on_init = json_stream_init,
    .on_data = json_stream_data,
    .on_complete = json_stream_complete,
    .on_cleanup = json_stream_cleanup,
};

http_body_handler_t *http_json_handler_stream(void)
{
    return &json_stream_handler;
}

/* ============ 缓冲 JSON 解析（传统方式） ============ */

static int json_buffer_init(struct http_conn *conn, const char *content_type)
{
    if (!content_type || strstr(content_type, "application/json") == NULL) {
        return 0;
    }
    
    http_json_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return -1;
    
    ctx->mode = JSON_MODE_BUFFER;
    ctx->buffer_cap = INITIAL_BUFFER_SIZE;
    ctx->buffer = malloc(ctx->buffer_cap);
    if (!ctx->buffer) {
        free(ctx);
        return -1;
    }
    
    conn->body_ctx = ctx;
    return 0;
}

static int json_buffer_data(struct http_conn *conn, const char *data, size_t len)
{
    http_json_ctx_t *ctx = (http_json_ctx_t *)conn->body_ctx;
    if (!ctx) return 0;
    
    /* 如果已经出错，跳过后续数据 */
    if (conn->parse_error) return 0;
    
    /* 检查容量 */
    if (ctx->buffer_len + len > MAX_BUFFER_SIZE) {
        fprintf(stderr, "JSON body too large (max %d bytes)\n", MAX_BUFFER_SIZE);
        conn->parse_error = 1;
        return 0; /* 继续解析 HTTP */
    }
    
    /* 扩容 */
    if (ctx->buffer_len + len + 1 > ctx->buffer_cap) {
        size_t new_cap = ctx->buffer_cap * 2;
        while (new_cap < ctx->buffer_len + len + 1) {
            new_cap *= 2;
        }
        if (new_cap > MAX_BUFFER_SIZE) {
            new_cap = MAX_BUFFER_SIZE;
        }
        
        char *new_buf = realloc(ctx->buffer, new_cap);
        if (!new_buf) {
            perror("realloc");
            return -1;
        }
        ctx->buffer = new_buf;
        ctx->buffer_cap = new_cap;
    }
    
    /* 拷贝数据 */
    memcpy(ctx->buffer + ctx->buffer_len, data, len);
    ctx->buffer_len += len;
    ctx->buffer[ctx->buffer_len] = '\0';
    
    return 0;
}

static int json_buffer_complete(struct http_conn *conn)
{
    http_json_ctx_t *ctx = (http_json_ctx_t *)conn->body_ctx;
    
    /* 检查是否有解析错误 */
    if (conn->parse_error) {
        conn->status_code = 400;
        conn->response_body = strdup("{\"error\":\"Invalid JSON or body too large\",\"status\":\"error\"}");
        conn->response_body_len = strlen(conn->response_body);
        conn->response_content_type = "application/json";
        return 0;
    }
    
    if (!ctx || !ctx->buffer || ctx->buffer_len == 0) {
        conn->status_code = 200;
        conn->response_body = strdup("{\"status\":\"ok\",\"message\":\"HTTP JSON Server (buffer)\"}");
        conn->response_body_len = strlen(conn->response_body);
        conn->response_content_type = "application/json";
        return 0;
    }
    
    /* 一次性解析完整 JSON */
    json_object *parsed = json_tokener_parse(ctx->buffer);
    if (!parsed) {
        fprintf(stderr, "Failed to parse JSON buffer\n");
        conn->status_code = 400;
        conn->response_body = strdup("{\"error\":\"Invalid JSON\",\"status\":\"error\"}");
        conn->response_body_len = strlen(conn->response_body);
        conn->response_content_type = "application/json";
        return 0;
    }
    
    /* 构建响应 */
    json_object *response = json_object_new_object();
    json_object_object_add(response, "status", json_object_new_string("ok"));
    json_object_object_add(response, "mode", json_object_new_string("buffer"));
    
    json_object *data = json_object_object_get(parsed, "data");
    if (data) {
        json_object_object_add(response, "echo", json_object_get(data));
    }
    
    const char *response_str = json_object_to_json_string_ext(
        response, JSON_C_TO_STRING_PLAIN);
    conn->response_body = strdup(response_str);
    conn->response_body_len = strlen(response_str);
    conn->status_code = 200;
    conn->response_content_type = "application/json";
    
    json_object_put(parsed);
    json_object_put(response);
    return 0;
}

static void json_buffer_cleanup(struct http_conn *conn)
{
    http_json_ctx_t *ctx = (http_json_ctx_t *)conn->body_ctx;
    if (!ctx) return;
    
    if (ctx->buffer) {
        free(ctx->buffer);
    }
    free(ctx);
    conn->body_ctx = NULL;
}

static http_body_handler_t json_buffer_handler = {
    .on_init = json_buffer_init,
    .on_data = json_buffer_data,
    .on_complete = json_buffer_complete,
    .on_cleanup = json_buffer_cleanup,
};

http_body_handler_t *http_json_handler_buffer(void)
{
    return &json_buffer_handler;
}

