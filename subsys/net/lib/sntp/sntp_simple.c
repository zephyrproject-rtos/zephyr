/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>

#include <zephyr/net/sntp.h>
#include <zephyr/net/socketutils.h>

static int sntp_simple_helper(struct sockaddr *addr, socklen_t addr_len, uint32_t timeout,
			      struct sntp_time *ts)
{
	int res;
	struct sntp_ctx sntp_ctx;
	uint64_t deadline;
	uint32_t iter_timeout;

	res = sntp_init(&sntp_ctx, addr, addr_len);
	if (res < 0) {
		return res;
	}

	if (timeout == SYS_FOREVER_MS) {
		deadline = (uint64_t)timeout;
	} else {
		deadline = k_uptime_get() + (uint64_t)timeout;
	}

	/* Timeout for current iteration */
	iter_timeout = 100;

	while (k_uptime_get() < deadline) {
		res = sntp_query(&sntp_ctx, iter_timeout, ts);

		if (res != -ETIMEDOUT) {
			break;
		}

		/* Exponential backoff with limit */
		if (iter_timeout < 1000) {
			iter_timeout *= 2;
		}
	}

	sntp_close(&sntp_ctx);

	return res;
}

int sntp_simple_addr(struct sockaddr *addr, socklen_t addr_len, uint32_t timeout,
		     struct sntp_time *ts)
{
	/* 123 is the standard SNTP port per RFC4330 */
	int res = net_port_set_default(addr, 123);

	if (res < 0) {
		return res;
	}

	return sntp_simple_helper(addr, addr_len, timeout, ts);
}

int sntp_simple(const char *server, uint32_t timeout, struct sntp_time *ts)
{
	int res;
	static struct zsock_addrinfo hints;
	struct zsock_addrinfo *addr;

	hints.ai_family = AF_UNSPEC;
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
	res = sntp_simple_helper(addr->ai_addr, addr->ai_addrlen, timeout, ts);

	zsock_freeaddrinfo(addr);

	return res;
}
