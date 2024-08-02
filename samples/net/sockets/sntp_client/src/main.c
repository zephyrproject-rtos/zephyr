/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sntp_client_sample, LOG_LEVEL_DBG);

#include <zephyr/net/sntp.h>
#include <arpa/inet.h>

#include "config.h"

#define SNTP_PORT 123

int main(void)
{
	struct sntp_ctx ctx;
	struct sockaddr_in addr;
#if defined(CONFIG_NET_IPV6)
	struct sockaddr_in6 addr6;
#endif
	struct sntp_time sntp_time;
	int rv;

	/* ipv4 */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SNTP_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &addr.sin_addr);

	rv = sntp_init(&ctx, (struct sockaddr *) &addr,
		       sizeof(struct sockaddr_in));
	if (rv < 0) {
		LOG_ERR("Failed to init SNTP IPv4 ctx: %d", rv);
		goto end;
	}

	LOG_INF("Sending SNTP IPv4 request...");
	rv = sntp_query(&ctx, 4 * MSEC_PER_SEC, &sntp_time);
	if (rv < 0) {
		LOG_ERR("SNTP IPv4 request failed: %d", rv);
		goto end;
	}

	LOG_INF("status: %d", rv);
	LOG_INF("time since Epoch: high word: %u, low word: %u",
		(uint32_t)(sntp_time.seconds >> 32), (uint32_t)sntp_time.seconds);

#if defined(CONFIG_NET_IPV6)
	sntp_close(&ctx);

	/* ipv6 */
	memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(SNTP_PORT);
	inet_pton(AF_INET6, SERVER_ADDR6, &addr6.sin6_addr);

	rv = sntp_init(&ctx, (struct sockaddr *) &addr6,
		       sizeof(struct sockaddr_in6));
	if (rv < 0) {
		LOG_ERR("Failed to init SNTP IPv6 ctx: %d", rv);
		goto end;
	}

	LOG_INF("Sending SNTP IPv6 request...");
	/* With such a timeout, this is expected to fail. */
	rv = sntp_query(&ctx, 0, &sntp_time);
	if (rv < 0) {
		LOG_ERR("SNTP IPv6 request: %d", rv);
		goto end;
	}

	LOG_INF("status: %d", rv);
	LOG_INF("time since Epoch: high word: %u, low word: %u",
		(uint32_t)(sntp_time.seconds >> 32), (uint32_t)sntp_time.seconds);
#endif

end:
	sntp_close(&ctx);
	return 0;
}
