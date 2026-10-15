/*
 * SPDX-FileCopyrightText: Copyright 2025 L. Felten <lothar.felten@gmail.com>
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_sntp, CONFIG_SNTP_LOG_LEVEL);

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/sntp_server.h>
#include <zephyr/sys/clock.h>
#include "sntp_pkt.h"

/* One listening socket per enabled address family. The IPv6 socket keeps the
 * default IPV6_V6ONLY setting so that it does not collide with the IPv4 one.
 */
#define SNTP_SERVER_SOCKET_COUNT (IS_ENABLED(CONFIG_NET_IPV4) + IS_ENABLED(CONFIG_NET_IPV6))

#define NS2FRAC(x) (uint32_t)((((uint64_t)(x)) << 32) / NSEC_PER_SEC)

/* Until the application tells us where our time comes from, we have to declare
 * ourselves unsynchronized so that clients discard the timestamps we send.
 */
static uint8_t sntp_server_li = SNTP_LEAP_INDICATOR_CLOCK_INVALID;
static uint8_t sntp_server_stratum = SNTP_STRATUM_UNSYNC;
static int8_t sntp_server_precision;
static uint8_t sntp_server_ref_id[4];
static uint32_t sntp_server_ref_tm_s;
static uint32_t sntp_server_ref_tm_f;

/* NTP timestamps are seconds since 1 Jan 1900 plus a binary fraction of a
 * second, both in network byte order.
 */
static uint32_t sntp_seconds(const struct timespec *ts)
{
	return net_htonl((uint32_t)((uint64_t)ts->tv_sec + OFFSET_1970_JAN_1));
}

static uint32_t sntp_fraction(const struct timespec *ts)
{
	return net_htonl(NS2FRAC(ts->tv_nsec));
}

static void sntp_service_handler(struct net_socket_service_event *pev)
{
	struct zsock_pollfd *pfd = &pev->event;
	int sock = pfd->fd;
	struct net_sockaddr_storage addr;
	net_socklen_t addrlen = sizeof(addr);
	struct sntp_pkt request, reply;
	struct timespec now;
	int ret;

	if ((pfd->revents & ZSOCK_POLLIN) == 0) {
		LOG_ERR("Socket %d not readable, revents 0x%x", sock, pfd->revents);
		return;
	}

	ret = zsock_recvfrom(sock, &request, sizeof(request), 0, net_sad(&addr), &addrlen);
	if (ret < 0) {
		LOG_ERR("rx socket error %d", errno);
		return;
	}

	/* Take the receive timestamp as early as possible after the request
	 * arrived, before spending time on validating it.
	 */
	if (sys_clock_gettime(SYS_CLOCK_REALTIME, &now) < 0) {
		LOG_ERR("system clock error");
		return;
	}

	/* A longer datagram, an NTP request carrying an authenticator, is
	 * truncated to the header we are interested in. A shorter one is not
	 * a request at all.
	 */
	if (ret != sizeof(request)) {
		LOG_DBG("Ignoring %d byte datagram", ret);
		return;
	}

	/* Answer client requests only. Replying to another server, or to the
	 * NTP control and private modes, would just create traffic loops.
	 */
	if (request.mode != SNTP_MODE_CLIENT) {
		LOG_DBG("Ignoring mode %u datagram", request.mode);
		return;
	}

	if (request.vn == 0U || request.vn > SNTP_VERSION_MAX) {
		LOG_DBG("Ignoring version %u datagram", request.vn);
		return;
	}

	memset(&reply, 0, sizeof(reply));
	reply.li = sntp_server_li;
	reply.vn = request.vn; /* copy from request */
	reply.mode = SNTP_MODE_SERVER;
	reply.stratum = sntp_server_stratum;
	reply.poll = request.poll; /* copy from request */
	reply.precision = sntp_server_precision;
	reply.root_delay = 0;
	reply.root_dispersion = 0;
	memcpy(&reply.ref_id, sntp_server_ref_id, sizeof(reply.ref_id));
	reply.ref_tm_s = sntp_server_ref_tm_s;
	reply.ref_tm_f = sntp_server_ref_tm_f;
	reply.orig_tm_s = request.tx_tm_s;
	reply.orig_tm_f = request.tx_tm_f;
	reply.rx_tm_s = sntp_seconds(&now);
	reply.rx_tm_f = sntp_fraction(&now);

	if (sys_clock_gettime(SYS_CLOCK_REALTIME, &now) < 0) {
		LOG_ERR("system clock error");
		return;
	}

	reply.tx_tm_s = sntp_seconds(&now);
	reply.tx_tm_f = sntp_fraction(&now);
	sntp_pkt_dump(&reply);

	ret = zsock_sendto(sock, &reply, sizeof(reply), 0, net_sad(&addr), addrlen);
	if (ret < 0) {
		LOG_ERR("tx socket error %d", errno);
	}
}

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(sntp_service, sntp_service_handler, SNTP_SERVER_SOCKET_COUNT);

