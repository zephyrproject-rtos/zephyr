/*
 * Copyright (c) 2024 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sntp_sample, LOG_LEVEL_DBG);

#include <time.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/sntp.h>

#include "net_sample_common.h"

#define SNTP_PORT 123

static K_SEM_DEFINE(sntp_async_received, 0, 1);
static void sntp_service_handler(struct net_socket_service_event *pev);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(service_sntp_async, sntp_service_handler, 1);

int dns_query(const char *host, uint16_t port, int family, int socktype, struct sockaddr *addr,
	      socklen_t *addrlen)
{
	struct zsock_addrinfo hints = {
		.ai_family = family,
		.ai_socktype = socktype,
	};
	struct zsock_addrinfo *res = NULL;
	int rc;

	/* Perform DNS query */
	rc = zsock_getaddrinfo(host, NULL, &hints, &res);
	if (rc < 0) {
		LOG_ERR("zsock_getaddrinfo failed (%d, errno %d)", rc, errno);
		return rc;
	}
	/* Store the first result */
	*addr = *res->ai_addr;
	*addrlen = res->ai_addrlen;
	/* Free the allocated memory */
	zsock_freeaddrinfo(res);

	if (addr->sa_family == AF_INET) {
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)addr;
		uint8_t *b = ipv4->sin_addr.s4_addr;

		ipv4->sin_port = htons(port);
		LOG_INF("%s -> %d.%d.%d.%d:%d", host, b[0], b[1], b[2], b[3], port);
	} else {
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)addr;

		ipv6->sin6_port = htons(port);
		LOG_INF("%s -> IPv6:%d", host, port);
	}
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

	LOG_INF(" SNTP Time: %ld", s_time.seconds);

#ifdef CONFIG_NATIVE_LIBC
	time_t local = time(NULL);

	LOG_INF("Local Time: %ld", local);
#endif /* CONFIG_NATIVE_LIBC */

	/* Notify test thread */
	k_sem_give(&sntp_async_received);
}

static int sntp_sync(struct sockaddr *addr, socklen_t addrlen)
{
	struct sntp_ctx ctx;
	struct sntp_time s_time;
	int rc;

	LOG_INF("SNTP query using synchronous APIs");

	rc = sntp_init(&ctx, addr, addrlen);
	if (rc < 0) {
		LOG_ERR("Failed to initialise SNTP context (%d)", rc);
		return rc;
	}

	rc = sntp_query(&ctx, CONFIG_NET_SAMPLE_SNTP_SERVER_TIMEOUT_MS, &s_time);
	if (rc < 0) {
		LOG_ERR("Failed to complete query SNTP (%d)", rc);
		return rc;
	}
	sntp_close(&ctx);

	LOG_INF(" SNTP Time: %ld", s_time.seconds);

#ifdef CONFIG_NATIVE_LIBC
	time_t local = time(NULL);

	LOG_INF("Local Time: %ld", local);
#endif /* CONFIG_NATIVE_LIBC */
	return 0;
}

static int sntp_async(struct sockaddr *addr, socklen_t addrlen)
{
	struct sntp_ctx ctx;
	int rc;

	LOG_INF("SNTP query using asynchronous APIs");

	rc = sntp_init_async(&ctx, addr, addrlen, &service_sntp_async);
	if (rc < 0) {
		LOG_ERR("Failed to initialise SNTP context (%d)", rc);
		return rc;
	}

	rc = sntp_send_async(&ctx);
	if (rc < 0) {
		LOG_ERR("Failed to send SNTP query (%d)", rc);
		return rc;
	}

	/* Wait for the response to be received asynchronously */
	rc = k_sem_take(&sntp_async_received, K_MSEC(CONFIG_NET_SAMPLE_SNTP_SERVER_TIMEOUT_MS));
	if (rc < 0) {
		LOG_INF("SNTP response timed out (%d)", rc);
		return rc;
	}
	return 0;
}

int main(void)
{
	struct sockaddr addr;
	socklen_t addrlen;
	int rc;

	LOG_INF("Starting SNTP sample");

	wait_for_network();

	/* Get SNTP server */
	rc = dns_query(CONFIG_NET_SAMPLE_SNTP_SERVER_ADDRESS, CONFIG_NET_SAMPLE_SNTP_SERVER_PORT, AF_INET, SOCK_DGRAM, &addr, &addrlen);
	if (rc != 0) {
		LOG_ERR("Failed to lookup SNTP response (%d)", rc);
		return rc;
	}

	rc = sntp_sync(&addr, addrlen);
	if (rc < 0) {
		return rc;
	}
	k_sleep(K_SECONDS(1));
	rc = sntp_async(&addr, addrlen);
	if (rc < 0) {
		return rc;
	}
	LOG_INF("SNTP sample complete");
}
