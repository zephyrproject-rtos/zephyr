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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdio.h>

#include <zephyr/kernel.h>
#include <stdbool.h>
#include <errno.h>
#include <stddef.h>

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth_stats.h>

#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/net/gptp.h>
#include <zephyr/net/lldp.h>

#include "eth_native_posix_priv.h"
#include "eth.h"

#define NET_BUF_TIMEOUT K_MSEC(100)

#if defined(CONFIG_NET_VLAN)
#define ETH_HDR_LEN sizeof(struct net_eth_vlan_hdr)
#else
#define ETH_HDR_LEN sizeof(struct net_eth_hdr)
#endif

struct eth_context {
	uint8_t recv[NET_ETH_MTU + ETH_HDR_LEN];
	uint8_t send[NET_ETH_MTU + ETH_HDR_LEN];
	uint8_t mac_addr[6];
	struct net_linkaddr ll_addr;
	struct net_if *iface;
	const char *if_name;
	k_tid_t rx_thread;
	struct z_thread_stack_element *rx_stack;
	size_t rx_stack_size;
	int dev_fd;
	bool init_done;
	bool status;
	bool promisc_mode;

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
#if defined(CONFIG_ETH_NATIVE_POSIX_PTP_CLOCK)
	const struct device *ptp_clock;
#endif
};

#define DEFINE_RX_THREAD(x, _)						\
	K_KERNEL_STACK_DEFINE(rx_thread_stack_##x,			\
			      CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);\
	static struct k_thread rx_thread_data_##x

LISTIFY(CONFIG_ETH_NATIVE_POSIX_INTERFACE_COUNT, DEFINE_RX_THREAD, (;), _);

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
	uint8_t *msg_start = net_pkt_data(pkt);
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

static int eth_send(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_context *ctx = dev->data;
	int count = net_pkt_get_len(pkt);
	int ret;

	ret = net_pkt_read(pkt, ctx->send, count);
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

static int eth_init(const struct device *dev)
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
static struct net_pkt *prepare_vlan_pkt(struct eth_context *ctx,
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

static struct net_pkt *prepare_non_vlan_pkt(struct eth_context *ctx,
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

static int read_data(struct eth_context *ctx, int fd)
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

	update_gptp(iface, pkt, false);

	if (net_recv_data(iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}

	return 0;
}

static void eth_rx(struct eth_context *ctx)
{
	LOG_DBG("Starting ZETH RX thread");

	while (1) {
		if (net_if_is_up(ctx->iface)) {
			while (!eth_wait_data(ctx->dev_fd)) {
				read_data(ctx, ctx->dev_fd);
				k_yield();
			}
		}

		if (IS_ENABLED(CONFIG_NET_GPTP)) {
			k_sleep(K_MSEC(1));
		} else {
			k_sleep(K_MSEC(50));
		}
	}
}

#if defined(CONFIG_THREAD_MAX_NAME_LEN)
#define THREAD_MAX_NAME_LEN CONFIG_THREAD_MAX_NAME_LEN
#else
#define THREAD_MAX_NAME_LEN 1
#endif

static void create_rx_handler(struct eth_context *ctx)
{
	k_thread_create(ctx->rx_thread,
			ctx->rx_stack,
			ctx->rx_stack_size,
			(k_thread_entry_t)eth_rx,
			ctx, NULL, NULL, K_PRIO_COOP(14),
			0, K_NO_WAIT);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		char name[THREAD_MAX_NAME_LEN];

		snprintk(name, sizeof(name), "eth_native_posix_rx-%s",
			 ctx->if_name);
		k_thread_name_set(ctx->rx_thread, name);
	}
}

static void eth_iface_init(struct net_if *iface)
{
	struct eth_context *ctx = net_if_get_device(iface)->data;
	struct net_linkaddr *ll_addr = eth_get_mac(ctx);

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

	net_lldp_set_lldpdu(iface);

	ctx->init_done = true;

#if IS_ENABLED(CONFIG_ETH_NATIVE_POSIX_RANDOM_MAC)
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
	BUILD_ASSERT(CONFIG_ETH_NATIVE_POSIX_INTERFACE_COUNT == 1,
		     "Cannot have static MAC if interface count > 1");

	if (CONFIG_ETH_NATIVE_POSIX_MAC_ADDR[0] != 0) {
		if (net_bytes_from_str(ctx->mac_addr, sizeof(ctx->mac_addr),
				       CONFIG_ETH_NATIVE_POSIX_MAC_ADDR) < 0) {
			LOG_ERR("Invalid MAC address %s",
				CONFIG_ETH_NATIVE_POSIX_MAC_ADDR);
		}
	}
#endif

	/* If we have only one network interface, then use the name
	 * defined in the Kconfig directly. This way there is no need to
	 * change the documentation etc. and break things.
	 */
	if (CONFIG_ETH_NATIVE_POSIX_INTERFACE_COUNT == 1) {
		ctx->if_name = ETH_NATIVE_POSIX_DRV_NAME;
	}

	LOG_DBG("Interface %p using \"%s\"", iface, log_strdup(ctx->if_name));

	net_if_set_link_addr(iface, ll_addr->addr, ll_addr->len,
			     NET_LINK_ETHERNET);

	ctx->dev_fd = eth_iface_create(ctx->if_name, false);
	if (ctx->dev_fd < 0) {
		LOG_ERR("Cannot create %s (%d)", ctx->if_name, -errno);
	} else {
		/* Create a thread that will handle incoming data from host */
		create_rx_handler(ctx);

		eth_setup_host(ctx->if_name);

		eth_start_script(ctx->if_name);
	}
}

static
enum ethernet_hw_caps eth_posix_native_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_TXTIME
#if defined(CONFIG_NET_VLAN)
		| ETHERNET_HW_VLAN
#endif
#if defined(CONFIG_ETH_NATIVE_POSIX_VLAN_TAG_STRIP)
		| ETHERNET_HW_VLAN_TAG_STRIP
#endif
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
static const struct device *eth_get_ptp_clock(const struct device *dev)
{
	struct eth_context *context = dev->data;

	return context->ptp_clock;
}
#endif

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *get_stats(const struct device *dev)
{
	struct eth_context *context = dev->data;

	return &(context->stats);
}
#endif

static int set_config(const struct device *dev,
		      enum ethernet_config_type type,
		      const struct ethernet_config *config)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE) &&
	    type == ETHERNET_CONFIG_TYPE_PROMISC_MODE) {
		struct eth_context *context = dev->data;

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
	} else if (type == ETHERNET_CONFIG_TYPE_MAC_ADDRESS) {
		struct eth_context *context = dev->data;

		memcpy(context->mac_addr, config->mac_address.addr,
		       sizeof(context->mac_addr));
	}

	return ret;
}

