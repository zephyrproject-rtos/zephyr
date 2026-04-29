/*
 * Copyright (c) 2025 Lothar Felten <lothar.felten@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_sntp, CONFIG_SNTP_LOG_LEVEL);

#include <errno.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/clock.h>
#include <zephyr/net/socket_service.h>
#include "sntp_pkt.h"

#define SNTP_SERVER_BUFFER_SIZE 1492

#define NS2FRAC(x) (uint32_t)((((uint64_t)x) << 32) / 1000000000ULL)

K_MUTEX_DEFINE(lock);
static int udp_socket = -1;
static unsigned char sntp_server_stratum;
static char sntp_server_precision;
static char sntp_server_ref_id[4];

static void sntp_service_handler(struct net_socket_service_event *pev)
{
	static char rxbuffer[SNTP_SERVER_BUFFER_SIZE];
	struct zsock_pollfd *pfd = &pev->event;
	int client = pfd->fd;
	struct net_sockaddr addr;
	net_socklen_t addrlen;
	struct sntp_pkt sntp_reply, *sntp_request;
	struct timespec now;
	int ret;

	ret = sys_clock_gettime(SYS_CLOCK_REALTIME, &now);
	if (ret < 0) {
		LOG_ERR("system clock error");
		return;
	}

	ret = zsock_recvfrom(client, rxbuffer, SNTP_SERVER_BUFFER_SIZE, 0, &addr, &addrlen);
	if (ret < 0) {
		LOG_ERR("rx socket error %d", errno);
		return;
	}
	if (ret != sizeof(struct sntp_pkt)) {
		LOG_ERR("received malformed message");
		return;
	}

	sntp_request = (struct sntp_pkt *)rxbuffer;
	sntp_reply.mode = SNTP_MODE_SERVER;
	sntp_reply.vn = sntp_request->vn; /* copy from request */
	sntp_reply.li = SNTP_LEAP_INDICATOR_NONE;
	sntp_reply.stratum = sntp_server_stratum;
	sntp_reply.poll = sntp_request->poll; /* copy from request */
	sntp_reply.precision = sntp_server_precision;
	sntp_reply.root_delay = 0;
	sntp_reply.root_dispersion = 0;
	strncpy((char *)(&sntp_reply.ref_id), sntp_server_ref_id, sizeof(sntp_reply.ref_id));
	sntp_reply.ref_tm_s = 0;
	sntp_reply.orig_tm_s = sntp_request->tx_tm_s;
	sntp_reply.orig_tm_f = sntp_request->tx_tm_f;
	sntp_reply.rx_tm_s = net_htonl(now.tv_sec + OFFSET_1970_JAN_1);
	sntp_reply.rx_tm_f = net_htonl(NS2FRAC(now.tv_nsec));
	ret = sys_clock_gettime(SYS_CLOCK_REALTIME, &now);
	if (ret < 0) {
		LOG_ERR("system clock error");
		return;
	}
	sntp_reply.tx_tm_s = net_htonl(now.tv_sec + OFFSET_1970_JAN_1);
	sntp_reply.tx_tm_f = net_htonl(NS2FRAC(now.tv_nsec));
	sntp_pkt_dump(&sntp_reply);
	ret = zsock_sendto(client, &sntp_reply, sizeof(struct sntp_pkt), 0, &addr, addrlen);
	if (ret < 0) {
		LOG_ERR("tx socket error %d", errno);
	}
}

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(sntp_service, sntp_service_handler, 1);

int sntp_server_clock_source(unsigned char *refid, unsigned char stratum, char precision)
{
	sntp_server_stratum = stratum;
	sntp_server_precision = precision;
	strncpy(sntp_server_ref_id, refid, sizeof(sntp_server_ref_id));
	return 0;
}

static int setup_sntp_service_socket(struct net_sockaddr_in6 *addr)
{
	net_socklen_t optlen = sizeof(int);
	int ret, sock, opt;

	sock = zsock_socket(NET_AF_INET6, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("socket error: %d", -errno);
		return -errno;
	}

	ret = zsock_getsockopt(sock, NET_IPPROTO_IPV6, ZSOCK_IPV6_V6ONLY, &opt, &optlen);
	if (ret == 0 && opt != 0) {
		opt = 0;
		ret = zsock_setsockopt(sock, NET_IPPROTO_IPV6, ZSOCK_IPV6_V6ONLY, &opt, optlen);
		if (ret < 0) {
			LOG_WRN("disabling ZSOCK_IPV6_V6ONLY failed");
		}
	}

	if (zsock_bind(sock, (struct net_sockaddr *)addr, sizeof(*addr)) < 0) {
		LOG_ERR("socket bind error: %d", -errno);
		zsock_close(sock);
		return -errno;
	}

	return sock;
}

static int start_sntp_service(void)
{
	struct zsock_pollfd sockfd_udp;
	int udp_sock, ret;
	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = NET_IN6ADDR_ANY_INIT,
		.sin6_port = net_htons(SNTP_PORT),
	};

	sntp_server_stratum = 255;
	sntp_server_precision = 0;
	udp_sock = setup_sntp_service_socket(&addr);
	if (udp_sock < 0) {
		LOG_ERR("failed to setup SNTP service socket");
		ret = udp_sock;
		return -1;
	}

	sockfd_udp = (struct zsock_pollfd){.fd = udp_sock, .events = ZSOCK_POLLIN};
	ret = net_socket_service_register(&sntp_service, &sockfd_udp, 1, NULL);
	if (ret < 0) {
		LOG_ERR("registering service handler failed: %d", ret);
		goto cleanup_sockets;
	}

	k_mutex_lock(&lock, K_FOREVER);
	udp_socket = udp_sock;
	k_mutex_unlock(&lock);

	LOG_INF("service started");
	return 0;

cleanup_sockets:
	zsock_close(udp_sock);
	return -1;
}

static int __maybe_unused stop_sntp_service(void)
{
	(void)net_socket_service_unregister(&sntp_service);

	k_mutex_lock(&lock, K_FOREVER);
	if (udp_socket >= 0) {
		zsock_close(udp_socket);
		udp_socket = -1;
	}
	k_mutex_unlock(&lock);

	return 0;
}

SYS_INIT(start_sntp_service, APPLICATION, 99);
