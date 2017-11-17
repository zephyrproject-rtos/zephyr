/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

/* These are provided by the SimpleLink SDK: */
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "simplelink_support.h"

#define MYPORT  4242

int main(void)
{
	int serv;
	struct sockaddr_in bind_addr;

	simplelink_init();

	serv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind_addr.sin_port = htons(MYPORT);
	bind(serv, (struct sockaddr *)&bind_addr, sizeof(bind_addr));

	listen(serv, 5);

	printk("On PC, type \"nc <Board_IP_addr> %d\"\n", MYPORT);
	printk("then enter lines to echo\n");
	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);

		int client = accept(serv, (struct sockaddr *)&client_addr,
				    &client_addr_len);

		printk("Connection accepted, sock = %d\n", client);
		while (1) {
			char buf[128];
			int len = recv(client, buf, sizeof(buf), 0);

			if (len == 0) {
				break;
			}
			send(client, buf, len, 0);
		}

		close(client);
		printk("Connection closed: %d\n", client);
	}
}
