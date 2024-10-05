/*
 * Copyright (c) 2024 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>

#include <zephyr/ztest.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/sntp.h>

#define SNTP_PORT 123

static K_SEM_DEFINE(sntp_async_received, 0, 1);
static void sntp_service_handler(struct k_work *work);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(service_sntp_async, NULL, sntp_service_handler, 1);

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
		printk("%s -> %d.%d.%d.%d:%d\n", host, b[0], b[1], b[2], b[3], port);
	} else {
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)addr;

		ipv6->sin6_port = htons(port);
		printk("%s -> IPv6:%d\n", host, port);
	}
	return 0;
}

ZTEST(sntp, test_sntp_init_error)
{
	struct sntp_ctx ctx;
	struct sockaddr addr;

	zassert_equal(-EFAULT, sntp_init(NULL, &addr, 0));
	zassert_equal(-EFAULT, sntp_init(&ctx, NULL, 0));

	zassert_equal(-EFAULT, sntp_init_async(&ctx, &addr, 0, NULL));
	zassert_equal(-EFAULT, sntp_init_async(NULL, &addr, 0, &service_sntp_async));
	zassert_equal(-EFAULT, sntp_init_async(&ctx, NULL, 0, &service_sntp_async));
}

ZTEST(sntp, test_sntp_sync)
{
	struct sntp_ctx ctx;
	struct sntp_time s_time;
	struct sockaddr addr;
	socklen_t addrlen;
	time_t local;

	/* Get SNTP server */
	zassert_equal(0, dns_query(CONFIG_SNTP_SERVER_ADDRESS, CONFIG_SNTP_SERVER_PORT, AF_INET,
				   SOCK_DGRAM, &addr, &addrlen));

	/* Standard API sequence */
	zassert_equal(0, sntp_init(&ctx, &addr, addrlen));
	zassert_equal(0, sntp_query(&ctx, CONFIG_SNTP_SERVER_TIMEOUT_MS, &s_time));
	sntp_close(&ctx);

	local = time(NULL);
	printk("Local Time: %ld\n", local);
	printk(" SNTP Time: %ld\n", s_time.seconds);

	/* Validate response approximately matches system time */
	zassert_within(local, s_time.seconds, 2);
}

ZTEST(sntp, test_sntp_sync_timeout)
{
	struct sntp_ctx ctx;
	struct sntp_time s_time;
	struct sockaddr addr;
	socklen_t addrlen;

	/* Not a valid SNTP server */
	zassert_equal(0, dns_query("www.google.com", CONFIG_SNTP_SERVER_PORT, AF_INET, SOCK_DGRAM,
				   &addr, &addrlen));

	/* Call should timeout */
	zassert_equal(0, sntp_init(&ctx, &addr, addrlen));
	zassert_equal(-ETIMEDOUT, sntp_query(&ctx, CONFIG_SNTP_SERVER_TIMEOUT_MS, &s_time));
	sntp_close(&ctx);
}

static void sntp_service_handler(struct k_work *work)
{
	struct net_socket_service_event *sev =
		CONTAINER_OF(work, struct net_socket_service_event, work);
	struct sntp_time s_time;
	time_t local = time(NULL);

	/* Read the response from the socket */
	zassert_equal(0, sntp_read_async(sev, &s_time));

	/* Close the service */
	sntp_close_async(&service_sntp_async);

	printk("Local Time: %ld\n", local);
	printk(" SNTP Time: %ld\n", s_time.seconds);

	/* Validate response approximately matches system time */
	zassert_within(local, s_time.seconds, 2);

	/* Notify test thread */
	k_sem_give(&sntp_async_received);
}

ZTEST(sntp, test_sntp_async)
{
	struct sntp_ctx ctx;
	struct sockaddr addr;
	socklen_t addrlen;

	/* Get SNTP server */
	zassert_equal(0, dns_query(CONFIG_SNTP_SERVER_ADDRESS, CONFIG_SNTP_SERVER_PORT, AF_INET,
				   SOCK_DGRAM, &addr, &addrlen));

	/* Send the SNTP query */
	zassert_equal(0, sntp_init_async(&ctx, &addr, addrlen, &service_sntp_async));
	zassert_equal(0, sntp_send_async(&ctx));

	/* Wait for the response to be received asynchronously */
	zassert_equal(0, k_sem_take(&sntp_async_received, K_MSEC(CONFIG_SNTP_SERVER_TIMEOUT_MS)));
}

ZTEST_SUITE(sntp, NULL, NULL, NULL, NULL, NULL);
