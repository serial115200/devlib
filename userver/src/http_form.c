#include "http_form.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_BUFFER_SIZE 4096
#define MAX_BUFFER_SIZE (1 * 1024 * 1024) /* 1MB */

/* URL 解码辅助函数 */
static int hex_to_int(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

static char *url_decode(const char *str, size_t len)
{
    char *decoded = malloc(len + 1);
    if (!decoded) return NULL;
    
    size_t i = 0, j = 0;
    while (i < len) {
        if (str[i] == '%' && i + 2 < len) {
            decoded[j++] = (hex_to_int(str[i + 1]) << 4) | hex_to_int(str[i + 2]);
            i += 3;
        } else if (str[i] == '+') {
            decoded[j++] = ' ';
            i++;
        } else {
            decoded[j++] = str[i++];
        }
    }
    decoded[j] = '\0';
    return decoded;
}

/* 解析 form 数据 */
static void parse_form_data(http_form_ctx_t *ctx, const char *data, size_t len)
{
    const char *p = data;
    const char *end = data + len;
    
    while (p < end) {
        const char *name_start = p;
        const char *name_end = strchr(p, '=');
        if (!name_end || name_end >= end) break;
        
        const char *value_start = name_end + 1;
        const char *value_end = strchr(value_start, '&');
        if (!value_end || value_end > end) {
            value_end = end;
        }
        
        /* 创建新字段 */
        form_field_t *field = calloc(1, sizeof(*field));
        if (!field) break;
        
        field->name = url_decode(name_start, name_end - name_start);
        field->value = url_decode(value_start, value_end - value_start);
        
        /* 添加到链表 */
        field->next = ctx->fields;
        ctx->fields = field;
        
        p = value_end + 1;
    }
}

/* Form 处理器：初始化 */
static int form_init(struct http_conn *conn, const char *content_type)
{
    if (!content_type || 
        strstr(content_type, "application/x-www-form-urlencoded") == NULL) {
        return 0;
    }
    
    http_form_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return -1;
    
    ctx->buffer_cap = INITIAL_BUFFER_SIZE;
    ctx->buffer = malloc(ctx->buffer_cap);
    if (!ctx->buffer) {
        free(ctx);
        return -1;
    }
    
    conn->body_ctx = ctx;
    return 0;
}

/* Form 处理器：接收数据 */
static int form_data(struct http_conn *conn, const char *data, size_t len)
{
    http_form_ctx_t *ctx = (http_form_ctx_t *)conn->body_ctx;
    if (!ctx) return 0;
    
    /* 如果已经出错，跳过后续数据 */
    if (conn->parse_error) return 0;
    
    /* 检查容量 */
    if (ctx->buffer_len + len > MAX_BUFFER_SIZE) {
        fprintf(stderr, "Form body too large\n");
        conn->parse_error = 1;
        return 0; /* 继续解析 HTTP */
    }
    
    /* 扩容 */
    if (ctx->buffer_len + len + 1 > ctx->buffer_cap) {
        size_t new_cap = ctx->buffer_cap * 2;
        while (new_cap < ctx->buffer_len + len + 1) {
            new_cap *= 2;
        }
        
        char *new_buf = realloc(ctx->buffer, new_cap);
        if (!new_buf) return -1;
        
        ctx->buffer = new_buf;
        ctx->buffer_cap = new_cap;
    }
    
    memcpy(ctx->buffer + ctx->buffer_len, data, len);
    ctx->buffer_len += len;
    ctx->buffer[ctx->buffer_len] = '\0';
    
    return 0;
}

/* Form 处理器：完成 */
static int form_complete(struct http_conn *conn)
{
    http_form_ctx_t *ctx = (http_form_ctx_t *)conn->body_ctx;
    
    /* 检查是否有解析错误 */
    if (conn->parse_error) {
        conn->status_code = 400;
        conn->response_body = strdup("{\"error\":\"Form body too large\",\"status\":\"error\"}");
        conn->response_body_len = strlen(conn->response_body);
        conn->response_content_type = "application/json";
        return 0;
    }
    
    if (!ctx || !ctx->buffer || ctx->buffer_len == 0) {
        conn->status_code = 200;
        conn->response_body = strdup("{\"status\":\"ok\",\"message\":\"HTTP Form Server\"}");
        conn->response_body_len = strlen(conn->response_body);
        conn->response_content_type = "application/json";
        return 0;
    }
    
    /* 解析 form 数据 */
    parse_form_data(ctx, ctx->buffer, ctx->buffer_len);
    
    /* 构建 JSON 响应 */
    json_object *response = json_object_new_object();
    json_object_object_add(response, "status", json_object_new_string("ok"));
    json_object_object_add(response, "type", json_object_new_string("form-urlencoded"));
    
    json_object *fields_obj = json_object_new_object();
    form_field_t *field = ctx->fields;
    while (field) {
        json_object_object_add(fields_obj, field->name, 
                              json_object_new_string(field->value));
        field = field->next;
    }
    json_object_object_add(response, "fields", fields_obj);
    
    const char *response_str = json_object_to_json_string_ext(
        response, JSON_C_TO_STRING_PLAIN);
    conn->response_body = strdup(response_str);
    conn->response_body_len = strlen(response_str);
    conn->status_code = 200;
    conn->response_content_type = "application/json";
    
    json_object_put(response);
    return 0;
}

/* Form 处理器：清理 */
static void form_cleanup(struct http_conn *conn)
{
    http_form_ctx_t *ctx = (http_form_ctx_t *)conn->body_ctx;
    if (!ctx) return;
    
    /* 清理字段链表 */
    form_field_t *field = ctx->fields;
    while (field) {
        form_field_t *next = field->next;
        free(field->name);
        free(field->value);
        free(field);
        field = next;
    }
    
    if (ctx->buffer) {
        free(ctx->buffer);
    }
    free(ctx);
    conn->body_ctx = NULL;
}

static http_body_handler_t form_urlencoded_handler = {
    .on_init = form_init,
    .on_data = form_data,
    .on_complete = form_complete,
    .on_cleanup = form_cleanup,
};

http_body_handler_t *http_form_handler_urlencoded(void)
{
    return &form_urlencoded_handler;
}

