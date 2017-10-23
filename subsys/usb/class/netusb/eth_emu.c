/* Ethernet Emulation driver
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ETHERNET_LEVEL
#define SYS_LOG_DOMAIN "dev/eth_emu"
#include <logging/sys_log.h>

#include <zephyr.h>
#include <net/net_pkt.h>
#include <net/net_if.h>

#include "netusb.h"
#include "eth_emu.h"

static struct eth_emu_context eth_emu_ctx;

static int eth_emu_init(struct device *dev)
{
	SYS_LOG_DBG("ctx %p", dev->driver_data);

	return 0;
}

static void eth_emu_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct eth_emu_context *ctx = dev->driver_data;
	/*
	 * Use RFC7042 Documentation values for MAC address
	 */
	u8_t mac[6] = { 0x00, 0x00, 0x5E, 0x00, 0x53, 0x00 };

	/*
	 * TODO: Implement parsing Descriptor table
	 */
	memcpy(ctx->mac_addr, mac, sizeof(mac));
	ctx->iface = iface;

	SYS_LOG_DBG("ctx %p", ctx);

	net_if_set_link_addr(iface, ctx->mac_addr,
			     sizeof(ctx->mac_addr),
			     NET_LINK_ETHERNET);
}

static int eth_emu_tx(struct net_if *iface, struct net_pkt *pkt)
{
	int ret = 0;

	SYS_LOG_DBG("pkt %p", pkt);

	ret = netusb_send(pkt);
	if (ret) {
		SYS_LOG_ERR("Error sending packet %p", pkt);
	} else {
		net_pkt_unref(pkt);
	}

	return ret;
}

int eth_emu_up(void)
{
	return net_if_up(eth_emu_ctx.iface);
}

int eth_emu_down(void)
{
	return net_if_down(eth_emu_ctx.iface);
}

static struct net_if_api api_funcs = {
	.init	= eth_emu_iface_init,
	.send	= eth_emu_tx,
};

NET_DEVICE_INIT(eth_emu, CONFIG_ETH_EMU_0_NAME,
		eth_emu_init, &eth_emu_ctx,
		NULL, CONFIG_ETH_INIT_PRIORITY, &api_funcs,
		ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2), 1500);