#if defined(CONFIG_NET_VLAN)
static int vlan_setup(const struct device *dev, struct net_if *iface,
		      uint16_t tag, bool enable)
{
	if (enable) {
		net_lldp_set_lldpdu(iface);
	} else {
		net_lldp_unset_lldpdu(iface);
	}

	return 0;
}
#endif /* CONFIG_NET_VLAN */

static int eth_start_device(const struct device *dev)
{
	struct eth_context *context = dev->data;
	int ret;

	context->status = true;

	ret = eth_if_up(context->if_name);

	eth_setup_host(context->if_name);

	return ret;
}

static int eth_stop_device(const struct device *dev)
{
	struct eth_context *context = dev->data;

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

#define DEFINE_ETH_DEV_DATA(x, _)					     \
	static struct eth_context eth_context_data_##x = {		     \
		.if_name = CONFIG_ETH_NATIVE_POSIX_DRV_NAME #x,		     \
		.rx_thread = &rx_thread_data_##x,			     \
		.rx_stack = rx_thread_stack_##x,			     \
		.rx_stack_size = K_KERNEL_STACK_SIZEOF(rx_thread_stack_##x), \
	}

LISTIFY(CONFIG_ETH_NATIVE_POSIX_INTERFACE_COUNT, DEFINE_ETH_DEV_DATA, (;), _);

#define DEFINE_ETH_DEVICE(x, _)						\
	ETH_NET_DEVICE_INIT(eth_native_posix_##x,			\
			    CONFIG_ETH_NATIVE_POSIX_DRV_NAME #x,	\
			    eth_init, NULL,				\
			    &eth_context_data_##x,			\
			    NULL,					\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			    &eth_if_api,				\
			    NET_ETH_MTU)

LISTIFY(CONFIG_ETH_NATIVE_POSIX_INTERFACE_COUNT, DEFINE_ETH_DEVICE, (;), _);

#if defined(CONFIG_ETH_NATIVE_POSIX_PTP_CLOCK)

#if IS_ENABLED(CONFIG_NET_GPTP)
BUILD_ASSERT(								\
	CONFIG_ETH_NATIVE_POSIX_INTERFACE_COUNT == CONFIG_NET_GPTP_NUM_PORTS, \
	"Number of network interfaces must match gPTP port count");
#endif

struct ptp_context {
	struct eth_context *eth_context;
};

#define DEFINE_PTP_DEV_DATA(x, _) \
	static struct ptp_context ptp_context_##x

LISTIFY(CONFIG_ETH_NATIVE_POSIX_INTERFACE_COUNT, DEFINE_PTP_DEV_DATA, (;), _);

static int ptp_clock_set_native_posix(const struct device *clk,
				      struct net_ptp_time *tm)
{
	ARG_UNUSED(clk);
	ARG_UNUSED(tm);

	/* We cannot set the host device time so this function
	 * does nothing.
	 */

	return 0;
}

static int ptp_clock_get_native_posix(const struct device *clk,
				      struct net_ptp_time *tm)
{
	ARG_UNUSED(clk);

	return eth_clock_gettime(tm);
}

static int ptp_clock_adjust_native_posix(const struct device *clk,
					 int increment)
{
	ARG_UNUSED(clk);
	ARG_UNUSED(increment);

	/* We cannot adjust the host device time so this function
	 * does nothing.
	 */

	return 0;
}

static int ptp_clock_rate_adjust_native_posix(const struct device *clk,
					      double ratio)
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

#define PTP_INIT_FUNC(x, _)						\
	static int ptp_init_##x(const struct device *port)			\
	{								\
		const struct device *eth_dev = DEVICE_GET(eth_native_posix_##x); \
		struct eth_context *context = eth_dev->data;	\
		struct ptp_context *ptp_context = port->data;	\
									\
		context->ptp_clock = port;				\
		ptp_context->eth_context = context;			\
									\
		return 0;						\
	}

LISTIFY(CONFIG_ETH_NATIVE_POSIX_INTERFACE_COUNT, PTP_INIT_FUNC, (), _)

#define DEFINE_PTP_DEVICE(x, _)						\
	DEVICE_DEFINE(eth_native_posix_ptp_clock_##x,			\
			    PTP_CLOCK_NAME "_" #x,			\
			    ptp_init_##x,				\
			    NULL,					\
			    &ptp_context_##x,				\
			    NULL,					\
			    POST_KERNEL,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			    &api)

LISTIFY(CONFIG_ETH_NATIVE_POSIX_INTERFACE_COUNT, DEFINE_PTP_DEVICE, (;), _);

#endif /* CONFIG_ETH_NATIVE_POSIX_PTP_CLOCK */
