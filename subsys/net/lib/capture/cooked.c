/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_cooked, CONFIG_NET_CAPTURE_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/virtual_mgmt.h>
#include <zephyr/net/capture.h>
#include <zephyr/net/net_l2.h>

#include "sll.h"

#define BUF_ALLOC_TIMEOUT 100 /* ms */

/* Use our own slabs for temporary pkts */
NET_PKT_SLAB_DEFINE(cooked_pkts, CONFIG_NET_CAPTURE_PKT_COUNT);

#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)
NET_BUF_POOL_FIXED_DEFINE(cooked_bufs, CONFIG_NET_CAPTURE_BUF_COUNT,
			  CONFIG_NET_BUF_DATA_SIZE, 4, NULL);
#else
NET_BUF_POOL_VAR_DEFINE(cooked_bufs, CONFIG_NET_CAPTURE_BUF_COUNT,
			CONFIG_NET_BUF_DATA_POOL_SIZE, 4, NULL);
#endif

#define COOKED_MTU 1024
#define COOKED_DEVICE "NET_COOKED"

struct cooked_context {
	struct net_if *iface;
	struct net_if *attached_to;

	/* -1 is used as a not configured link type */
	int link_types[CONFIG_NET_CAPTURE_COOKED_MODE_MAX_LINK_TYPES];
	int link_type_count;
	int mtu;
	bool init_done;
	bool status;
};

static void iface_init(struct net_if *iface)
{
	struct cooked_context *ctx = net_if_get_device(iface)->data;
	struct net_if *any_iface;
	int ifindex;
	int ret;

	ifindex = net_if_get_by_name("any");
	if (ifindex < 0) {
		NET_DBG("No such interface \"any\", cannot init interface %d",
			net_if_get_by_iface(iface));
		return;
	}

	if (net_if_l2(net_if_get_by_index(ifindex)) != &NET_L2_GET_NAME(DUMMY)) {
		NET_DBG("The \"any\" interface %d is wrong type", ifindex);
		return;
	}

	if (ctx->init_done) {
		return;
	}

	ctx->iface = iface;
	any_iface = net_if_get_by_index(ifindex);

	(void)net_if_set_name(iface,
			      CONFIG_NET_CAPTURE_COOKED_MODE_INTERFACE_NAME);
	(void)net_virtual_set_name(iface, "Cooked mode capture");

	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_flag_set(iface, NET_IF_POINTOPOINT);
	net_if_flag_clear(iface, NET_IF_IPV4);
	net_if_flag_clear(iface, NET_IF_IPV6);

	/* Hook into "any" interface so that we can receive the
	 * captured data.
	 */
	ret = net_virtual_interface_attach(ctx->iface, any_iface);
	if (ret < 0) {
		NET_DBG("Cannot hook into interface %d (%d)",
			net_if_get_by_iface(any_iface), ret);
		return;
	}

	NET_DBG("Interface %d attached on top of %d",
		net_if_get_by_iface(ctx->iface),
		net_if_get_by_iface(any_iface));

	ctx->init_done = true;
}

static int dev_init(const struct device *dev)
{
	struct cooked_context *ctx = dev->data;

	memset(ctx->link_types, -1, sizeof(ctx->link_types));

	return 0;
}

static int interface_start(const struct device *dev)
{
	struct cooked_context *ctx = dev->data;
	int ret = 0;

	if (ctx->status) {
		return -EALREADY;
	}

	ctx->status = true;

	NET_DBG("Starting iface %d", net_if_get_by_iface(ctx->iface));

	return ret;
}

static int interface_stop(const struct device *dev)
{
	struct cooked_context *ctx = dev->data;

	if (!ctx->status) {
		return -EALREADY;
	}

	ctx->status = false;

	NET_DBG("Stopping iface %d", net_if_get_by_iface(ctx->iface));

	return 0;
}

static enum net_verdict interface_recv(struct net_if *iface, struct net_pkt *pkt)
{
	struct cooked_context *ctx = net_if_get_device(iface)->data;
	bool found = false;
	uint16_t ptype;

	/* Feed the packet to capture system after verifying that we are capturing
	 * these types of packets.
	 * The packet will be freed by capture API after it has been processed.
	 */

	ptype = net_pkt_ll_proto_type(pkt);

	NET_DBG("Capture pkt %p for interface %d", pkt, net_if_get_by_iface(iface));

	for (int i = 0; i < ctx->link_type_count; i++) {
		if (ctx->link_types[i] == ptype) {
			found = true;
			break;
		}
	}

