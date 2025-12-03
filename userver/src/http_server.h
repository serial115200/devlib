#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <sys/socket.h>
#include <libubox/uloop.h>
#include <libubox/ustream.h>
#include <libubox/usock.h>
#include <llhttp.h>
#include <json-c/json.h>

struct http_server {
    int type;
    const char* host;
    const char* service;
    struct uloop_fd server_fd;
    struct sockaddr_storage addr;
};

struct http_conn {
    struct ustream_fd s;
    llhttp_t parser;
    llhttp_settings_t settings;
    char *request_path;
    char *request_body;
    size_t request_body_len;
    int status_code;
    char *response_body;
    size_t response_body_len;
};

int http_server_init(struct http_server *server);
void http_server_cleanup(struct http_server *server);

#endif // HTTP_SERVER_H

