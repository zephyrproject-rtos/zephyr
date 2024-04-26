/*
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wifi_w91.h"
#include <zephyr/net/net_pkt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(w91_wifi_l2, CONFIG_WIFI_LOG_LEVEL);

static enum net_verdict wifi_w91_recv(struct net_if *iface, struct net_pkt *pkt)
{
	ARG_UNUSED(iface);

	net_pkt_lladdr_src(pkt)->addr = NULL;
	net_pkt_lladdr_src(pkt)->len = 0U;
	net_pkt_lladdr_src(pkt)->type = NET_LINK_ETHERNET;
	net_pkt_lladdr_dst(pkt)->addr = NULL;
	net_pkt_lladdr_dst(pkt)->len = 0U;
	net_pkt_lladdr_dst(pkt)->type = NET_LINK_ETHERNET;

	return NET_CONTINUE;
}

static int wifi_w91_send(struct net_if *iface, struct net_pkt *pkt)
{
	int ret = 0;

	do {
		const struct device *dev = net_if_get_device(iface);
		struct wifi_w91_data *data = dev->data;
		size_t tx_len = net_pkt_get_len(pkt);

		if (!tx_len) {
			break;
		}
		if (tx_len > sizeof(data->l2.ipc_tx.tx)) {
			LOG_ERR("data exceeds MTU size");
			ret = -ENOBUFS;
			break;
		}
		ret = net_pkt_read(pkt, data->l2.ipc_tx.tx, tx_len);
		if (ret < 0) {
			LOG_ERR("data read error");
			break;
		}
		net_capture_pkt(iface, pkt);
		LOG_HEXDUMP_DBG(data->l2.ipc_tx.tx, tx_len, "TX");
		/* TODO: IPC data send */
#if CONFIG_WIFI_W91_L2_ECHO
	/*******************************************************
	 * We need to swap the IP addresses because otherwise
	 * the packet will be dropped.
	 *******************************************************/
#if CONFIG_NET_IPV6
	if (net_pkt_family(pkt) == AF_INET6) {
		struct in6_addr addr;

		net_ipv6_addr_copy_raw((uint8_t *)&addr, NET_IPV6_HDR(pkt)->src);
		net_ipv6_addr_copy_raw(NET_IPV6_HDR(pkt)->src,
				       NET_IPV6_HDR(pkt)->dst);
		net_ipv6_addr_copy_raw(NET_IPV6_HDR(pkt)->dst, (uint8_t *)&addr);
	}
#endif /* CONFIG_NET_IPV6 */
#if CONFIG_NET_IPV4
	if (net_pkt_family(pkt) == AF_INET) {
		struct in_addr addr;

		net_ipv4_addr_copy_raw((uint8_t *)&addr, NET_IPV4_HDR(pkt)->src);
		net_ipv4_addr_copy_raw(NET_IPV4_HDR(pkt)->src,
				       NET_IPV4_HDR(pkt)->dst);
		net_ipv4_addr_copy_raw(NET_IPV4_HDR(pkt)->dst, (uint8_t *)&addr);
	}
#endif /* CONFIG_NET_IPV4 */
	/*******************************************************
	 * We should simulate normal driver meaning that if the packet is
	 * properly sent (which is always in this driver), then the packet
	 * must be dropped. This is very much needed for TCP packets where
	 * the packet is reference counted in various stages of sending.
	 *******************************************************/
	struct net_pkt *cloned = net_pkt_rx_clone(pkt, K_MSEC(100));

	if (!cloned) {
		LOG_ERR("create packet failed");
		ret = -ENOMEM;
		break;
	}

	ret = net_recv_data(net_pkt_iface(cloned), cloned);
	if (ret < 0) {
		LOG_ERR("data receive failed");
		break;
	}

#endif /* CONFIG_WIFI_W91_L2_ECHO */
	} while (0);

	if (ret >= 0) {
		net_pkt_unref(pkt);
	}

	return ret;
}

static int wifi_w91_enable(struct net_if *iface, bool state)
{
	LOG_INF("%s [%u]", __func__, state);

	if (state) {
		const struct device *dev = net_if_get_device(iface);
		struct wifi_w91_data *data = dev->data;

		/* TODO: Foll MAC address here */
		(void) net_if_set_link_addr(iface, data->l2.mac,
			WIFI_MAC_ADDR_LEN, NET_LINK_ETHERNET);
	}

	return 0;
}

static enum net_l2_flags wifi_w91_flags(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return NET_L2_MULTICAST;
}

NET_L2_INIT(W91_WIFI_L2, wifi_w91_recv, wifi_w91_send, wifi_w91_enable, wifi_w91_flags);