	if (found) {
		int ret;

		NET_DBG("Handler found for packet type 0x%04x", ptype);

		/* Normally capture API will clone the net_pkt and we would
		 * always need to unref it. But for cooked packets, we can avoid
		 * the cloning so need to unref only if there was an error with
		 * capturing.
		 */
		ret = net_capture_pkt_with_status(iface, pkt);
		if (ret < 0) {
			net_pkt_unref(pkt);
		}

		return NET_OK;
	}

	NET_DBG("No handler found for packet type 0x%04x", ptype);

	return NET_DROP;
}

static int interface_attach(struct net_if *iface, struct net_if *lower_iface)
{
	struct cooked_context *ctx;

	if (net_if_get_by_iface(iface) < 0) {
		return -ENOENT;
	}

	ctx = net_if_get_device(iface)->data;
	ctx->attached_to = lower_iface;

	return 0;
}

static int interface_set_config(struct net_if *iface,
				enum virtual_interface_config_type type,
				const struct virtual_interface_config *config)
{
	struct cooked_context *ctx = net_if_get_device(iface)->data;

	switch (type) {
	case VIRTUAL_INTERFACE_CONFIG_TYPE_LINK_TYPE:
		if (config->link_types.count > ARRAY_SIZE(ctx->link_types)) {
			return -ERANGE;
		}

		for (int i = 0; i < config->link_types.count; i++) {
			NET_DBG("Adding link type %u", config->link_types.type[i]);

			ctx->link_types[i] = (int)config->link_types.type[i];
		}

		ctx->link_type_count = config->link_types.count;

		/* Mark the rest of the types as invalid */
		for (int i = ctx->link_type_count; i < ARRAY_SIZE(ctx->link_types); i++) {
			ctx->link_types[i] = -1;
		}

		return 0;

	case VIRTUAL_INTERFACE_CONFIG_TYPE_MTU:
		NET_DBG("Interface %d MTU set to %d",
			net_if_get_by_iface(iface), config->mtu);
		net_if_set_mtu(iface, config->mtu);
		return 0;

	default:
		break;
	}

	return -ENOTSUP;
}

static int interface_get_config(struct net_if *iface,
				enum virtual_interface_config_type type,
				struct virtual_interface_config *config)
{
	struct cooked_context *ctx = net_if_get_device(iface)->data;
	int i;

	switch (type) {
	case VIRTUAL_INTERFACE_CONFIG_TYPE_LINK_TYPE:
		for (i = 0; i < ctx->link_type_count; i++) {
			if (ctx->link_types[i] < 0) {
				break;
			}

			config->link_types.type[i] = (uint16_t)ctx->link_types[i];
		}

		config->link_types.count = i;
		NET_ASSERT(config->link_types.count == ctx->link_type_count);
		return 0;

	case VIRTUAL_INTERFACE_CONFIG_TYPE_MTU:
		config->mtu = net_if_get_mtu(iface);
		return 0;

	default:
		break;
	}

	return -ENOTSUP;
}

static const struct virtual_interface_api cooked_api = {
	.iface_api.init = iface_init,

	.start = interface_start,
	.stop = interface_stop,
	.recv = interface_recv,
	.attach = interface_attach,
	.set_config = interface_set_config,
	.get_config = interface_get_config,
};

static struct cooked_context cooked_context_data;

NET_VIRTUAL_INTERFACE_INIT(cooked, COOKED_DEVICE, dev_init,
			   NULL, &cooked_context_data, NULL,
			   CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			   &cooked_api, COOKED_MTU);

int net_capture_cooked_setup(struct net_capture_cooked *ctx,
			     uint16_t hatype,
			     uint16_t halen,
			     uint8_t *addr)
{
	if (halen == 0 || halen > NET_CAPTURE_LL_ADDRLEN) {
		return -EINVAL;
	}

	memset(ctx, 0, sizeof(*ctx));

	ctx->hatype = hatype;
	ctx->halen = halen;

	memcpy(ctx->addr, addr, halen);

	return 0;
}

