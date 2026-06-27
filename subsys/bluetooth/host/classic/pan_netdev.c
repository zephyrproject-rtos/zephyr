/* pan_netdev.c - Bluetooth PAN network interface */

/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/bluetooth/classic/pan.h>

#include "pan_internal.h"

#define LOG_LEVEL CONFIG_BT_PAN_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bt_pan);

void bt_pan_net_rx(struct bt_pan *pan, struct net_buf *buf)
{
	struct net_if *iface = pan->iface;
	struct net_pkt *pkt;

	if (iface == NULL) {
		return;
	}

	pkt = net_pkt_rx_alloc_with_buffer(iface, buf->len, AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		LOG_WRN("Failed to allocate RX packet");
		return;
	}

	if (net_pkt_write(pkt, buf->data, buf->len) != 0) {
		net_pkt_unref(pkt);
		return;
	}

	if (net_recv_data(iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}
}

static int pan_net_send(const struct device *dev, struct net_pkt *pkt)
{
	struct bt_pan_net_context *ctx = dev->data;
	struct net_buf *buf;
	size_t len;

	/* Packet ownership stays with Ethernet L2 / net_if; do not unref here. */
	if (ctx->pan == NULL || bt_pan_get_state(ctx->pan) != BT_PAN_STATE_CONNECTED) {
		return -ENETDOWN;
	}

	len = net_pkt_get_len(pkt);
	buf = bt_pan_alloc_buf(len);
	if (buf == NULL) {
		return -ENOMEM;
	}

	if (net_pkt_read(pkt, buf->data, len) != 0) {
		net_buf_unref(buf);
		return -EIO;
	}

	net_buf_add(buf, len);

	if (bt_pan_send(ctx->pan, buf) != 0) {
		net_buf_unref(buf);
		return -EIO;
	}

	return 0;
}

static void pan_net_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct bt_pan_net_context *ctx = dev->data;

	net_if_set_link_addr(iface, ctx->mac_addr, sizeof(ctx->mac_addr), NET_LINK_ETHERNET);
	ethernet_init(iface);
}

static enum ethernet_hw_caps pan_net_caps(const struct device *dev, struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return 0;
}

int bt_pan_net_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

const struct ethernet_api bt_pan_net_api = {
	.iface_api.init = pan_net_iface_init,
	.get_capabilities = pan_net_caps,
	.send = pan_net_send,
};

struct net_if *bt_pan_net_get_iface(const struct device *dev)
{
	return net_if_lookup_by_dev(dev);
}

void bt_pan_net_attach(struct bt_pan *pan, struct net_if *iface)
{
	const struct device *dev;

	if (pan == NULL || iface == NULL) {
		return;
	}

	pan->iface = iface;

	dev = net_if_get_device(iface);
	if (dev != NULL) {
		struct bt_pan_net_context *ctx = dev->data;

		ctx->pan = pan;
	}

	if (bt_pan_get_state(pan) == BT_PAN_STATE_CONNECTED) {
		net_if_carrier_on(iface);
	}
}

void bt_pan_net_detach(struct bt_pan *pan)
{
	const struct device *dev;

	if (pan == NULL) {
		return;
	}

	if (pan->iface != NULL) {
		net_if_carrier_off(pan->iface);
		dev = net_if_get_device(pan->iface);
		if (dev != NULL) {
			struct bt_pan_net_context *ctx = dev->data;

			ctx->pan = NULL;
		}
	}

	pan->iface = NULL;
}
