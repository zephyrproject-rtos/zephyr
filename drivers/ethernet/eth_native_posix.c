/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Ethernet driver for native posix board. This is meant for network
 * connectivity between host and Zephyr.
 */

#define LOG_MODULE_NAME eth_posix
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdio.h>

#include <kernel.h>
#include <stdbool.h>
#include <errno.h>
#include <stddef.h>

#include <net/net_pkt.h>
#include <net/net_core.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <ethernet/eth_stats.h>

#include <ptp_clock.h>
#include <net/gptp.h>

#include "eth_native_posix_priv.h"

#if defined(CONFIG_NET_L2_ETHERNET)
#define _ETH_MTU 1500
#endif

#define NET_BUF_TIMEOUT K_MSEC(100)

#if defined(CONFIG_NET_VLAN)
#define ETH_HDR_LEN sizeof(struct net_eth_vlan_hdr)
#else
#define ETH_HDR_LEN sizeof(struct net_eth_hdr)
#endif

#if defined(CONFIG_NET_LLDP)
static const struct net_lldpdu lldpdu = {
	.chassis_id = {
		.type_length = htons((LLDP_TLV_CHASSIS_ID << 9) |
			NET_LLDP_CHASSIS_ID_TLV_LEN),
		.subtype = CONFIG_NET_LLDP_CHASSIS_ID_SUBTYPE,
		.value = NET_LLDP_CHASSIS_ID_VALUE
	},
	.port_id = {
		.type_length = htons((LLDP_TLV_PORT_ID << 9) |
			NET_LLDP_PORT_ID_TLV_LEN),
		.subtype = CONFIG_NET_LLDP_PORT_ID_SUBTYPE,
		.value = NET_LLDP_PORT_ID_VALUE
	},
	.ttl = {
		.type_length = htons((LLDP_TLV_TTL << 9) |
			NET_LLDP_TTL_TLV_LEN),
		.ttl = htons(NET_LLDP_TTL)
	},
#if defined(CONFIG_NET_LLDP_END_LLDPDU_TLV_ENABLED)
	.end_lldpdu_tlv = NET_LLDP_END_LLDPDU_VALUE
#endif /* CONFIG_NET_LLDP_END_LLDPDU_TLV_ENABLED */
};

#define lldpdu_ptr (&lldpdu)
#else
#define lldpdu_ptr NULL
#endif /* CONFIG_NET_LLDP */

struct eth_context {
	u8_t recv[_ETH_MTU + ETH_HDR_LEN];
	u8_t send[_ETH_MTU + ETH_HDR_LEN];
	u8_t mac_addr[6];
	struct net_linkaddr ll_addr;
	struct net_if *iface;
	const char *if_name;
	int dev_fd;
	bool init_done;
	bool status;
	bool promisc_mode;

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
#if defined(CONFIG_ETH_NATIVE_POSIX_PTP_CLOCK)
	struct device *ptp_clock;
#endif
};

