#include "http_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static struct http_server server;
static int running = 1;

static void signal_handler(int sig) {
    (void)sig;
    running = 0;
    uloop_end();
}

int main(int argc, char *argv[]) {
    int port = 8080;
    
    // 解析命令行参数
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number: %s\n", argv[1]);
            return 1;
        }
    }
    
    // 初始化 uloop
    uloop_init();
    
    // 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化 HTTP 服务器
    if (http_server_init(&server, port) < 0) {
        fprintf(stderr, "Failed to initialize HTTP server\n");
        uloop_done();
        return 1;
    }
    
    // 启动服务器
    http_server_start(&server);
    
    // 清理
    http_server_cleanup(&server);
    uloop_done();
    
    return 0;
}

