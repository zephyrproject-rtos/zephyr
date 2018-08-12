/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define SYS_LOG_DOMAIN "wpanusb"
#include <logging/sys_log.h>

/* Enable net_hexdump() here */
#define NET_LOG_ENABLED		1
#include <net_private.h>

#include <device.h>
#include <shell/shell.h>

#include <usb/usb_device.h>
#include <usb/usb_common.h>

#include <net/buf.h>
#include <net/ieee802154_radio.h>

#include "wpanusb.h"

#define WPANUSB_SUBCLASS	0
#define WPANUSB_PROTOCOL	0

/* Max packet size for endpoints */
#define WPANUSB_BULK_EP_MPS		64

/* Max Bluetooth command data size */
#define WPANUSB_CLASS_MAX_DATA_SIZE	100

#define WPANUSB_ENDP_BULK_IN		0x81

static struct device *wpanusb_dev;

static struct ieee802154_radio_api *radio_api;
static struct device *ieee802154_dev;

static struct k_fifo tx_queue;

/**
 * Stack for the tx thread.
 */
static K_THREAD_STACK_DEFINE(tx_stack, 1024);
static struct k_thread tx_thread_data;

#define DEV_DATA(dev) \
	((struct wpanusb_dev_data_t * const)(dev)->driver_data)

/* Device data structure */
struct wpanusb_dev_data_t {
	/* USB device status code */
	enum usb_dc_status_code usb_status;
	u8_t interface_data[WPANUSB_CLASS_MAX_DATA_SIZE];
};

static const struct dev_common_descriptor {
	struct usb_device_descriptor device_descriptor;
	struct usb_cfg_descriptor configuration_descr;
	struct usb_device_config {
		struct usb_if_descriptor if0;
		struct usb_ep_descriptor if0_in_ep;
	} __packed device_configuration;
	/*
	 * String descriptors not enabled at the moment
	 */
} __packed wpanusb_desc = {
	/* Device descriptor */
	.device_descriptor = {
		.bLength = sizeof(struct usb_device_descriptor),
		.bDescriptorType = USB_DEVICE_DESC,
		.bcdUSB = sys_cpu_to_le16(USB_1_1),
		.bDeviceClass = CUSTOM_CLASS,
		.bDeviceSubClass = 0,
		.bDeviceProtocol = 0,
		.bMaxPacketSize0 = MAX_PACKET_SIZE0,
		.idVendor = sys_cpu_to_le16((u16_t)CONFIG_USB_DEVICE_VID),
		.idProduct = sys_cpu_to_le16((u16_t)CONFIG_USB_DEVICE_PID),
		.bcdDevice = sys_cpu_to_le16(BCDDEVICE_RELNUM),
		.iManufacturer = 0,
		.iProduct = 0,
		.iSerialNumber = 0,
		.bNumConfigurations = 1,
	},

	/* Configuration descriptor */
	.configuration_descr = {
		.bLength = sizeof(struct usb_cfg_descriptor),
		.bDescriptorType = USB_CONFIGURATION_DESC,
		.wTotalLength = sizeof(struct dev_common_descriptor)
			      - sizeof(struct usb_device_descriptor),
		.bNumInterfaces = 1,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_CONFIGURATION_ATTRIBUTES,
		.bMaxPower = MAX_LOW_POWER,
	},

	/* Device configuration */
	.device_configuration = {
		/* Interface descriptor */
		.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 0,
			.bNumEndpoints = 1,
			.bInterfaceClass = CUSTOM_CLASS,
			.bInterfaceSubClass = WPANUSB_SUBCLASS,
			.bInterfaceProtocol = WPANUSB_PROTOCOL,
			.iInterface = 0,
		},

		/* Endpoint IN */
		.if0_in_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = WPANUSB_ENDP_BULK_IN,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize = sys_cpu_to_le16(WPANUSB_BULK_EP_MPS),
			.bInterval = 0x00,
		},
	},
};

/* EP Bulk IN handler, used to send data to the Host */
static void wpanusb_bulk_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
}

/* Describe EndPoints configuration */
static struct usb_ep_cfg_data wpanusb_ep[] = {
	{
		.ep_cb = wpanusb_bulk_in,
		.ep_addr = WPANUSB_ENDP_BULK_IN
	},
};

static void wpanusb_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	struct wpanusb_dev_data_t * const dev_data = DEV_DATA(wpanusb_dev);

	ARG_UNUSED(param);

	/* Store the new status */
	dev_data->usb_status = status;

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

