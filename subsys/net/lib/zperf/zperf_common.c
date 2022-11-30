/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include "zperf_internal.h"
#include "zperf_session.h"

LOG_MODULE_DECLARE(net_zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

#define ZPERF_WORK_Q_THREAD_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO
#define ZPERF_WORK_Q_STACK_SIZE 2048

K_THREAD_STACK_DEFINE(zperf_work_q_stack, ZPERF_WORK_Q_STACK_SIZE);

static struct k_work_q zperf_work_q;

int zperf_prepare_upload_sock(const struct sockaddr *peer_addr, int tos,
			      int proto)
{
	socklen_t addrlen = peer_addr->sa_family == AF_INET6 ?
			    sizeof(struct sockaddr_in6) :
			    sizeof(struct sockaddr_in);
	int type = (proto == IPPROTO_UDP) ? SOCK_DGRAM : SOCK_STREAM;
	int sock = -1;
	int ret;

	switch (peer_addr->sa_family) {
	case AF_INET:
		if (!IS_ENABLED(CONFIG_NET_IPV4)) {
			NET_ERR("IPv4 not available.");
			return -EINVAL;
		}

		sock = zsock_socket(AF_INET, type, proto);
		if (sock < 0) {
			NET_ERR("Cannot create IPv4 network socket (%d)",
				errno);
			return -errno;
		}

		if (tos > 0) {
			if (zsock_setsockopt(sock, IPPROTO_IP, IP_TOS,
					     &tos, sizeof(tos)) != 0) {
				NET_WARN("Failed to set IP_TOS socket option. "
					 "Please enable CONFIG_NET_CONTEXT_DSCP_ECN.");
			}
		}

		break;

	case AF_INET6:
		if (!IS_ENABLED(CONFIG_NET_IPV6)) {
			NET_ERR("IPv6 not available.");
			return -EINVAL;
		}

		sock = zsock_socket(AF_INET6, type, proto);
		if (sock < 0) {
			NET_ERR("Cannot create IPv6 network socket (%d)",
				errno);
			return -errno;
		}

		if (tos >= 0) {
			if (zsock_setsockopt(sock, IPPROTO_IPV6, IPV6_TCLASS,
					     &tos, sizeof(tos)) != 0) {
				NET_WARN("Failed to set IPV6_TCLASS socket option. "
					 "Please enable CONFIG_NET_CONTEXT_DSCP_ECN.");
			}
		}

		break;

	default:
		LOG_ERR("Invalid address family (%d)", peer_addr->sa_family);
		return -EINVAL;
	}

	ret = zsock_connect(sock, peer_addr, addrlen);
	if (ret < 0) {
		NET_ERR("Connect failed (%d)", errno);
		ret = -errno;
		zsock_close(sock);
		return ret;
	}

	return sock;
}

uint32_t zperf_packet_duration(uint32_t packet_size, uint32_t rate_in_kbps)
{
	return (uint32_t)(((uint64_t)packet_size * 8U * USEC_PER_SEC) /
			  (rate_in_kbps * 1024U));
}

void zperf_async_work_submit(struct k_work *work)
{
	k_work_submit_to_queue(&zperf_work_q, work);
}

static int zperf_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	k_work_queue_init(&zperf_work_q);
	k_work_queue_start(&zperf_work_q, zperf_work_q_stack,
			   K_THREAD_STACK_SIZEOF(zperf_work_q_stack),
			   ZPERF_WORK_Q_THREAD_PRIORITY, NULL);
	k_thread_name_set(&zperf_work_q.thread, "zperf_work_q");

	zperf_udp_uploader_init();
	zperf_tcp_uploader_init();
	zperf_udp_receiver_init();
	zperf_tcp_receiver_init();

	zperf_session_init();
	zperf_shell_init();

	return 0;
}

SYS_INIT(zperf_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
