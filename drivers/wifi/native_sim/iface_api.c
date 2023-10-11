/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_native_sim, CONFIG_WIFI_LOG_LEVEL);

#include "../../ethernet/eth.h"

#include "internal.h"
#include "iface_api.h"
#include "wifi_drv_priv.h"

#define NET_BUF_TIMEOUT K_MSEC(100)

static bool extra_debug; /* set to true to hex dump the sent pkt */

static struct net_linkaddr *eth_get_mac(struct wifi_context *ctx)
{
	ctx->ll_addr.addr = ctx->mac_addr;
	ctx->ll_addr.len = sizeof(ctx->mac_addr);

	return &ctx->ll_addr;
}

static struct net_if *get_iface(struct wifi_context *ctx,
				uint16_t vlan_tag)
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

#if defined(CONFIG_NET_VLAN)
static struct net_pkt *prepare_vlan_pkt(struct wifi_context *ctx,
					int count, uint16_t *vlan_tag, int *status)
{
	struct net_eth_vlan_hdr *hdr = (struct net_eth_vlan_hdr *)ctx->recv;
	struct net_pkt *pkt;
	uint8_t pos;

	if (IS_ENABLED(CONFIG_ETH_NATIVE_POSIX_VLAN_TAG_STRIP)) {
		count -= NET_ETH_VLAN_HDR_SIZE;
	}

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, count,
					   AF_UNSPEC, 0, NET_BUF_TIMEOUT);
	if (!pkt) {
		*status = -ENOMEM;
		return NULL;
	}

	net_pkt_set_vlan_tci(pkt, ntohs(hdr->vlan.tci));
	*vlan_tag = net_pkt_vlan_tag(pkt);

	pos = 0;

	if (IS_ENABLED(CONFIG_ETH_NATIVE_POSIX_VLAN_TAG_STRIP)) {
		if (net_pkt_write(pkt, ctx->recv,
				  2 * sizeof(struct net_eth_addr))) {
			goto error;
		}

		pos = (2 * sizeof(struct net_eth_addr)) + NET_ETH_VLAN_HDR_SIZE;
		count -= (2 * sizeof(struct net_eth_addr));
	}

	if (net_pkt_write(pkt, ctx->recv + pos, count)) {
		goto error;
	}

#if CONFIG_NET_TC_RX_COUNT > 1
	{
		enum net_priority prio;

		prio = net_vlan2priority(net_pkt_vlan_priority(pkt));
		net_pkt_set_priority(pkt, prio);
	}
#endif

	*status = 0;

	LOG_DBG("Recv pkt %p len %d", pkt, count);

	return pkt;

error:
	net_pkt_unref(pkt);
	*status = -ENOBUFS;
	return NULL;
}
#endif

static struct net_pkt *prepare_non_vlan_pkt(struct wifi_context *ctx,
					    int count, int *status)
{
	struct net_pkt *pkt;

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, count,
					   AF_UNSPEC, 0, NET_BUF_TIMEOUT);
	if (!pkt) {
		*status = -ENOMEM;
		return NULL;
	}

	if (net_pkt_write(pkt, ctx->recv, count)) {
		net_pkt_unref(pkt);
		*status = -ENOBUFS;
		return NULL;
	}

	*status = 0;

	LOG_DBG("Recv pkt %p len %d", pkt, count);

	return pkt;
}

static int read_data(struct wifi_context *ctx, int fd)
{
	uint16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	struct net_if *iface;
	struct net_pkt *pkt = NULL;
	int status;
	int count;

	count = eth_read_data(fd, ctx->recv, sizeof(ctx->recv));
	if (count <= 0) {
		return 0;
	}

#if defined(CONFIG_NET_VLAN)
	{
		struct net_eth_hdr *hdr = (struct net_eth_hdr *)(ctx->recv);

		if (ntohs(hdr->type) == NET_ETH_PTYPE_VLAN) {
			pkt = prepare_vlan_pkt(ctx, count, &vlan_tag, &status);
			if (!pkt) {
				return status;
			}
		} else {
			pkt = prepare_non_vlan_pkt(ctx, count, &status);
			if (!pkt) {
				return status;
			}

			net_pkt_set_vlan_tci(pkt, 0);
		}
	}
#else
	{
		pkt = prepare_non_vlan_pkt(ctx, count, &status);
		if (!pkt) {
			return status;
		}
	}
#endif

	iface = get_iface(ctx, vlan_tag);

	if (net_recv_data(iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}

	return 0;
}

static void wifi_rx(struct wifi_context *ctx)
{
	LOG_DBG("Starting RX thread");

	while (1) {
		if (net_if_is_up(ctx->iface)) {
			while (!eth_wait_data(ctx->dev_fd)) {
				read_data(ctx, ctx->dev_fd);
				k_yield();
			}
		}

		k_sleep(K_MSEC(CONFIG_WIFI_NATIVE_SIM_RX_TIMEOUT));
	}
}

#if defined(CONFIG_THREAD_MAX_NAME_LEN)
#define THREAD_MAX_NAME_LEN CONFIG_THREAD_MAX_NAME_LEN
#else
#define THREAD_MAX_NAME_LEN 1
#endif

static void create_rx_handler(struct wifi_context *ctx)
{
	k_thread_create(ctx->rx_thread,
			ctx->rx_stack,
			ctx->rx_stack_size,
			(k_thread_entry_t)wifi_rx,
			ctx, NULL, NULL, K_PRIO_COOP(14),
			0, K_NO_WAIT);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		char name[THREAD_MAX_NAME_LEN];

		snprintk(name, sizeof(name), "wifi_native_sim_rx-%s",
			 ctx->if_name_host);
		k_thread_name_set(ctx->rx_thread, name);
	}
}