void sntp_server_clock_source(const uint8_t ref_id[4], uint8_t stratum, int8_t precision)
{
	struct timespec now;

	/* The reference timestamp tells the client when our clock was last
	 * set, which is what the application just did.
	 */
	if (sys_clock_gettime(SYS_CLOCK_REALTIME, &now) == 0) {
		sntp_server_ref_tm_s = sntp_seconds(&now);
		sntp_server_ref_tm_f = sntp_fraction(&now);
	}

	memcpy(sntp_server_ref_id, ref_id, sizeof(sntp_server_ref_id));
	sntp_server_stratum = stratum;
	sntp_server_precision = precision;
	sntp_server_li = SNTP_LEAP_INDICATOR_NONE;
}

static int setup_sntp_service_socket(const struct net_sockaddr *addr, net_socklen_t addrlen)
{
	int sock, ret;

	sock = zsock_socket(addr->sa_family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("socket error: %d", -errno);
		return -errno;
	}

	if (zsock_bind(sock, addr, addrlen) < 0) {
		ret = -errno;
		LOG_ERR("socket bind error: %d", ret);
		(void)zsock_close(sock);
		return ret;
	}

	return sock;
}

static int start_sntp_service(void)
{
	struct zsock_pollfd fds[SNTP_SERVER_SOCKET_COUNT];
	int i, count = 0, ret;

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		struct net_sockaddr_in addr4 = {
			.sin_family = NET_AF_INET,
			.sin_addr = NET_INADDR_ANY_INIT,
			.sin_port = net_htons(SNTP_PORT),
		};

		ret = setup_sntp_service_socket((struct net_sockaddr *)&addr4, sizeof(addr4));
		if (ret < 0) {
			LOG_ERR("failed to setup IPv4 SNTP service socket");
			goto cleanup_sockets;
		}

		fds[count++] = (struct zsock_pollfd){.fd = ret, .events = ZSOCK_POLLIN};
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		struct net_sockaddr_in6 addr6 = {
			.sin6_family = NET_AF_INET6,
			.sin6_addr = NET_IN6ADDR_ANY_INIT,
			.sin6_port = net_htons(SNTP_PORT),
		};

		ret = setup_sntp_service_socket((struct net_sockaddr *)&addr6, sizeof(addr6));
		if (ret < 0) {
			LOG_ERR("failed to setup IPv6 SNTP service socket");
			goto cleanup_sockets;
		}

		fds[count++] = (struct zsock_pollfd){.fd = ret, .events = ZSOCK_POLLIN};
	}

	ret = net_socket_service_register(&sntp_service, fds, count, NULL);
	if (ret < 0) {
		LOG_ERR("registering service handler failed: %d", ret);
		goto cleanup_sockets;
	}

	return 0;

cleanup_sockets:
	for (i = 0; i < count; i++) {
		(void)zsock_close(fds[i].fd);
	}

	return ret;
}

SYS_INIT(start_sntp_service, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
