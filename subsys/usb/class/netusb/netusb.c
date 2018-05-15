/*
 * Ethernet over USB device
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DEVICE_NETWORK_DEBUG_LEVEL
#define SYS_LOG_DOMAIN "usb/net"
#include <logging/sys_log.h>

/* Enable verbose debug printing extra hexdumps */
#define VERBOSE_DEBUG	0

/* This enables basic hexdumps */
#define NET_LOG_ENABLED	0
#include <net_private.h>

#include <init.h>

#include <usb_device.h>
#include <usb_common.h>
#include <class/usb_cdc.h>
#include <net/ethernet.h>

#include <net/ethernet.h>

#include <usb_descriptor.h>
#include <composite.h>
#include "netusb.h"

static struct __netusb {
	struct net_if *iface;
	bool enabled;
	struct netusb_function *func;
} netusb;

static int netusb_send(struct net_if *iface, struct net_pkt *pkt)
{
	int ret;

	SYS_LOG_DBG("Send pkt, len %u", net_pkt_get_len(pkt));

	if (!netusb.enabled) {
		SYS_LOG_ERR("interface disabled");
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
	SYS_LOG_DBG("Recv pkt, len %u", net_pkt_get_len(pkt));

	if (net_recv_data(netusb.iface, pkt) < 0) {
		SYS_LOG_ERR("Packet %p dropped by NET stack", pkt);
		net_pkt_unref(pkt);
	}
}

static int netusb_connect_media(void)
{
	SYS_LOG_DBG("");

	if (!netusb.func->connect_media) {
		return -ENOTSUP;
	}

	return netusb.func->connect_media(true);
}

static int netusb_disconnect_media(void)
{
	SYS_LOG_DBG("");

	if (!netusb.func->connect_media) {
		return -ENOTSUP;
	}

	return netusb.func->connect_media(false);
}

void netusb_enable(void)
{
	SYS_LOG_DBG("");

	netusb.enabled = true;
	net_if_up(netusb.iface);
	netusb_connect_media();
}

void netusb_disable(void)
{
	SYS_LOG_DBG("");

	if (!netusb.enabled) {
		return;
	}

	netusb.enabled = false;
	netusb_disconnect_media();
	net_if_down(netusb.iface);
}

static int netusb_class_handler(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	SYS_LOG_DBG("len %d req_type 0x%x req 0x%x enabled %u",
		    *len, setup->bmRequestType, setup->bRequest,
		    netusb.enabled);

	if (!netusb.enabled) {
		SYS_LOG_ERR("interface disabled");
		return -ENODEV;
	}

	return netusb.func->class_handler(setup, len, data);
}

#if !defined(CONFIG_USB_COMPOSITE_DEVICE)
/* TODO: FIXME: correct buffer size */
static u8_t interface_data[300];
#endif

static struct usb_cfg_data netusb_config = {
	.usb_device_description = NULL,
	.cb_usb_status = NULL,
	.interface = {
		.class_handler = netusb_class_handler,
		.custom_handler = NULL,
		.vendor_handler = NULL,
		.payload_data = NULL,
	},
};

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
				SYS_LOG_WRN("Error: EAGAIN. Another try");
				continue;
			}

			return ret;
		case 0:
			/* Check wrote bytes */
			break;
		/* TODO: Handle other error codes */
		default:
			SYS_LOG_WRN("Error writing to ep 0x%x ret %d", ep, ret);
			return ret;
		}

		len -= wrote;
		data += wrote;
#if VERBOSE_DEBUG
		SYS_LOG_DBG("Wrote %u bytes, remaining %u", wrote, len);
#endif

		if (len) {
			SYS_LOG_WRN("Remaining bytes %d wrote %d", len, wrote);
		}
	}

	return ret;
}

static void netusb_init(struct net_if *iface)
{
	static u8_t mac[6] = { 0x00, 0x00, 0x5E, 0x00, 0x53, 0x00 };
	int ret;

	SYS_LOG_DBG("netusb device initialization");

	netusb.iface = iface;

	net_if_set_link_addr(iface, mac, sizeof(mac), NET_LINK_ETHERNET);

	net_if_down(iface);

/*
 * TODO: Add multi-function configuration
 */
#if defined(CONFIG_USB_DEVICE_NETWORK_ECM)
	netusb.func = &ecm_function;
#elif defined(CONFIG_USB_DEVICE_NETWORK_RNDIS)
	netusb.func = &rndis_function;
#elif defined(CONFIG_USB_DEVICE_NETWORK_EEM)
	netusb.func = &eem_function;
#else
#error Unknown USB Device Networking function
#endif
	if (netusb.func->init && netusb.func->init()) {
		SYS_LOG_ERR("Initialization failed");
		return;
	}

	netusb_config.endpoint = netusb.func->ep;
	netusb_config.num_endpoints = netusb.func->num_ep;
	netusb_config.cb_usb_status = netusb.func->status_cb;

#if defined(CONFIG_USB_COMPOSITE_DEVICE)
	ret = composite_add_function(&netusb_config, NETUSB_IFACE_IDX);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to add CDC ECM function");
		return;
	}
#else /* CONFIG_USB_COMPOSITE_DEVICE */
	netusb_config.interface.payload_data = interface_data;
	netusb_config.usb_device_description = usb_get_device_descriptor();

	ret = usb_set_config(&netusb_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to configure USB device");
		return;
	}

	ret = usb_enable(&netusb_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to enable USB");
		return;
	}
#endif /* CONFIG_USB_COMPOSITE_DEVICE */

	SYS_LOG_INF("netusb initialized");
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
