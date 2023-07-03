#ifndef HTTP2_SERVER_H
#define HTTP2_SERVER_H

#include <zephyr/posix/poll.h>

#define MAX_CLIENTS 10

struct http2_server_config {
    char *ip_addr;
    int port;
    int address_family;
};

struct http2_server_ctx {
	int sockfd;
	struct pollfd client_fds[MAX_CLIENTS];
	int num_clients;
	struct k_sem my_sem;
};

typedef struct {
    uint32_t length;
    uint8_t type;
    uint8_t flags;
    uint32_t stream_identifier;
    unsigned char *payload;
} Http2Frame;

/* Initializes the HTTP2 server */
int http2_server_init(struct http2_server_ctx *ctx,
		      struct http2_server_config *config);

/* Starts the HTTP2 server */
int http2_server_start(struct http2_server_ctx *ctx);

/* Stops the HTTP2 server */
int http2_server_stop(void);

#endif
