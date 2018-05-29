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

#define SYS_LOG_DOMAIN "netlo"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_NET_LOOPBACK_LEVEL
#include <logging/sys_log.h>

#include <misc/printk.h>

#include <net/net_pkt.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_if.h>

int loopback_dev_init(struct device *dev)
{
	return 0;
}

static void loopback_init(struct net_if *iface)
{
	/* RFC 7042, s.2.1.1. address to use in documentation */
	net_if_set_link_addr(iface, "\x00\x00\x5e\x00\x53\xff", 6,
			     NET_LINK_DUMMY);
}

static int loopback_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_pkt *cloned;
	int res;

	if (!pkt->frags) {
		SYS_LOG_ERR("No data to send");
		return -ENODATA;
	}

	/* We need to swap the IP addresses because otherwise
	 * the packet will be dropped.
	 */

	if (net_pkt_family(pkt) == AF_INET6) {
		struct in6_addr addr;

		net_ipaddr_copy(&addr, &NET_IPV6_HDR(pkt)->src);
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
				&NET_IPV6_HDR(pkt)->dst);
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst, &addr);
	} else {
		struct in_addr addr;

		net_ipaddr_copy(&addr, &NET_IPV4_HDR(pkt)->src);
		net_ipaddr_copy(&NET_IPV4_HDR(pkt)->src,
				&NET_IPV4_HDR(pkt)->dst);
		net_ipaddr_copy(&NET_IPV4_HDR(pkt)->dst, &addr);
	}

	/* We should simulate normal driver meaning that if the packet is
	 * properly sent (which is always in this driver), then the packet
	 * must be dropped. This is very much needed for TCP packets where
	 * the packet is reference counted in various stages of sending.
	 */
	cloned = net_pkt_clone(pkt, K_MSEC(100));
	if (!cloned) {
		res = -ENOMEM;
		goto out;
	}

	res = net_recv_data(iface, cloned);
	if (res < 0) {
		SYS_LOG_ERR("Data receive failed.");
		goto out;
	}

	net_pkt_unref(pkt);

out:
	/* Let the receiving thread run now */
	k_yield();

	return res;
}

static struct net_if_api loopback_if_api = {
	.init = loopback_init,
	.send = loopback_send,
};

NET_DEVICE_INIT(loopback, "lo",
		loopback_dev_init, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&loopback_if_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 536);
