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

#if !defined(CONFIG_USB_COMPOSITE_DEVICE)
/* TODO: FIXME: correct buffer size */
static u8_t interface_data[300];
#endif

static int netusb_send(struct device *dev, struct net_pkt *pkt)
{
	int ret;

	ARG_UNUSED(dev);

	LOG_DBG("Send pkt, len %u", net_pkt_get_len(pkt));

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
	LOG_DBG("Recv pkt, len %u", net_pkt_get_len(pkt));

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
	static u8_t mac[6] = { 0x00, 0x00, 0x5E, 0x00, 0x53, 0x00 };

	LOG_DBG("netusb device initialization");

	netusb.iface = iface;

	ethernet_init(iface);

	net_if_set_link_addr(iface, mac, sizeof(mac), NET_LINK_ETHERNET);

	net_if_down(iface);

#ifndef CONFIG_USB_COMPOSITE_DEVICE
	/* Linker-defined symbols bound the USB descriptor structs */
	extern struct usb_cfg_data __usb_data_start[];
	extern struct usb_cfg_data __usb_data_end[];
	size_t size = (__usb_data_end - __usb_data_start);

	for (size_t i = 0; i < size; i++) {
		struct usb_cfg_data *cfg = &(__usb_data_start[i]);
		int ret;

		LOG_DBG("Registering function %u", i);

		cfg->interface.payload_data = interface_data;
		cfg->usb_device_description = usb_get_device_descriptor();

		ret = usb_set_config(cfg);
		if (ret < 0) {
			LOG_ERR("Failed to configure USB device");
			return;
		}

		ret = usb_enable(cfg);
		if (ret < 0) {
			LOG_ERR("Failed to enable USB");
			return;
		}
	}
#endif /* CONFIG_USB_COMPOSITE_DEVICE */

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

NET_DEVICE_INIT(eth_netusb, "eth_netusb", netusb_init_dev, NULL, NULL,
		CONFIG_ETH_INIT_PRIORITY, &netusb_api_funcs, ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);
