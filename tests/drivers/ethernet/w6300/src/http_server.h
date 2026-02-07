#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>

/* HTTP Server Configuration */
#define HTTP_SERVER_PORT 8080
#define MAX_HTTP_CLIENTS 4
#define HTTP_RX_BUF_SIZE 2048
#define HTTP_TX_BUF_SIZE 2048

/* Function Prototypes */
void http_server_init(void);
void http_server_run(void);

#endif /* __HTTP_SERVER_H__ */