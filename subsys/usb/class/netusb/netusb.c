/*
 * Ethernet over USB device
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_USB_DEVICE_NETWORK_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_net);

#include <init.h>

#include <net/ethernet.h>
#include <net_private.h>

#include <usb_device.h>
#include <usb_common.h>
#include <usb_descriptor.h>

#include "netusb.h"

static struct __netusb {
	struct net_if *iface;
	const struct netusb_function *func;
} netusb;

static int netusb_send(struct device *dev, struct net_pkt *pkt)
{
	int ret;

	ARG_UNUSED(dev);

	LOG_DBG("Send pkt, len %zu", net_pkt_get_len(pkt));

	if (!netusb_enabled()) {
		LOG_ERR("interface disabled");
		return -ENODEV;
	}

	ret = netusb.func->send_pkt(pkt);
	if (ret) {
		return ret;
	}

	return 0;
}

struct net_if *netusb_net_iface(void)
{
	return netusb.iface;
}

void netusb_recv(struct net_pkt *pkt)
{
	LOG_DBG("Recv pkt, len %zu", net_pkt_get_len(pkt));

	if (net_recv_data(netusb.iface, pkt) < 0) {
		LOG_ERR("Packet %p dropped by NET stack", pkt);
		net_pkt_unref(pkt);
	}
}

static int netusb_connect_media(void)
{
	LOG_DBG("");

	if (!netusb_enabled()) {
		LOG_ERR("interface disabled");
		return -ENODEV;
	}

	if (!netusb.func->connect_media) {
		return -ENOTSUP;
	}

	return netusb.func->connect_media(true);
}

static int netusb_disconnect_media(void)
{
	LOG_DBG("");

	if (!netusb_enabled()) {
		LOG_ERR("interface disabled");
		return -ENODEV;
	}

	if (!netusb.func->connect_media) {
		return -ENOTSUP;
	}

	return netusb.func->connect_media(false);
}

void netusb_enable(const struct netusb_function *func)
{
	LOG_DBG("");

	netusb.func = func;

	net_if_up(netusb.iface);
	netusb_connect_media();
}

void netusb_disable(void)
{
	LOG_DBG("");

	if (!netusb_enabled()) {
		return;
	}

	netusb.func = NULL;

	netusb_disconnect_media();
	net_if_down(netusb.iface);
}

bool netusb_enabled(void)
{
	return !!netusb.func;
}

static void netusb_init(struct net_if *iface)
{
	static uint8_t mac[6] = { 0x00, 0x00, 0x5E, 0x00, 0x53, 0x00 };

	LOG_DBG("netusb device initialization");

	netusb.iface = iface;

	ethernet_init(iface);
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);

	net_if_set_link_addr(iface, mac, sizeof(mac), NET_LINK_ETHERNET);

	LOG_INF("netusb initialized");
}

static const struct ethernet_api netusb_api_funcs = {
	.iface_api.init = netusb_init,

	.get_capabilities = NULL,
	.send = netusb_send,
};

static int netusb_init_dev(struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

NET_DEVICE_INIT(eth_netusb, "eth_netusb", netusb_init_dev,
		device_pm_control_nop, NULL, NULL,
		CONFIG_ETH_INIT_PRIORITY, &netusb_api_funcs, ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);
