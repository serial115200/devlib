#ifndef HTTP_FORM_H
#define HTTP_FORM_H

#include "http.h"
#include <json-c/json.h>

/* Form 键值对 */
typedef struct form_field {
    char *name;
    char *value;
    struct form_field *next;
} form_field_t;

/* Form body 上下文 */
typedef struct {
    form_field_t *fields;
    char *buffer;
    size_t buffer_len;
    size_t buffer_cap;
} http_form_ctx_t;

/* 获取 Form body 处理器（application/x-www-form-urlencoded） */
http_body_handler_t *http_form_handler_urlencoded(void);

#endif // HTTP_FORM_H

