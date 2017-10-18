/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#ifndef __ZEPHYR__

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#else

#include <net/socket.h>
#include <kernel.h>
#include <net/net_app.h>

#include <net/buf.h>

#endif

#ifndef USE_BIG_PAYLOAD
#define USE_BIG_PAYLOAD 1
#endif

static const char content[] = {
#if USE_BIG_PAYLOAD
    #include "response_big.html.bin.inc"
#else
    #include "response_small.html.bin.inc"
#endif
};

int main(void)
{
	int serv;
	struct sockaddr_in bind_addr;
	static int counter;

	serv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind_addr.sin_port = htons(8080);
	bind(serv, (struct sockaddr *)&bind_addr, sizeof(bind_addr));

	listen(serv, 5);

	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		char addr_str[32];
		int req_state = 0;
		const char *data;
		size_t len;

		int client = accept(serv, (struct sockaddr *)&client_addr,
				    &client_addr_len);
		inet_ntop(client_addr.sin_family, &client_addr.sin_addr,
			  addr_str, sizeof(addr_str));
		printf("Connection #%d from %s\n", counter++, addr_str);

		/* Discard HTTP request (or otherwise client will get
		 * connection reset error).
		 */
		while (1) {
			char c;

			recv(client, &c, 1, 0);
			if (req_state == 0 && c == '\r') {
				req_state++;
			} else if (req_state == 1 && c == '\n') {
				req_state++;
			} else if (req_state == 2 && c == '\r') {
				req_state++;
			} else if (req_state == 3 && c == '\n') {
				break;
			} else {
				req_state = 0;
			}
		}

		data = content;
		len = sizeof(content);
		while (len) {
			int sent_len = send(client, data, len, 0);

			if (sent_len == -1) {
				printf("Error sending data to peer\n");
				break;
			}
			data += sent_len;
			len -= sent_len;
		}

		close(client);
		printf("Connection from %s closed\n", addr_str);

#if defined(__ZEPHYR__) && defined(CONFIG_NET_BUF_POOL_USAGE)
		struct k_mem_slab *rx, *tx;
		struct net_buf_pool *rx_data, *tx_data;

		net_pkt_get_info(&rx, &tx, &rx_data, &tx_data);
		printf("rx buf: %d, tx buf: %d\n",
		       rx_data->avail_count, tx_data->avail_count);
#endif

	}
}
