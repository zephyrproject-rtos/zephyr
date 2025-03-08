/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sntp_client_sample, LOG_LEVEL_DBG);

#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/sntp.h>
#include <arpa/inet.h>

#include "net_sample_common.h"

static K_SEM_DEFINE(sntp_async_received, 0, 1);
static void sntp_service_handler(struct net_socket_service_event *pev);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(service_sntp_async, sntp_service_handler, 1);

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

static void sntp_service_handler(struct net_socket_service_event *pev)
{
	struct sntp_time s_time;
	int rc;

	/* Read the response from the socket */
	rc = sntp_read_async(pev, &s_time);
	if (rc != 0) {
		LOG_ERR("Failed to read SNTP response (%d)", rc);
		return;
	}

	/* Close the service */
	sntp_close_async(&service_sntp_async);

	LOG_INF("SNTP Time: %llu (async)", s_time.seconds);

	/* Notify test thread */
	k_sem_give(&sntp_async_received);
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

	sntp_close(&ctx);

	rv = sntp_init_async(&ctx, &addr, addrlen, &service_sntp_async);
	if (rv < 0) {
		LOG_ERR("Failed to initialise SNTP context (%d)", rv);
		goto end;
	}

	rv = sntp_send_async(&ctx);
	if (rv < 0) {
		LOG_ERR("Failed to send SNTP query (%d)", rv);
		goto end;
	}

	/* Wait for the response to be received asynchronously */
	rv = k_sem_take(&sntp_async_received, K_MSEC(CONFIG_NET_SAMPLE_SNTP_SERVER_TIMEOUT_MS));
	if (rv < 0) {
		LOG_INF("SNTP response timed out (%d)", rv);
	}

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
