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
#include "eth_emu.h"
#include "function_ecm.h"

#define NETUSB_NUM_CONF 1

static struct __netusb {
	u8_t conf;
	u8_t conf_index;
	struct netusb_function *func[NETUSB_NUM_CONF];
} netusb;

/* Ethernet Emulation device */
static struct eth_emu_context *eth_emu_ctx;

int netusb_send(struct net_pkt *pkt)
{
	SYS_LOG_DBG("Send pkt, len %u", net_pkt_get_len(pkt));

	switch (netusb.conf) {
	case 1:
		return ecm_send(pkt);
	default:
		SYS_LOG_ERR("Unconfigured device, conf %u", netusb.conf);
		break;
	}

	return -ENODEV;
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
	if (!netusb.func[netusb.conf_index]->connect_media) {
		return -ENOTSUP;
	}

	return netusb.func[netusb.conf_index]->connect_media(true);
}

static int netusb_disconnect_media(void)
{
	if (!netusb.func[netusb.conf_index]->connect_media) {
		return -ENOTSUP;
	}

	return netusb.func[netusb.conf_index]->connect_media(false);
}

static void netusb_status_configured(u8_t *conf)
{
	SYS_LOG_INF("Enable configuration number %u", *conf);

	netusb.conf = *conf;

	switch (*conf) {
	case 1:
		netusb.conf_index = 0;
		eth_emu_up();
		netusb_connect_media();
		break;
	case 0:
		netusb_disconnect_media();
		eth_emu_down();
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

static int netusb_device_init(struct device *dev)
{
	struct device *eth_dev;
	struct net_if *iface;
	int ret;

	ARG_UNUSED(dev);

	SYS_LOG_DBG("netusb device initialization");

	eth_dev = device_get_binding(CONFIG_ETH_EMU_0_NAME);
	if (!eth_dev) {
		SYS_LOG_ERR("Cannot get Ethernet Emulation device");
		return -ENODEV;
	}

	eth_emu_ctx = (struct eth_emu_context *)eth_dev->driver_data;

	iface = eth_emu_ctx->iface;

	SYS_LOG_DBG("iface %p", iface);

	net_if_down(iface);

#if defined(CONFIG_USB_DEVICE_NETWORK_ECM)
	netusb.func[0] = ecm_register_function(iface,
					       CONFIG_CDC_ECM_IN_EP_ADDR);
#endif

#if defined(CONFIG_USB_COMPOSITE_DEVICE)
#if defined(CONFIG_USB_DEVICE_NETWORK_ECM)
	ret = composite_add_function(&netusb_config, FIRST_IFACE_CDC_ECM);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to add CDC ECM function");
		return ret;
	}
#endif /* CONFIG_USB_DEVICE_NETWORK_ECM */
#else /* CONFIG_USB_COMPOSITE_DEVICE */
	netusb_config.interface.payload_data = interface_data;
	netusb_config.usb_device_description = usb_get_device_descriptor();

	ret = usb_set_config(&netusb_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to configure USB device");
		return ret;
	}

	ret = usb_enable(&netusb_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to enable USB");
		return ret;
	}
#endif /* CONFIG_USB_COMPOSITE_DEVICE */

	SYS_LOG_INF("netusb initialized");

	return ret;
}

/*
 * Initialization priority should be bigger then CONFIG_ETH_INIT_PRIORITY
 * in order to be able to get eth_dev
 */
SYS_INIT(netusb_device_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
