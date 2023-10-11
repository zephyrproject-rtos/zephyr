/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Network loopback interface implementation.
 */

#define LOG_MODULE_NAME netlo
#define LOG_LEVEL CONFIG_NET_LOOPBACK_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/loopback.h>

#include <zephyr/net/dummy.h>

int loopback_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static void loopback_init(struct net_if *iface)
{
	struct net_if_addr *ifaddr;

	/* RFC 7042, s.2.1.1. address to use in documentation */
	net_if_set_link_addr(iface, "\x00\x00\x5e\x00\x53\xff", 6,
			     NET_LINK_DUMMY);

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		struct in_addr ipv4_loopback = INADDR_LOOPBACK_INIT;

		ifaddr = net_if_ipv4_addr_add(iface, &ipv4_loopback,
					      NET_ADDR_AUTOCONF, 0);
		if (!ifaddr) {
			LOG_ERR("Failed to register IPv4 loopback address");
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		struct in6_addr ipv6_loopback = IN6ADDR_LOOPBACK_INIT;

		ifaddr = net_if_ipv6_addr_add(iface, &ipv6_loopback,
					      NET_ADDR_AUTOCONF, 0);
		if (!ifaddr) {
			LOG_ERR("Failed to register IPv6 loopback address");
		}
	}
}

#ifdef CONFIG_NET_LOOPBACK_SIMULATE_PACKET_DROP
static float loopback_packet_drop_ratio = 0.0f;
static float loopback_packet_drop_state = 0.0f;
static int loopback_packet_dropped_count;

int loopback_set_packet_drop_ratio(float ratio)
{
	if (ratio < 0.0f || ratio > 1.0f) {
		return -EINVAL;
	}
	loopback_packet_drop_ratio = ratio;
	return 0;
}

int loopback_get_num_dropped_packets(void)
{
	return loopback_packet_dropped_count;
}

#endif

static int loopback_send(const struct device *dev, struct net_pkt *pkt)
{
	struct net_pkt *cloned;
	int res;

	ARG_UNUSED(dev);

#ifdef CONFIG_NET_LOOPBACK_SIMULATE_PACKET_DROP
	/* Drop packets based on the loopback_packet_drop_ratio
	 * a ratio of 0.2 will drop one every 5 packets
	 */
	loopback_packet_drop_state += loopback_packet_drop_ratio;
	if (loopback_packet_drop_state >= 1.0f) {
		/* Administrate we dropped a packet */
		loopback_packet_drop_state -= 1.0f;
		loopback_packet_dropped_count++;
		return 0;
	}
#endif

	if (!pkt->frags) {
		LOG_ERR("No data to send");
		return -ENODATA;
	}

	/* We need to swap the IP addresses because otherwise
	 * the packet will be dropped.
	 */

	if (net_pkt_family(pkt) == AF_INET6) {
		struct in6_addr addr;

		net_ipv6_addr_copy_raw((uint8_t *)&addr, NET_IPV6_HDR(pkt)->src);
		net_ipv6_addr_copy_raw(NET_IPV6_HDR(pkt)->src,
				       NET_IPV6_HDR(pkt)->dst);
		net_ipv6_addr_copy_raw(NET_IPV6_HDR(pkt)->dst, (uint8_t *)&addr);
	} else {
		struct in_addr addr;

		net_ipv4_addr_copy_raw((uint8_t *)&addr, NET_IPV4_HDR(pkt)->src);
		net_ipv4_addr_copy_raw(NET_IPV4_HDR(pkt)->src,
				       NET_IPV4_HDR(pkt)->dst);
		net_ipv4_addr_copy_raw(NET_IPV4_HDR(pkt)->dst, (uint8_t *)&addr);
	}

	/* We should simulate normal driver meaning that if the packet is
	 * properly sent (which is always in this driver), then the packet
	 * must be dropped. This is very much needed for TCP packets where
	 * the packet is reference counted in various stages of sending.
	 */
	cloned = net_pkt_rx_clone(pkt, K_MSEC(100));
	if (!cloned) {
		res = -ENOMEM;
		goto out;
	}

	res = net_recv_data(net_pkt_iface(cloned), cloned);
	if (res < 0) {
		LOG_ERR("Data receive failed.");
	}

out:
	/* Let the receiving thread run now */
	k_yield();

	return res;
}

static struct dummy_api loopback_api = {
	.iface_api.init = loopback_init,

	.send = loopback_send,
};

NET_DEVICE_INIT(loopback, "lo",
		loopback_dev_init, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&loopback_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), CONFIG_NET_LOOPBACK_MTU);