NET_STACK_DEFINE(RX_ZETH, eth_rx_stack,
		 CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE,
		 CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
static struct k_thread rx_thread_data;

/* TODO: support multiple interfaces */
static struct eth_context eth_context_data;

#if defined(CONFIG_NET_GPTP)
static bool need_timestamping(struct gptp_hdr *hdr)
{
	switch (hdr->message_type) {
	case GPTP_SYNC_MESSAGE:
	case GPTP_PATH_DELAY_RESP_MESSAGE:
		return true;
	default:
		return false;
	}
}

static struct gptp_hdr *check_gptp_msg(struct net_if *iface,
				       struct net_pkt *pkt,
				       bool is_tx)
{
	u8_t *msg_start = net_pkt_data(pkt);
	struct gptp_hdr *gptp_hdr;
	int eth_hlen;

#if defined(CONFIG_NET_VLAN)
	if (net_eth_get_vlan_status(iface)) {
		struct net_eth_vlan_hdr *hdr_vlan;

		hdr_vlan = (struct net_eth_vlan_hdr *)msg_start;
		if (ntohs(hdr_vlan->type) != NET_ETH_PTYPE_PTP) {
			return NULL;
		}

		eth_hlen = sizeof(struct net_eth_vlan_hdr);
	} else
#endif
	{
		struct net_eth_hdr *hdr;

		hdr = (struct net_eth_hdr *)msg_start;
		if (ntohs(hdr->type) != NET_ETH_PTYPE_PTP) {
			return NULL;
		}


		eth_hlen = sizeof(struct net_eth_hdr);
	}

	/* In TX, the first net_buf contains the Ethernet header
	 * and the actual gPTP header is in the second net_buf.
	 * In RX, the Ethernet header + other headers are in the
	 * first net_buf.
	 */
	if (is_tx) {
		if (pkt->frags->frags == NULL) {
			return false;
		}

		gptp_hdr = (struct gptp_hdr *)pkt->frags->frags->data;
	} else {
		gptp_hdr = (struct gptp_hdr *)(pkt->frags->data + eth_hlen);
	}

	return gptp_hdr;
}

static void update_pkt_priority(struct gptp_hdr *hdr, struct net_pkt *pkt)
{
	if (GPTP_IS_EVENT_MSG(hdr->message_type)) {
		net_pkt_set_priority(pkt, NET_PRIORITY_CA);
	} else {
		net_pkt_set_priority(pkt, NET_PRIORITY_IC);
	}
}

static void update_gptp(struct net_if *iface, struct net_pkt *pkt,
			bool send)
{
	struct net_ptp_time timestamp;
	struct gptp_hdr *hdr;
	int ret;

	ret = eth_clock_gettime(&timestamp);
	if (ret < 0) {
		return;
	}

	net_pkt_set_timestamp(pkt, &timestamp);

	hdr = check_gptp_msg(iface, pkt, send);
	if (!hdr) {
		return;
	}

	if (send) {
		ret = need_timestamping(hdr);
		if (ret) {
			net_if_add_tx_timestamp(pkt);
		}
	} else {
		update_pkt_priority(hdr, pkt);
	}
}
#else
#define update_gptp(iface, pkt, send)
#endif /* CONFIG_NET_GPTP */

static int eth_send(struct device *dev, struct net_pkt *pkt)
{
	struct eth_context *ctx = dev->driver_data;
	int count = net_pkt_get_len(pkt);
	int ret;

	ret = net_pkt_read_new(pkt, ctx->send, count);
	if (ret) {
		return ret;
	}

	update_gptp(net_pkt_iface(pkt), pkt, true);

	LOG_DBG("Send pkt %p len %d", pkt, count);

	ret = eth_write_data(ctx->dev_fd, ctx->send, count);
	if (ret < 0) {
		LOG_DBG("Cannot send pkt %p (%d)", pkt, ret);
	}

	return ret < 0 ? ret : 0;
}

static int eth_init(struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static struct net_linkaddr *eth_get_mac(struct eth_context *ctx)
{
	ctx->ll_addr.addr = ctx->mac_addr;
	ctx->ll_addr.len = sizeof(ctx->mac_addr);

	return &ctx->ll_addr;
}

static inline struct net_if *get_iface(struct eth_context *ctx,
				       u16_t vlan_tag)
{
#if defined(CONFIG_NET_VLAN)
	struct net_if *iface;

	iface = net_eth_get_vlan_iface(ctx->iface, vlan_tag);
	if (!iface) {
		return ctx->iface;
	}

	return iface;
#else
	ARG_UNUSED(vlan_tag);

	return ctx->iface;
#endif
}

static int read_data(struct eth_context *ctx, int fd)
{
	u16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	struct net_if *iface;
	struct net_pkt *pkt;
	int count;

	count = eth_read_data(fd, ctx->recv, sizeof(ctx->recv));
	if (count <= 0) {
		return 0;
	}

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, count,
					   AF_UNSPEC, 0, NET_BUF_TIMEOUT);
	if (!pkt) {
		return -ENOMEM;
	}

	if (net_pkt_write_new(pkt, ctx->recv, count)) {
		return -ENOBUFS;
	}

#if defined(CONFIG_NET_VLAN)
	{
		struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);

		if (ntohs(hdr->type) == NET_ETH_PTYPE_VLAN) {
			struct net_eth_vlan_hdr *hdr_vlan =
				(struct net_eth_vlan_hdr *)NET_ETH_HDR(pkt);

			net_pkt_set_vlan_tci(pkt, ntohs(hdr_vlan->vlan.tci));
			vlan_tag = net_pkt_vlan_tag(pkt);
		}

#if CONFIG_NET_TC_RX_COUNT > 1
		{
			enum net_priority prio;

			prio = net_vlan2priority(net_pkt_vlan_priority(pkt));
			net_pkt_set_priority(pkt, prio);
		}
#endif
	}
#endif

	iface = get_iface(ctx, vlan_tag);

	LOG_DBG("Recv pkt %p len %d", pkt, count);

	update_gptp(iface, pkt, false);

	if (net_recv_data(iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}

	return 0;
}

static void eth_rx(struct eth_context *ctx)
{
	int ret;

	LOG_DBG("Starting ZETH RX thread");

	while (1) {
		if (net_if_is_up(ctx->iface)) {
			ret = eth_wait_data(ctx->dev_fd);
			if (!ret) {
				read_data(ctx, ctx->dev_fd);
			} else {
				eth_stats_update_errors_rx(ctx->iface);
			}
		}

		k_sleep(K_MSEC(50));
	}
}

static void create_rx_handler(struct eth_context *ctx)
{
	k_thread_create(&rx_thread_data, eth_rx_stack,
			K_THREAD_STACK_SIZEOF(eth_rx_stack),
			(k_thread_entry_t)eth_rx,
			ctx, NULL, NULL, K_PRIO_COOP(14),
			0, K_NO_WAIT);
}

static void eth_iface_init(struct net_if *iface)
{
	struct eth_context *ctx = net_if_get_device(iface)->driver_data;
	struct net_linkaddr *ll_addr = eth_get_mac(ctx);

	ctx->iface = iface;

	ethernet_init(iface);

	if (ctx->init_done) {
		return;
	}

	net_eth_set_lldpdu(iface, lldpdu_ptr);

	ctx->init_done = true;

#if defined(CONFIG_ETH_NATIVE_POSIX_RANDOM_MAC)
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	ctx->mac_addr[0] = 0x00;
	ctx->mac_addr[1] = 0x00;
	ctx->mac_addr[2] = 0x5E;
	ctx->mac_addr[3] = 0x00;
	ctx->mac_addr[4] = 0x53;
	ctx->mac_addr[5] = sys_rand32_get();

	/* The TUN/TAP setup script will by default set the MAC address of host
	 * interface to 00:00:5E:00:53:FF so do not allow that.
	 */
	if (ctx->mac_addr[5] == 0xff) {
		ctx->mac_addr[5] = 0x01;
	}
#else
	if (CONFIG_ETH_NATIVE_POSIX_MAC_ADDR[0] != 0) {
		if (net_bytes_from_str(ctx->mac_addr, sizeof(ctx->mac_addr),
				       CONFIG_ETH_NATIVE_POSIX_MAC_ADDR) < 0) {
			LOG_ERR("Invalid MAC address %s",
				CONFIG_ETH_NATIVE_POSIX_MAC_ADDR);
		}
	}
#endif

	net_if_set_link_addr(iface, ll_addr->addr, ll_addr->len,
			     NET_LINK_ETHERNET);

	ctx->if_name = ETH_NATIVE_POSIX_DRV_NAME;

	ctx->dev_fd = eth_iface_create(ctx->if_name, false);
	if (ctx->dev_fd < 0) {
		LOG_ERR("Cannot create %s (%d)", ctx->if_name, ctx->dev_fd);
	} else {
		/* Create a thread that will handle incoming data from host */
		create_rx_handler(ctx);

		eth_setup_host(ctx->if_name);

		eth_start_script(ctx->if_name);
	}
}

static
enum ethernet_hw_caps eth_posix_native_get_capabilities(struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_HW_VLAN
#if defined(CONFIG_ETH_NATIVE_POSIX_PTP_CLOCK)
		| ETHERNET_PTP
#endif
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		| ETHERNET_PROMISC_MODE
#endif
#if defined(CONFIG_NET_LLDP)
		| ETHERNET_LLDP
#endif
		;
}

#if defined(CONFIG_ETH_NATIVE_POSIX_PTP_CLOCK)
static struct device *eth_get_ptp_clock(struct device *dev)
{
	struct eth_context *context = dev->driver_data;

	return context->ptp_clock;
}
#endif

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *get_stats(struct device *dev)
{
	struct eth_context *context = dev->driver_data;

	return &(context->stats);
}
#endif

static int set_config(struct device *dev,
		      enum ethernet_config_type type,
		      const struct ethernet_config *config)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE) &&
	    type == ETHERNET_CONFIG_TYPE_PROMISC_MODE) {
		struct eth_context *context = dev->driver_data;

		if (config->promisc_mode) {
			if (context->promisc_mode) {
				return -EALREADY;
			}

			context->promisc_mode = true;
		} else {
			if (!context->promisc_mode) {
				return -EALREADY;
			}

			context->promisc_mode = false;
		}

		ret = eth_promisc_mode(context->if_name,
				       context->promisc_mode);
	}

	return ret;
}