void wifi_if_init(struct net_if *iface)
{
	struct wifi_context *ctx = net_if_get_device(iface)->data;
	struct net_linkaddr *ll_addr = eth_get_mac(ctx);
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;

	/* The iface pointer in context should contain the main interface
	 * if the VLANs are enabled.
	 */
	if (ctx->iface == NULL) {
		ctx->iface = iface;
	}

	ethernet_init(iface);

	if (ctx->init_done) {
		return;
	}

	ctx->init_done = true;

	ctx->if_index = net_if_get_by_iface(iface);

#if defined(CONFIG_WIFI_NATIVE_SIM_RANDOM_MAC)
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	gen_random_mac(ctx->mac_addr, 0x00, 0x00, 0x5E);

	ctx->mac_addr[3] = 0x00;
	ctx->mac_addr[4] = 0x53;

	/* The TUN/TAP setup script will by default set the MAC address of host
	 * interface to 00:00:5E:00:53:FF so do not allow that.
	 */
	if (ctx->mac_addr[5] == 0xff) {
		ctx->mac_addr[5] = 0x01;
	}
#else
	/* Difficult to configure MAC addresses any sane way if we have more
	 * than one network interface.
	 */
	BUILD_ASSERT(CONFIG_WIFI_NATIVE_SIM_INTERFACE_COUNT == 1,
		     "Cannot have static MAC if interface count > 1");

	if (CONFIG_WIFI_NATIVE_SIM_MAC_ADDR[0] != 0) {
		if (net_bytes_from_str(ctx->mac_addr, sizeof(ctx->mac_addr),
				       CONFIG_WIFI_NATIVE_SIM_MAC_ADDR) < 0) {
			LOG_ERR("Invalid MAC address %s",
				CONFIG_WIFI_NATIVE_SIM_MAC_ADDR);
		}
	}
#endif

	/* If we have only one network interface, then use the name
	 * defined in the Kconfig directly. This way there is no need to
	 * change the documentation etc. and break things.
	 */
	if (CONFIG_WIFI_NATIVE_SIM_INTERFACE_COUNT == 1) {
		ctx->if_name_host = CONFIG_WIFI_NATIVE_SIM_DRV_NAME;
	}

	LOG_DBG("Interface %p using \"%s\"", iface, ctx->if_name_host);

	net_if_set_link_addr(iface, ll_addr->addr, ll_addr->len,
			     NET_LINK_ETHERNET);

	ctx->dev_fd = eth_iface_create(ctx->if_name_host);
	if (ctx->dev_fd < 0) {
		LOG_ERR("Cannot create %s (%d)", ctx->if_name_host, ctx->dev_fd);
	} else {
		/* Create a thread that will handle incoming data from host */
		create_rx_handler(ctx);
	}
}

int wifi_if_start(const struct device *dev)
{
	struct wifi_context *context = dev->data;

	context->status = true;
	return 0;
}

int wifi_if_stop(const struct device *dev)
{
	struct wifi_context *context = dev->data;

	context->status = false;
	return 0;
}

int wifi_if_set_config(const struct device *dev,
		       enum ethernet_config_type type,
		       const struct ethernet_config *config)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE) &&
	    type == ETHERNET_CONFIG_TYPE_PROMISC_MODE) {
		struct wifi_context *context = dev->data;

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

		ret = eth_promisc_mode(context->if_name_host,
				       context->promisc_mode);
	} else if (type == ETHERNET_CONFIG_TYPE_MAC_ADDRESS) {
		struct wifi_context *context = dev->data;

		memcpy(context->mac_addr, config->mac_address.addr,
		       sizeof(context->mac_addr));
	}

	return ret;
}

#ifdef CONFIG_NET_STATISTICS_ETHERNET
struct net_stats_eth *wifi_if_stats_get(const struct device *dev)
{
	struct wifi_context *context = dev->data;

	return &(context->stats_eth);
}
#endif

enum ethernet_hw_caps wifi_if_caps_get(const struct device *dev)
{
	enum ethernet_hw_caps caps = (ETHERNET_LINK_10BASE_T |
			ETHERNET_LINK_100BASE_T | ETHERNET_LINK_1000BASE_T);

	return caps;
}

int wifi_if_send(const struct device *dev, struct net_pkt *pkt)
{
	struct wifi_context *ctx = dev->data;
	int count = net_pkt_get_len(pkt);
	int ret;

	if (ctx->name[0] == '\0') {
		ret = net_if_get_name(ctx->iface, ctx->name, sizeof(ctx->name) - 1);
		if (ret < 0) {
			LOG_DBG("Cannot get interface name for %d", ctx->if_index);
		}
	}

	ret = net_pkt_read(pkt, ctx->send, count);
	if (ret) {
		return ret;
	}

	LOG_DBG("Send pkt %p len %d", pkt, count);

	if (extra_debug) {
		LOG_HEXDUMP_DBG(ctx->send, count, "pkt");
	}

	ret = eth_write_data(ctx->dev_fd, ctx->send, count);
	if (ret < 0) {
		LOG_DBG("Cannot send pkt %p (%d)", pkt, ret);
	}

	return ret < 0 ? ret : 0;
}
