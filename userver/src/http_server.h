#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <libubox/uloop.h>
#include <libubox/ustream.h>
#include <libubox/usock.h>
#include <llhttp.h>
#include <json-c/json.h>

struct http_server {
    struct uloop_fd server_fd;
    int port;
    struct uloop_timeout timeout;
};

struct http_connection {
    struct ustream_fd s;
    llhttp_t parser;
    llhttp_settings_t settings;
    char *request_body;
    size_t request_body_len;
    int status_code;
    char *response_body;
    size_t response_body_len;
};

int http_server_init(struct http_server *server, int port);
void http_server_cleanup(struct http_server *server);
int http_server_start(struct http_server *server);

#endif // HTTP_SERVER_H