static int try_write(u8_t ep, u8_t *data, u16_t len)
{
	while (1) {
		int ret = usb_write(ep, data, len, NULL);

		switch (ret) {
		case -EAGAIN:
			break;
		/* TODO: Handle other error codes */
		default:
			return ret;
		}
	}
}

/* Decode wpanusb commands */

static int set_channel(void *data, int len)
{
	struct set_channel *req = data;

	SYS_LOG_DBG("page %u channel %u", req->page, req->channel);

	return radio_api->set_channel(ieee802154_dev, req->channel);
}

static int set_ieee_addr(void *data, int len)
{
	struct set_ieee_addr *req = data;

	SYS_LOG_DBG("len %u", len);

	if (IEEE802154_HW_FILTER &
	    radio_api->get_capabilities(ieee802154_dev)) {
		struct ieee802154_filter filter;

		filter.ieee_addr = (u8_t *)&req->ieee_addr;

		return radio_api->filter(ieee802154_dev, true,
					 IEEE802154_FILTER_TYPE_IEEE_ADDR,
					 &filter);
	}

	return 0;
}

static int set_short_addr(void *data, int len)
{
	struct set_short_addr *req = data;

	SYS_LOG_DBG("len %u", len);


	if (IEEE802154_HW_FILTER &
	    radio_api->get_capabilities(ieee802154_dev)) {
		struct ieee802154_filter filter;

		filter.short_addr = req->short_addr;

		return radio_api->filter(ieee802154_dev, true,
					 IEEE802154_FILTER_TYPE_SHORT_ADDR,
					 &filter);
	}

	return 0;
}

static int set_pan_id(void *data, int len)
{
	struct set_pan_id *req = data;

	SYS_LOG_DBG("len %u", len);

	if (IEEE802154_HW_FILTER &
	    radio_api->get_capabilities(ieee802154_dev)) {
		struct ieee802154_filter filter;

		filter.pan_id = req->pan_id;

		return radio_api->filter(ieee802154_dev, true,
					 IEEE802154_FILTER_TYPE_PAN_ID,
					 &filter);
	}

	return 0;
}

static int start(void)
{
	SYS_LOG_INF("Start IEEE 802.15.4 device");

	return radio_api->start(ieee802154_dev);
}

static int stop(void)
{
	SYS_LOG_INF("Stop IEEE 802.15.4 device");

	return radio_api->stop(ieee802154_dev);
}

static int tx(struct net_pkt *pkt)
{
	struct net_buf *buf = net_buf_frag_last(pkt->frags);
	u8_t seq = net_buf_pull_u8(buf);
	int retries = 3;
	int ret;

	SYS_LOG_DBG("len %d seq %u", buf->len, seq);

	do {
		ret = radio_api->tx(ieee802154_dev, pkt, buf);
	} while (ret && retries--);

	if (ret) {
		SYS_LOG_ERR("Error sending data, seq %u", seq);
		/* Send seq = 0 for unsuccessful send */
		seq = 0;
	}

	try_write(WPANUSB_ENDP_BULK_IN, &seq, sizeof(seq));

	return ret;
}

/**
 * Vendor handler is executed in the ISR context, queue data for
 * later processing
 */