static int create_sll_header(struct net_if *iface,
			     struct net_pkt *pkt,
			     struct net_capture_cooked *ctx,
			     enum net_capture_packet_type type,
			     uint16_t ptype)
{
	struct sll2_header hdr2;
	struct sll_header hdr1;
	size_t hdr_len;
	uint8_t *hdr;
	int ret;

	if (IS_ENABLED(CONFIG_NET_CAPTURE_COOKED_MODE_SLLV1)) {
		hdr1.sll_pkttype = htons(type);
		hdr1.sll_hatype = htons(ctx->hatype);
		hdr1.sll_halen = htons(ctx->halen);
		memcpy(hdr1.sll_addr, ctx->addr, sizeof(ctx->addr));
		hdr1.sll_protocol = htons(ptype);

		hdr = (uint8_t *)&hdr1;
		hdr_len = sizeof(hdr1);

	} else {
		hdr2.sll2_protocol = htons(ptype);
		hdr2.sll2_reserved_mbz = 0;
		hdr2.sll2_if_index = net_if_get_by_iface(iface);
		hdr2.sll2_hatype = htons(ctx->hatype);
		hdr2.sll2_pkttype = htons(type);
		hdr2.sll2_halen = htons(ctx->halen);
		memcpy(hdr2.sll2_addr, ctx->addr, sizeof(ctx->addr));

		hdr = (uint8_t *)&hdr2;
		hdr_len = sizeof(hdr2);
	}

	ret = net_pkt_write(pkt, hdr, hdr_len);
	if (ret < 0) {
		NET_DBG("Cannot write sll%s header (%d)",
			IS_ENABLED(CONFIG_NET_CAPTURE_COOKED_MODE_SLLV1) ?
			"" : "2", ret);
	}

	return ret;
}

static struct k_mem_slab *get_net_pkt(void)
{
	return &cooked_pkts;
}

static struct net_buf_pool *get_net_buf(void)
{
	return &cooked_bufs;
}

void net_capture_data(struct net_capture_cooked *ctx,
		      const uint8_t *data, size_t data_len,
		      enum net_capture_packet_type type,
		      uint16_t ptype)
{
	static struct net_context context;
	const struct device *dev;
	struct net_if *iface;
	struct net_pkt *pkt;
	int ret;

	net_context_setup_pools(&context, get_net_pkt, get_net_buf);

	pkt = net_pkt_alloc_from_slab(&cooked_pkts, K_MSEC(BUF_ALLOC_TIMEOUT));
	if (pkt == NULL) {
		NET_DBG("Cannot allocate %s", "net_pkt");
		return;
	}

	net_pkt_set_context(pkt, &context);

	ret = net_pkt_alloc_buffer_raw(pkt,
			sizeof(COND_CODE_1(CONFIG_NET_CAPTURE_COOKED_MODE_SLLV1,
					   (struct sll_header),
					   (struct sll2_header))) + data_len,
				       K_MSEC(BUF_ALLOC_TIMEOUT));
	if (ret < 0) {
		NET_DBG("Cannot allocate %s %zd bytes (%d)", "net_buf for",
			sizeof(struct sll_header) + data_len, ret);
		net_pkt_unref(pkt);
		return;
	}

	/* Write the packet to "any" interface which will pass it to
	 * the virtual interface that does the actual capturing of the packet.
	 * The reason for this trickery is that we do not have a network
	 * interface in use in this API, and want to have the packet routed
	 * via the any interface which can then deliver it to registered
	 * virtual interfaces.
	 */
	dev = device_get_binding(COOKED_DEVICE);
	if (dev == NULL) {
		NET_DBG("No such device %s found, data not captured!",
			COOKED_DEVICE);
		net_pkt_unref(pkt);
		return;
	}

	/* Next we feed the data to the any interface, which
	 * will pass the data to the virtual interface.
	 * When using capture API or net-shell capture command, one
	 * should capture packets from the virtual interface.
	 */
	iface = ((struct cooked_context *)dev->data)->attached_to;

	ret = create_sll_header(iface, pkt, ctx, type, ptype);
	if (ret < 0) {
		NET_DBG("Cannot write %s %zd bytes (%d)", "header",
			sizeof(struct sll_header), ret);
		net_pkt_unref(pkt);
		return;
	}

	ret = net_pkt_write(pkt, data, data_len);
	if (ret < 0) {
		NET_DBG("Cannot write %s %zd bytes (%d)", "payload",
			data_len, ret);
		net_pkt_unref(pkt);
		return;
	}

	/* Mark that this packet came from cooked capture mode.
	 * This will prevent the capture API from cloning the packet,
	 * so that the net_pkt will be passed as is to capture interface.
	 */
	net_pkt_set_cooked_mode(pkt, true);

	/* The protocol type is used by virtual cooked interface to decide
	 * whether we capture the packet or not.
	 */
	net_pkt_set_ll_proto_type(pkt, ptype);

	net_pkt_lladdr_src(pkt)->addr = NULL;
	net_pkt_lladdr_src(pkt)->len = 0U;
	net_pkt_lladdr_src(pkt)->type = NET_LINK_DUMMY;
	net_pkt_lladdr_dst(pkt)->addr = NULL;
	net_pkt_lladdr_dst(pkt)->len = 0U;
	net_pkt_lladdr_dst(pkt)->type = NET_LINK_DUMMY;

	ret = net_recv_data(iface, pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}
}
