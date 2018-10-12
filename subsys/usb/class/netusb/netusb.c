/*
 * Ethernet over USB device
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_USB_DEVICE_NETWORK_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_net)

/* Enable verbose debug printing extra hexdumps */
#define VERBOSE_DEBUG	0

/* This enables basic hexdumps */
#define NET_LOG_ENABLED	0
#include <net_private.h>

#include <init.h>

#include <usb_device.h>
#include <usb_common.h>
#include <net/ethernet.h>

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

static int netusb_send(struct net_if *iface, struct net_pkt *pkt)
{
	int ret;

	USB_DBG("Send pkt, len %u", net_pkt_get_len(pkt));

	if (!netusb_enabled()) {
		USB_ERR("interface disabled");
		return -ENODEV;
	}

	ret = netusb.func->send_pkt(pkt);
	if (ret) {
		return ret;
	}

	net_pkt_unref(pkt);
	return 0;
}

void netusb_recv(struct net_pkt *pkt)
{
	USB_DBG("Recv pkt, len %u", net_pkt_get_len(pkt));

	if (net_recv_data(netusb.iface, pkt) < 0) {
		USB_ERR("Packet %p dropped by NET stack", pkt);
		net_pkt_unref(pkt);
	}
}

static int netusb_connect_media(void)
{
	USB_DBG("");

	if (!netusb_enabled()) {
		USB_ERR("interface disabled");
		return -ENODEV;
	}

	if (!netusb.func->connect_media) {
		return -ENOTSUP;
	}

	return netusb.func->connect_media(true);
}

static int netusb_disconnect_media(void)
{
	USB_DBG("");

	if (!netusb_enabled()) {
		USB_ERR("interface disabled");
		return -ENODEV;
	}

	if (!netusb.func->connect_media) {
		return -ENOTSUP;
	}

	return netusb.func->connect_media(false);
}

void netusb_enable(const struct netusb_function *func)
{
	USB_DBG("");

	netusb.func = func;

	net_if_up(netusb.iface);
	netusb_connect_media();
}

void netusb_disable(void)
{
	USB_DBG("");

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

int try_write(u8_t ep, u8_t *data, u16_t len)
{
	u8_t tries = 10;
	int ret = 0;

	net_hexdump("USB <", data, len);

	while (len) {
		u32_t wrote;

		ret = usb_write(ep, data, len, &wrote);

		switch (ret) {
		case -EAGAIN:
			/*
			 * In a case when host has not yet enabled endpoint
			 * to get this message we might get No Space Available
			 * error from the controller, try only several times.
			 */
			if (tries--) {
				USB_WRN("Error: EAGAIN. Another try");
				continue;
			}

			return ret;
		case 0:
			/* Check wrote bytes */
			break;
		/* TODO: Handle other error codes */
		default:
			USB_WRN("Error writing to ep 0x%x ret %d", ep, ret);
			return ret;
		}

		len -= wrote;
		data += wrote;
#if VERBOSE_DEBUG
		USB_DBG("Wrote %u bytes, remaining %u", wrote, len);
#endif

		if (len) {
			USB_WRN("Remaining bytes %d wrote %d", len, wrote);
		}
	}

	return ret;
}

static void netusb_init(struct net_if *iface)
{
	static u8_t mac[6] = { 0x00, 0x00, 0x5E, 0x00, 0x53, 0x00 };

	USB_DBG("netusb device initialization");

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

		USB_DBG("Registering function %u", i);

		cfg->interface.payload_data = interface_data;
		cfg->usb_device_description = usb_get_device_descriptor();

		ret = usb_set_config(cfg);
		if (ret < 0) {
			USB_ERR("Failed to configure USB device");
			return;
		}

		ret = usb_enable(cfg);
		if (ret < 0) {
			USB_ERR("Failed to enable USB");
			return;
		}
	}
#endif /* CONFIG_USB_COMPOSITE_DEVICE */

	USB_INF("netusb initialized");
}

static const struct ethernet_api netusb_api_funcs = {
	.iface_api = {
		.init = netusb_init,
		.send = netusb_send,
	},
	.get_capabilities = NULL,
};

static int netusb_init_dev(struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

NET_DEVICE_INIT(eth_netusb, "eth_netusb", netusb_init_dev, NULL, NULL,
		CONFIG_ETH_INIT_PRIORITY, &netusb_api_funcs, ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), NETUSB_MTU);
