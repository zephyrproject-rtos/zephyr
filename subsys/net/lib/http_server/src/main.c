/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/http/server.h>
#include <zephyr/posix/netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

int main(void)
{
	struct http2_server_config config;
	struct http2_server_ctx ctx;

	config.port = 8080;
	config.address_family = AF_INET;

	int server_fd = http2_server_init(&ctx, &config);

	if (server_fd < 0) {
		printf("Failed to initialize HTTP2 server\n");
		return -1;
	}

	http2_server_start(&ctx);

	return 0;
}
