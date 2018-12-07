/*
 * Copyright (c) 2018 Antmicro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <net/socks.h>

#define SOCKS_PROXY_SERVER_PORT 1080

static struct socks_ctx socks_ctx;

int main(void)
{
	printf("Starting SOCKS5 proxy server sample");

	socks_init(&socks_ctx, SOCKS_PROXY_SERVER_PORT);

	return 0;
}
