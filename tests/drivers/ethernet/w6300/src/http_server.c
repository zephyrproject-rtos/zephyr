#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include "http_server.h"

LOG_MODULE_DECLARE(w6300_http_server, LOG_LEVEL_INF);

static const char http_response[] =
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: text/html\r\n"
	"Connection: close\r\n"
	"\r\n"
	"<!DOCTYPE html>"
	"<html>"
	"<head>"
	"<title>W6300 Server</title>"
	"<meta name='viewport' content='width=device-width, initial-scale=1'>"
	"<style>"
	"body { font-family: sans-serif; text-align: center; margin-top: 50px; background-color: #f0f0f0; }"
	"h1 { color: #0055aa; }"
	".card { background: white; padding: 20px; margin: auto; max-width: 400px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }"
	".status { color: green; font-weight: bold; }"
	"</style>"
	"</head>"
	"<body>"
	"<div class='card'>"
	"<h1>W6300 HTTP Server</h1>"
	"<p>Board: <strong>W6300 EVB PICO2</strong></p>"
	"<p>Status: <span class='status'>Online</span></p>"
	"<p>Core: Hazard3 (RISC-V)</p>"
	"</div>"
	"</body>"
	"</html>";

void http_server_init(void)
{
    LOG_INF("Initializing HTTP Server");
}

void http_server_run(void)
{
    int server_fd;
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(HTTP_SERVER_PORT),
        .sin_addr = {
            .s_addr = htonl(INADDR_ANY),
        },
    };

    server_fd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0) {
        LOG_ERR("Failed to create socket: %d", errno);
        return;
    }

    if (zsock_bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERR("Failed to bind socket: %d", errno);
        zsock_close(server_fd);
        return;
    }

    if (zsock_listen(server_fd, MAX_HTTP_CLIENTS) < 0) {
        LOG_ERR("Failed to listen: %d", errno);
        zsock_close(server_fd);
        return;
    }

    LOG_INF("HTTP server listening on port %d", HTTP_SERVER_PORT);

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = zsock_accept(server_fd, (struct sockaddr *)&client_addr,
                        &client_addr_len);

        if (client_fd < 0) {
            LOG_ERR("Failed to accept connection: %d", errno);
            k_sleep(K_MSEC(100));
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        zsock_inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        LOG_INF("Client connected from %s", client_ip);

        char recv_buf[HTTP_RX_BUF_SIZE];
        ssize_t total_received = 0;
        ssize_t received;

        /* Set receive timeout to prevent hanging */
        struct zsock_timeval tv = {
            .tv_sec = 2,
            .tv_usec = 0,
        };
        zsock_setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        /* Simple read loop to drain request headers */
        do {
            received = zsock_recv(client_fd, recv_buf + total_received, 
                          HTTP_RX_BUF_SIZE - total_received - 1, 0);
            if (received > 0) {
                total_received += received;
                recv_buf[total_received] = '\0';
                if (strstr(recv_buf, "\r\n\r\n")) {
                    break;
                }
            } else {
                break;
            }
        } while (total_received < HTTP_RX_BUF_SIZE - 1);

        if (total_received > 0) {
            LOG_INF("Request received from %s (%d bytes)", client_ip, (int)total_received);
        }

        ssize_t sent = zsock_send(client_fd, http_response, sizeof(http_response) - 1, 0);
        if (sent < 0) {
            LOG_ERR("Failed to send response to %s: %d", client_ip, errno);
        }

        k_sleep(K_MSEC(50)); /* Give client time to receive */
        zsock_close(client_fd);
        LOG_INF("Client %s disconnected", client_ip);
    }
}
