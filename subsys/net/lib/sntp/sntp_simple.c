/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>

#include <zephyr/net/sntp.h>
#include <zephyr/net/socketutils.h>

int sntp_simple(const char *server, uint32_t timeout, struct sntp_time *time)
{
	int res;
	static struct addrinfo hints;
	struct addrinfo *addr;
	struct sntp_ctx sntp_ctx;
	uint64_t deadline;
	uint32_t iter_timeout;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	/* 123 is the standard SNTP port per RFC4330 */
	res = net_getaddrinfo_addr_str(server, "123", &hints, &addr);

	if (res < 0) {
		/* Just in case, as namespace for getaddrinfo errors is
		 * different from errno errors.
		 */
		errno = EDOM;
		return res;
	}

	res = sntp_init(&sntp_ctx, addr->ai_addr, addr->ai_addrlen);
	if (res < 0) {
		goto freeaddr;
	}

	if (timeout == SYS_FOREVER_MS) {
		deadline = (uint64_t)timeout;
	} else {
		deadline = k_uptime_get() + (uint64_t)timeout;
	}

	/* Timeout for current iteration */
	iter_timeout = 100;

	while (k_uptime_get() < deadline) {
		res = sntp_query(&sntp_ctx, iter_timeout, time);

		if (res != -ETIMEDOUT) {
			break;
		}

		/* Exponential backoff with limit */
		if (iter_timeout < 1000) {
			iter_timeout *= 2;
		}
	}

	sntp_close(&sntp_ctx);

freeaddr:
	freeaddrinfo(addr);

	return res;
}