static int wpanusb_vendor_handler(struct usb_setup_packet *setup,
				  s32_t *len, u8_t **data)
{
	struct net_pkt *pkt;
	struct net_buf *buf;

	pkt = net_pkt_get_reserve_tx(0, K_NO_WAIT);
	if (!pkt) {
		return -ENOMEM;
	}

	buf = net_pkt_get_frag(pkt, K_NO_WAIT);
	if (!buf) {
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

	net_pkt_frag_insert(pkt, buf);

	net_buf_add_u8(buf, setup->bRequest);

	/* Add seq to TX */
	if (setup->bRequest == TX) {
		net_buf_add_u8(buf, setup->wIndex);
	}

	memcpy(net_buf_add(buf, *len), *data, *len);

	SYS_LOG_DBG("len %u seq %u", *len, setup->wIndex);

	k_fifo_put(&tx_queue, pkt);

	return 0;
}

static void tx_thread(void)
{
	SYS_LOG_DBG("Tx thread started");

	while (1) {
		u8_t cmd;
		struct net_pkt *pkt;
		struct net_buf *buf;

		pkt = k_fifo_get(&tx_queue, K_FOREVER);
		buf = net_buf_frag_last(pkt->frags);
		cmd = net_buf_pull_u8(buf);

		net_hexdump(">", buf->data, buf->len);

		switch (cmd) {
		case RESET:
			SYS_LOG_DBG("Reset device");
			break;
		case TX:
			tx(pkt);
			break;
		case START:
			start();
			break;
		case STOP:
			stop();
			break;
		case SET_CHANNEL:
			set_channel(buf->data, buf->len);
			break;
		case SET_IEEE_ADDR:
			set_ieee_addr(buf->data, buf->len);
			break;
		case SET_SHORT_ADDR:
			set_short_addr(buf->data, buf->len);
			break;
		case SET_PAN_ID:
			set_pan_id(buf->data, buf->len);
			break;
		default:
			SYS_LOG_ERR("%x: Not handled for now", cmd);
			break;
		}

		net_pkt_unref(pkt);

		k_yield();
	}
}

/* TODO: FIXME: correct buffer size */
static u8_t buffer[300];

static struct usb_cfg_data wpanusb_config = {
	.usb_device_description = (u8_t *)&wpanusb_desc,
	.cb_usb_status = wpanusb_status_cb,
	.interface = {
		.vendor_handler = wpanusb_vendor_handler,
		.vendor_data = buffer,
		.class_handler = NULL,
		.custom_handler = NULL,
	},
	.num_endpoints = ARRAY_SIZE(wpanusb_ep),
	.endpoint = wpanusb_ep,
};

static int wpanusb_init(struct device *dev)
{
	struct wpanusb_dev_data_t * const dev_data = DEV_DATA(dev);
	int ret;

	SYS_LOG_DBG("");

	wpanusb_config.interface.payload_data = dev_data->interface_data;
	wpanusb_dev = dev;

	/* Initialize the USB driver with the right configuration */
	ret = usb_set_config(&wpanusb_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to configure USB");
		return ret;
	}

	/* Enable USB driver */
	ret = usb_enable(&wpanusb_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to enable USB");
		return ret;
	}

	return 0;
}

static struct wpanusb_dev_data_t wpanusb_dev_data = {
	.usb_status = USB_DC_UNKNOWN,
};

#if BOOT_INITIALIZED
DEVICE_INIT(wpanusb, "wpanusb", &wpanusb_init,
	    &wpanusb_dev_data, NULL,
	    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
#define wpanusb_start(dev)
#else
static struct device __dev = {
	.driver_data = &wpanusb_dev_data,
};
static void wpanusb_start(struct device *dev)
{
	wpanusb_init(dev);
}
#endif

static void init_tx_queue(void)
{
	/* Transmit queue init */
	k_fifo_init(&tx_queue);

	k_thread_create(&tx_thread_data, tx_stack,
			K_THREAD_STACK_SIZEOF(tx_stack),
			(k_thread_entry_t)tx_thread,
			NULL, NULL, NULL, K_PRIO_COOP(8), 0, K_NO_WAIT);
}

/**
 * Interface to the network stack, will be called when the packet is
 * received
 */
int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_buf *frag;

	SYS_LOG_DBG("Got data, pkt %p, len %d", pkt, net_pkt_get_len(pkt));

	frag = net_buf_frag_last(pkt->frags);

	/**
	 * Add length 1 byte, do not forget to reserve it
	 */
	net_buf_push_u8(frag, net_pkt_get_len(pkt));

	/**
	 * Add LQI at the end of the packet
	 */
	net_buf_add_u8(frag, net_pkt_ieee802154_lqi(pkt));

	net_hexdump("<", frag->data, net_pkt_get_len(pkt));

	try_write(WPANUSB_ENDP_BULK_IN, frag->data, net_pkt_get_len(pkt));

	net_pkt_unref(pkt);

	return 0;
}

struct shell_cmd commands[] = {
	{ NULL, NULL }
};

void main(void)
{
	wpanusb_start(&__dev);

	SYS_LOG_INF("Start");

	ieee802154_dev = device_get_binding(CONFIG_NET_CONFIG_IEEE802154_DEV_NAME);
	if (!ieee802154_dev) {
		SYS_LOG_ERR("Cannot get IEEE802.15.4 device");
		return;
	}

	/* Initialize net_pkt */
	net_pkt_init();

	/* Initialize transmit queue */
	init_tx_queue();

	radio_api = (struct ieee802154_radio_api *)ieee802154_dev->driver_api;

	/* TODO: Initialize more */

	SYS_LOG_DBG("radio_api %p initialized", radio_api);

	SHELL_REGISTER("wpan", commands);
}
