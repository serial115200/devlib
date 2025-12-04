#ifndef HTTP_JSON_H
#define HTTP_JSON_H

#include "http.h"
#include <json-c/json.h>

/* JSON 处理模式 */
typedef enum {
    JSON_MODE_STREAM,   /* 流式解析（零拷贝，推荐） */
    JSON_MODE_BUFFER    /* 缓冲解析（兼容模式） */
} json_parse_mode_t;

/* JSON body 上下文 */
typedef struct {
    json_parse_mode_t mode;
    
    /* 流式解析 */
    json_tokener *tokener;
    json_object *parsed;
    
    /* 缓冲解析 */
    char *buffer;
    size_t buffer_len;
    size_t buffer_cap;
} http_json_ctx_t;

/* 获取 JSON body 处理器（流式模式） */
http_body_handler_t *http_json_handler_stream(void);

/* 获取 JSON body 处理器（缓冲模式） */
http_body_handler_t *http_json_handler_buffer(void);

#endif // HTTP_JSON_H

