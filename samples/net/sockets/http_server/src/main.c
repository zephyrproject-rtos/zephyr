/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/net/http/server.h>

#if defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)

#include <zephyr/kernel.h>

#elif defined(__linux__)

#include <netinet/in.h>

#endif

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