#if defined(CONFIG_NET_VLAN)
static int vlan_setup(struct device *dev, struct net_if *iface,
		      u16_t tag, bool enable)
{
	if (enable) {
		net_eth_set_lldpdu(iface, lldpdu_ptr);
	} else {
		net_eth_unset_lldpdu(iface);
	}

	return 0;
}
#endif /* CONFIG_NET_VLAN */

static int eth_start_device(struct device *dev)
{
	struct eth_context *context = dev->driver_data;
	int ret;

	context->status = true;

	ret = eth_if_up(context->if_name);

	eth_setup_host(context->if_name);

	return ret;
}

static int eth_stop_device(struct device *dev)
{
	struct eth_context *context = dev->driver_data;

	context->status = false;

	return eth_if_down(context->if_name);
}

static const struct ethernet_api eth_if_api = {
	.iface_api.init = eth_iface_init,

	.get_capabilities = eth_posix_native_get_capabilities,
	.set_config = set_config,
	.start = eth_start_device,
	.stop = eth_stop_device,
	.send = eth_send,

#if defined(CONFIG_NET_VLAN)
	.vlan_setup = vlan_setup,
#endif
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = get_stats,
#endif
#if defined(CONFIG_ETH_NATIVE_POSIX_PTP_CLOCK)
	.get_ptp_clock = eth_get_ptp_clock,
#endif
};

