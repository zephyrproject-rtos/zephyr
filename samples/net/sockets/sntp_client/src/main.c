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

#include "net_sample_common.h"

int dns_query(const char *host, uint16_t port, int family, int socktype, struct sockaddr *addr,
			  socklen_t *addrlen)
{
	struct addrinfo hints = {
		.ai_family = family,
		.ai_socktype = socktype,
	};
	struct addrinfo *res = NULL;
	char addr_str[INET6_ADDRSTRLEN] = {0};
	int rv;

	/* Perform DNS query */
	rv = getaddrinfo(host, NULL, &hints, &res);
	if (rv < 0) {
		LOG_ERR("getaddrinfo failed (%d, errno %d)", rv, errno);
		return rv;
	}
	/* Store the first result */
	*addr = *res->ai_addr;
	*addrlen = res->ai_addrlen;
	/* Free the allocated memory */
	freeaddrinfo(res);
	/* Store the port */
	net_sin(addr)->sin_port = htons(port);
	/* Print the found address */
	inet_ntop(addr->sa_family, &net_sin(addr)->sin_addr, addr_str, sizeof(addr_str));
	LOG_INF("%s -> %s", host, addr_str);
	return 0;
}

static void do_sntp(int family)
{
	char *family_str = family == AF_INET ? "IPv4" : "IPv6";
	struct sntp_time s_time;
	struct sntp_ctx ctx;
	struct sockaddr addr;
	socklen_t addrlen;
	int rv;

	/* Get SNTP server */
	rv = dns_query(CONFIG_NET_SAMPLE_SNTP_SERVER_ADDRESS, CONFIG_NET_SAMPLE_SNTP_SERVER_PORT,
				   family, SOCK_DGRAM, &addr, &addrlen);
	if (rv != 0) {
		LOG_ERR("Failed to lookup %s SNTP server (%d)", family_str, rv);
		return;
	}

	rv = sntp_init(&ctx, &addr, addrlen);
	if (rv < 0) {
		LOG_ERR("Failed to init SNTP %s ctx: %d", family_str, rv);
		goto end;
	}

	LOG_INF("Sending SNTP %s request...", family_str);
	rv = sntp_query(&ctx, 4 * MSEC_PER_SEC, &s_time);
	if (rv < 0) {
		LOG_ERR("SNTP %s request failed: %d", family_str, rv);
		goto end;
	}

	LOG_INF("SNTP Time: %llu", s_time.seconds);

end:
	sntp_close(&ctx);
}

int main(void)
{
	wait_for_network();

	do_sntp(AF_INET);

#if defined(CONFIG_NET_IPV6)
	do_sntp(AF_INET6);
#endif

	return 0;
}
