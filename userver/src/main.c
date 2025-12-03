#include "http_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

static struct http_server server;
static int running = 1;

static void signal_handler(int sig) {
    (void)sig;
    running = 0;
    uloop_end();
}

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [OPTIONS]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -p PORT         Listen on TCP port (default: 8080)\n");
    fprintf(stderr, "  -h HOST         Bind to specific host (default: all interfaces)\n");
    fprintf(stderr, "  -s SOCKET       Listen on Unix socket path\n");
    fprintf(stderr, "\nExamples:\n");
    fprintf(stderr, "  %s -p 8080              # Listen on all interfaces, port 8080\n", prog);
    fprintf(stderr, "  %s -h 127.0.0.1 -p 8080 # Listen on localhost only\n", prog);
    fprintf(stderr, "  %s -s /tmp/userver.sock # Use Unix socket\n", prog);
}

int main(int argc, char *argv[]) {
    char *host = NULL;
    char *port = "8080";
    char *socket_path = NULL;
    int opt;
    int type = USOCK_TCP | USOCK_SERVER | USOCK_NONBLOCK;
    
    // 解析命令行参数
    while ((opt = getopt(argc, argv, "h:p:s:u")) != -1) {
        switch (opt) {
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 's':
                socket_path = optarg;
                break;
            case 'u':
                type |= USOCK_UNIX;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // 初始化 uloop
    uloop_init();
    
    // 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化 HTTP 服务器
    server.type = type;
    server.host = host;
    server.service = port;
    if (http_server_init(&server) < 0) {
        fprintf(stderr, "Failed to initialize HTTP server\n");
        uloop_done();
        return 1;
    }
    
    // 打印服务器信息
    if (socket_path) {
        printf("HTTP JSON Server listening on Unix socket: %s\n", socket_path);
    } else if (host) {
        printf("HTTP JSON Server listening on %s:%s\n", host, port);
    } else {
        printf("HTTP JSON Server listening on port %s (all interfaces)\n", port);
    }
    
    uloop_run();
    http_server_cleanup(&server);
    uloop_done();
    return 0;
}