ETH_NET_DEVICE_INIT(eth_native_posix, ETH_NATIVE_POSIX_DRV_NAME,
		    eth_init, &eth_context_data, NULL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &eth_if_api,
		    _ETH_MTU);

#if defined(CONFIG_ETH_NATIVE_POSIX_PTP_CLOCK)
struct ptp_context {
	struct eth_context *eth_context;
};

static struct ptp_context ptp_0_context;


static int ptp_clock_set_native_posix(struct device *clk,
				      struct net_ptp_time *tm)
{
	ARG_UNUSED(clk);
	ARG_UNUSED(tm);

	/* We cannot set the host device time so this function
	 * does nothing.
	 */

	return 0;
}

static int ptp_clock_get_native_posix(struct device *clk,
				      struct net_ptp_time *tm)
{
	ARG_UNUSED(clk);

	return eth_clock_gettime(tm);
}

static int ptp_clock_adjust_native_posix(struct device *clk,
					 int increment)
{
	ARG_UNUSED(clk);
	ARG_UNUSED(increment);

	/* We cannot adjust the host device time so this function
	 * does nothing.
	 */

	return 0;
}

static int ptp_clock_rate_adjust_native_posix(struct device *clk,
					      float ratio)
{
	ARG_UNUSED(clk);
	ARG_UNUSED(ratio);

	/* We cannot adjust the host device time so this function
	 * does nothing.
	 */

	return 0;
}

static const struct ptp_clock_driver_api api = {
	.set = ptp_clock_set_native_posix,
	.get = ptp_clock_get_native_posix,
	.adjust = ptp_clock_adjust_native_posix,
	.rate_adjust = ptp_clock_rate_adjust_native_posix,
};

static int ptp_init(struct device *port)
{
	struct device *eth_dev = DEVICE_GET(eth_native_posix);
	struct eth_context *context = eth_dev->driver_data;
	struct ptp_context *ptp_context = port->driver_data;

	context->ptp_clock = port;
	ptp_context->eth_context = context;

	return 0;
}

DEVICE_AND_API_INIT(eth_native_posix_ptp_clock_0, PTP_CLOCK_NAME,
		    ptp_init, &ptp_0_context, NULL, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &api);

#endif /* CONFIG_ETH_NATIVE_POSIX_PTP_CLOCK */
