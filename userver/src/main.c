#include "http.h"
#include "http_json.h"
#include "http_form.h"
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
    fprintf(stderr, "  -m MODE         Body handler mode:\n");
    fprintf(stderr, "                    json-stream  - JSON 流式解析（零拷贝，默认）\n");
    fprintf(stderr, "                    json-buffer  - JSON 缓冲解析（传统）\n");
    fprintf(stderr, "                    form         - Form URL 编码解析\n");
    fprintf(stderr, "\n  SSL/TLS 选项:\n");
    fprintf(stderr, "  -S              Enable HTTPS (SSL/TLS)\n");
    fprintf(stderr, "  -c CERT         SSL certificate file (PEM format)\n");
    fprintf(stderr, "  -k KEY          SSL private key file (PEM format)\n");
    fprintf(stderr, "  -C CA           CA certificate file for client verification\n");
    fprintf(stderr, "\nExamples:\n");
    fprintf(stderr, "  HTTP:\n");
    fprintf(stderr, "    %s -p 8080                    # JSON 流式模式\n", prog);
    fprintf(stderr, "    %s -p 8080 -m json-buffer    # JSON 缓冲模式\n", prog);
    fprintf(stderr, "    %s -p 8080 -m form           # Form 解析模式\n", prog);
    fprintf(stderr, "\n  HTTPS:\n");
    fprintf(stderr, "    %s -S -p 8443 -c server.crt -k server.key\n", prog);
    fprintf(stderr, "    %s -S -p 8443 -c server.crt -k server.key -C ca.crt\n", prog);
    fprintf(stderr, "\n测试命令:\n");
    fprintf(stderr, "  HTTP JSON:  curl -X POST -H 'Content-Type: application/json' \\\n");
    fprintf(stderr, "              -d '{\"data\":\"hello\"}' http://localhost:8080\n");
    fprintf(stderr, "  HTTPS JSON: curl -k -X POST -H 'Content-Type: application/json' \\\n");
    fprintf(stderr, "              -d '{\"data\":\"hello\"}' https://localhost:8443\n");
    fprintf(stderr, "  Form:       curl -X POST -H 'Content-Type: application/x-www-form-urlencoded' \\\n");
    fprintf(stderr, "              -d 'name=John&age=30' http://localhost:8080\n");
}

int main(int argc, char *argv[]) {
    char *host = NULL;
    char *port = "8080";
    char *socket_path = NULL;
    char *mode = "json-stream";
    int opt;
    int type = USOCK_TCP | USOCK_SERVER | USOCK_NONBLOCK;
    
    /* SSL 选项 */
    int use_ssl = 0;
    char *cert_file = NULL;
    char *key_file = NULL;
    char *ca_file = NULL;
    
    while ((opt = getopt(argc, argv, "h:p:s:m:Sc:k:C:")) != -1) {
        switch (opt) {
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 's':
                socket_path = optarg;
                type |= USOCK_UNIX;
                break;
            case 'm':
                mode = optarg;
                break;
            case 'S':
                use_ssl = 1;
                break;
            case 'c':
                cert_file = optarg;
                break;
            case 'k':
                key_file = optarg;
                break;
            case 'C':
                ca_file = optarg;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    /* 验证 SSL 配置 */
    if (use_ssl) {
        if (!cert_file || !key_file) {
            fprintf(stderr, "Error: SSL enabled but certificate or key file not specified\n");
            fprintf(stderr, "Use -c for certificate and -k for private key\n");
            print_usage(argv[0]);
            return 1;
        }
        if (port && strcmp(port, "8080") == 0) {
            /* 如果用户没有指定端口，默认使用 8443 */
            port = "8443";
        }
    }
    
    /* 选择 body 处理器 */
    http_body_handler_t *handler;
    if (strcmp(mode, "json-stream") == 0) {
        handler = http_json_handler_stream();
        printf("Using JSON stream mode (zero-copy)\n");
    } else if (strcmp(mode, "json-buffer") == 0) {
        handler = http_json_handler_buffer();
        printf("Using JSON buffer mode (traditional)\n");
    } else if (strcmp(mode, "form") == 0) {
        handler = http_form_handler_urlencoded();
        printf("Using Form URL-encoded mode\n");
    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        print_usage(argv[0]);
        return 1;
    }
    
    uloop_init();
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* 配置服务器 */
    server.type = type;
    server.host = socket_path ? socket_path : host;
    server.service = socket_path ? NULL : port;
    server.use_ssl = use_ssl;
    
    if (use_ssl) {
        server.ssl_config.cert_file = cert_file;
        server.ssl_config.key_file = key_file;
        server.ssl_config.ca_file = ca_file;
        server.ssl_config.verify_client = 0;
    }
    
    if (http_init(&server, handler) < 0) {
        fprintf(stderr, "Failed to initialize %s server\n", use_ssl ? "HTTPS" : "HTTP");
        uloop_done();
        return 1;
    }
    
    /* 打印服务器信息 */
    const char *protocol = use_ssl ? "HTTPS" : "HTTP";
    if (socket_path) {
        printf("%s Server listening on Unix socket: %s\n", protocol, socket_path);
    } else if (host) {
        printf("%s Server listening on %s:%s\n", protocol, host, port);
    } else {
        printf("%s Server listening on port %s (all interfaces)\n", protocol, port);
    }
    
    if (use_ssl) {
        printf("SSL certificate: %s\n", cert_file);
        printf("SSL private key: %s\n", key_file);
        if (ca_file) {
            printf("SSL CA file: %s\n", ca_file);
        }
    }
    
    uloop_run();
    http_cleanup(&server);
    uloop_done();
    return 0;
}
