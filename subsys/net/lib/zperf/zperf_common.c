/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include "zperf_internal.h"
#include "zperf_session.h"

LOG_MODULE_REGISTER(net_zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

/* Get some useful debug routings from net_private.h, requires
 * that NET_LOG_ENABLED is set.
 */
#define NET_LOG_ENABLED 1
#include "net_private.h"

#include "ipv6.h" /* to get infinite lifetime */

static struct sockaddr_in6 in6_addr_my = {
	.sin6_family = AF_INET6,
	.sin6_port = htons(MY_SRC_PORT),
};

static struct sockaddr_in in4_addr_my = {
	.sin_family = AF_INET,
	.sin_port = htons(MY_SRC_PORT),
};

struct sockaddr_in6 *zperf_get_sin6(void)
{
	return &in6_addr_my;
}

struct sockaddr_in *zperf_get_sin(void)
{
	return &in4_addr_my;
}

#define ZPERF_WORK_Q_THREAD_PRIORITY                                                               \
	CLAMP(CONFIG_ZPERF_WORK_Q_THREAD_PRIORITY, K_HIGHEST_APPLICATION_THREAD_PRIO,              \
	      K_LOWEST_APPLICATION_THREAD_PRIO)
K_THREAD_STACK_DEFINE(zperf_work_q_stack, CONFIG_ZPERF_WORK_Q_STACK_SIZE);

static struct k_work_q zperf_work_q;

int zperf_get_ipv6_addr(char *host, char *prefix_str, struct in6_addr *addr)
{
	struct net_if_ipv6_prefix *prefix;
	struct net_if_addr *ifaddr;
	int prefix_len;
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = net_addr_pton(AF_INET6, host, addr);
	if (ret < 0) {
		return -EINVAL;
	}

	prefix_len = strtoul(prefix_str, NULL, 10);

	ifaddr = net_if_ipv6_addr_add(net_if_get_default(),
				      addr, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		NET_ERR("Cannot set IPv6 address %s", host);
		return -EINVAL;
	}

	prefix = net_if_ipv6_prefix_add(net_if_get_default(),
					addr, prefix_len,
					NET_IPV6_ND_INFINITE_LIFETIME);
	if (!prefix) {
		NET_ERR("Cannot set IPv6 prefix %s", prefix_str);
		return -EINVAL;
	}

	return 0;
}


int zperf_get_ipv4_addr(char *host, struct in_addr *addr)
{
	struct net_if_addr *ifaddr;
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = net_addr_pton(AF_INET, host, addr);
	if (ret < 0) {
		return -EINVAL;
	}

	ifaddr = net_if_ipv4_addr_add(net_if_get_default(),
				      addr, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		NET_ERR("Cannot set IPv4 address %s", host);
		return -EINVAL;
	}

	return 0;
}


const struct in_addr *zperf_get_default_if_in4_addr(void)
{
#if CONFIG_NET_IPV4
	return net_if_ipv4_select_src_addr(NULL,
					   net_ipv4_unspecified_address());
#else
	return NULL;
#endif
}

const struct in6_addr *zperf_get_default_if_in6_addr(void)
{
#if CONFIG_NET_IPV6
	return net_if_ipv6_select_src_addr(NULL,
					   net_ipv6_unspecified_address());
#else
	return NULL;
#endif
}

int zperf_prepare_upload_sock(const struct sockaddr *peer_addr, int tos,
			      int priority, int proto)
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

	if (IS_ENABLED(CONFIG_NET_CONTEXT_PRIORITY) && priority >= 0) {
		uint8_t prio = priority;

		if (!IS_ENABLED(CONFIG_NET_ALLOW_ANY_PRIORITY) &&
		    (prio >= NET_MAX_PRIORITIES)) {
			NET_ERR("Priority %d is too large, maximum allowed is %d",
				prio, NET_MAX_PRIORITIES - 1);
			return -EINVAL;
		}

		if (zsock_setsockopt(sock, SOL_SOCKET, SO_PRIORITY,
				     &prio,
				     sizeof(prio)) != 0) {
			NET_WARN("Failed to set SOL_SOCKET - SO_PRIORITY socket option.");
			return -EINVAL;
		}
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

static int zperf_init(void)
{

	k_work_queue_init(&zperf_work_q);
	k_work_queue_start(&zperf_work_q, zperf_work_q_stack,
			   K_THREAD_STACK_SIZEOF(zperf_work_q_stack), ZPERF_WORK_Q_THREAD_PRIORITY,
			   NULL);
	k_thread_name_set(&zperf_work_q.thread, "zperf_work_q");

	zperf_udp_uploader_init();
	zperf_tcp_uploader_init();
	zperf_udp_receiver_init();
	zperf_tcp_receiver_init();

	zperf_session_init();

	if (IS_ENABLED(CONFIG_NET_SHELL)) {
		zperf_shell_init();
	}

	return 0;
}

SYS_INIT(zperf_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
