/*
 * Ethernet over USB device
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL 3
#define SYS_LOG_DOMAIN "netusb"
#include <logging/sys_log.h>

#include <init.h>

#include <usb_device.h>
#include <usb_common.h>
#include <class/cdc_acm.h>

#include <net_private.h>

#include "../../usb_descriptor.h"
#include "../../composite.h"
#include "netusb.h"
#include "function_ecm.h"

static struct __netusb {
	struct net_if *iface;
	u8_t conf;
	struct netusb_function *func;
} netusb;

static int netusb_send(struct net_if *iface, struct net_pkt *pkt)
{
	SYS_LOG_DBG("Send pkt, len %u", net_pkt_get_len(pkt));

	switch (netusb.conf) {
	case 1:
		if (!ecm_send(pkt)) {
			net_pkt_unref(pkt);
			return 0;
		}
	default:
		SYS_LOG_ERR("Unconfigured device, conf %u", netusb.conf);
		break;
	}

	return -ENODEV;
}

void netusb_recv(struct net_pkt *pkt)
{
	SYS_LOG_DBG("Recv pkt, len %u", net_pkt_get_len(pkt));

	if (net_recv_data(netusb.iface, pkt)) {
		SYS_LOG_ERR("Queueing packet %p failed", pkt);
		net_pkt_unref(pkt);
	}
}

static struct usb_ep_cfg_data netusb_ep_data[] = {
	/* Configuration ECM */
	{
		.ep_cb = ecm_int_in,
		.ep_addr = CONFIG_CDC_ECM_INT_EP_ADDR
	},
	{
		.ep_cb = ecm_bulk_out,
		.ep_addr = CONFIG_CDC_ECM_OUT_EP_ADDR
	},
	{
		.ep_cb = ecm_bulk_in,
		.ep_addr = CONFIG_CDC_ECM_IN_EP_ADDR
	},
};

static int netusb_connect_media(void)
{
	if (!netusb.func->connect_media) {
		return -ENOTSUP;
	}

	return netusb.func->connect_media(true);
}

static int netusb_disconnect_media(void)
{
	if (!netusb.func->connect_media) {
		return -ENOTSUP;
	}

	return netusb.func->connect_media(false);
}

static void netusb_status_configured(u8_t *conf)
{
	SYS_LOG_INF("Enable configuration number %u", *conf);

	netusb.conf = *conf;

	switch (*conf) {
	case 1:
		net_if_up(netusb.iface);
		netusb_connect_media();
		break;
	case 0:
		netusb_disconnect_media();
		net_if_down(netusb.iface);
		break;
	default:
		SYS_LOG_ERR("Wrong configuration chosen: %u", *conf);
		netusb.conf = 0;
		break;
	}
}

static void netusb_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_ERROR:
		SYS_LOG_DBG("USB device error");
		break;
	case USB_DC_RESET:
		SYS_LOG_DBG("USB device reset detected");
		break;
	case USB_DC_CONNECTED:
		SYS_LOG_DBG("USB device connected");
		break;
	case USB_DC_CONFIGURED:
		SYS_LOG_DBG("USB device configured");
		netusb_status_configured(param);
		break;
	case USB_DC_DISCONNECTED:
		SYS_LOG_DBG("USB device disconnected");
		break;
	case USB_DC_SUSPEND:
		SYS_LOG_DBG("USB device suspended");
		break;
	case USB_DC_RESUME:
		SYS_LOG_DBG("USB device resumed");
		break;
	case USB_DC_UNKNOWN:
	default:
		SYS_LOG_DBG("USB unknown state");
		break;
		}
}

static int netusb_class_handler(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	SYS_LOG_DBG("len %d req_type 0x%x req 0x%x conf %u",
		    *len, setup->bmRequestType, setup->bRequest, netusb.conf);

	switch (netusb.conf) {
	case 1:
		return ecm_class_handler(setup, len, data);
	default:
		SYS_LOG_ERR("Unconfigured device, conf %u", netusb.conf);
		return -ENODEV;
	}
}

static int netusb_custom_handler(struct usb_setup_packet *setup,
				 s32_t *len, u8_t **data)
{
	/**
	 * FIXME:
	 * Keep custom handler for now in a case some data is sent
	 * over this endpoint, at the moment return to higher layer.
	 */
	return -ENOTSUP;
}

#if !defined(CONFIG_USB_COMPOSITE_DEVICE)
/* TODO: FIXME: correct buffer size */
static u8_t interface_data[300];
#endif

static struct usb_cfg_data netusb_config = {
	.usb_device_description = NULL,
	.cb_usb_status = netusb_status_cb,
	.interface = {
		.class_handler = netusb_class_handler,
		.custom_handler = netusb_custom_handler,
		.custom_handler = NULL,
		.vendor_handler = NULL,
		.payload_data = NULL,
	},
	.num_endpoints = ARRAY_SIZE(netusb_ep_data),
	.endpoint = netusb_ep_data,
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

	SYS_LOG_DBG("iface %p", iface);

	netusb.iface = iface;

	SYS_LOG_DBG("iface %p", iface);

	net_if_set_link_addr(iface, mac, sizeof(mac), NET_LINK_ETHERNET);

	net_if_down(iface);

#if defined(CONFIG_USB_DEVICE_NETWORK_ECM)
	netusb.func = ecm_register_function(iface, CONFIG_CDC_ECM_IN_EP_ADDR);
#endif

#if defined(CONFIG_USB_COMPOSITE_DEVICE)
#if defined(CONFIG_USB_DEVICE_NETWORK_ECM)
	ret = composite_add_function(&netusb_config, FIRST_IFACE_CDC_ECM);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to add CDC ECM function");
		return;
	}
#endif /* CONFIG_USB_DEVICE_NETWORK_ECM */
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

static struct net_if_api api_funcs = {
	.init	= netusb_init,
	.send	= netusb_send,
};

static int netusb_init_dev(struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

NET_DEVICE_INIT(eth_netusb, "eth_netusb", netusb_init_dev, NULL, NULL,
		CONFIG_ETH_INIT_PRIORITY, &api_funcs, ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), 1500);
