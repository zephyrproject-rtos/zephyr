/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL 3
#define SYS_LOG_DOMAIN "wpanusb"
#include <logging/sys_log.h>

#include <linker/sections.h>
#include <toolchain.h>
#include <string.h>
#include <misc/printk.h>

#include <device.h>
#include <uart.h>

#include <misc/util.h>
#include <shell/shell.h>

#include <net/buf.h>

#include <usb/usb_device.h>
#include <usb/usb_common.h>

#include <net/ieee802154_radio.h>
#include <net_private.h>

#include "wpanusb.h"

#define WPANUSB_SUBCLASS	0
#define WPANUSB_PROTOCOL	0

#define WPANUSB_BUFFER_SIZE	64

/* Max packet size for endpoints */
#define WPANUSB_BULK_EP_MPS		64

/* Max packet size for Interrupt endpoints */
#define WPANUSB_INTERRUPT_EP_MPS	16

/* Max Bluetooth command data size */
#define WPANUSB_CLASS_MAX_DATA_SIZE	100

#define VENDOR_ID			0x8086 /* Intel Hardware */
#define PRODUCT_ID			0xFF03 /* TODO: Find  better id */
#define DEVICE_RELNUM			0x0100

#define WPANUSB_NUM_CONF		1

#define WPANUSB_NUM_ITF			1

#define WPANUSB_IF1_NUM_EP		1
/* Include all endpoints, also alternate configurations */
#define WPANUSB_NUM_EP		(WPANUSB_IF1_NUM_EP)

#define WPANUSB_ENDP_BULK_IN		0x81

#define WPANUSB_CONF_SIZE (USB_CONFIGURATION_DESC_SIZE + \
			 (WPANUSB_NUM_ITF * USB_INTERFACE_DESC_SIZE) + \
			 (WPANUSB_NUM_EP * USB_ENDPOINT_DESC_SIZE))

/* Misc. macros */
#define LOW_BYTE(x)	((x) & 0xFF)
#define HIGH_BYTE(x)	((x) >> 8)

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
	u8_t notification_sent;
};

/**
 * ieee802.15.4 USB descriptors configuration
 */
static const u8_t wpanusb_desc[] = {
	/* Device descriptor */
	USB_DEVICE_DESC_SIZE,		/* Descriptor size */
	USB_DEVICE_DESC,		/* Descriptor type */
	LOW_BYTE(USB_1_1),
	HIGH_BYTE(USB_1_1),		/* USB version in BCD format */
	CUSTOM_CLASS,			/* Class */
	WPANUSB_SUBCLASS,		/* SubClass - Interface specific */
	WPANUSB_PROTOCOL,		/* Protocol - Interface specific */
	MAX_PACKET_SIZE0,		/* Max Packet Size */
	LOW_BYTE(VENDOR_ID),
	HIGH_BYTE(VENDOR_ID),		/* Vendor Id */
	LOW_BYTE(PRODUCT_ID),
	HIGH_BYTE(PRODUCT_ID),		/* Product Id */
	LOW_BYTE(DEVICE_RELNUM),
	HIGH_BYTE(DEVICE_RELNUM),	/* Device Release Number */
	/* Index of Manufacturer String Descriptor */
	0x00,
	/* Index of Product String Descriptor */
	0x00,
	/* Index of Serial Number String Descriptor */
	0x00,
	WPANUSB_NUM_CONF,		/* Number of Possible Configuration */

	/* Configuration descriptor */
	USB_CONFIGURATION_DESC_SIZE,	/* Descriptor size */
	USB_CONFIGURATION_DESC,		/* Descriptor type */
	/* Total length in bytes of data returned */
	LOW_BYTE(WPANUSB_CONF_SIZE),
	HIGH_BYTE(WPANUSB_CONF_SIZE),
	WPANUSB_NUM_ITF,		/* Number of interfaces */
	0x01,				/* Configuration value */
	0x00,				/* Index of the Configuration string */
	USB_CONFIGURATION_ATTRIBUTES,	/* Attributes */
	MAX_LOW_POWER,			/* Max power consumption */

	/* Interface descriptor 0 */
	USB_INTERFACE_DESC_SIZE,	/* Descriptor size */
	USB_INTERFACE_DESC,		/* Descriptor type */
	0x00,				/* Interface index */
	0x00,				/* Alternate setting */
	WPANUSB_IF1_NUM_EP,		/* Number of Endpoints */
	CUSTOM_CLASS,			/* Class */
	WPANUSB_SUBCLASS,		/* SubClass */
	WPANUSB_PROTOCOL,		/* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Endpoint IN */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	WPANUSB_ENDP_BULK_IN,		/* Endpoint address */
	USB_DC_EP_BULK,			/* Attributes */
	LOW_BYTE(WPANUSB_BULK_EP_MPS),
	HIGH_BYTE(WPANUSB_BULK_EP_MPS),	/* Max packet size */
	0x00,				/* Interval */

#if 0
	/* String descriptor language, only one, so min size 4 bytes.
	 * 0x0409 English(US) language code used
	 */
	USB_STRING_DESC_SIZE,           /* Descriptor size */
	USB_STRING_DESC,                /* Descriptor type */
	0x09,
	0x04,

	/* Manufacturer String Descriptor "Intel" */
	0x0C,
	USB_STRING_DESC,
	'I', 0, 'n', 0, 't', 0, 'e', 0, 'l', 0,

	/* Product String Descriptor */
	0x10,
	USB_STRING_DESC,
	'Q', 0, 'U', 0, 'A', 0, 'R', 0, 'Q', 0, 'A', 0, 'T', 0,

	/* Serial Number String Descriptor "00.01" */
	0x0C,
	USB_STRING_DESC,
	'0', 0, '0', 0, '.', 0, '0', 0, '1', 0,
#endif
};

#ifdef VERBOSE_DEBUG
/* TODO: move to standard utils */
static void hexdump(const char *str, const u8_t *packet, size_t length)
{
	int n = 0;

	if (!length) {
		printk("%s zero-length signal packet\n", str);
		return;
	}

	while (length--) {
		if (n % 16 == 0) {
			printk("%s %08X ", str, n);
		}

		printk("%02X ", *packet++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printk("\n");
			} else {
				printk(" ");
			}
		}
	}

	if (n % 16) {
		printk("\n");
	}
}
#else
#define hexdump(...)
#endif

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

static int wpanusb_class_handler(struct usb_setup_packet *setup,
				 s32_t *len, u8_t **data)
{
	SYS_LOG_DBG("len %d", *len);

	hexdump(">", *data, *len);

	return 0;
}

static int wpanusb_custom_handler(struct usb_setup_packet *setup,
				  s32_t *len, u8_t **data)
{
	/**
	 * FIXME:
	 * Keep custom handler for now in a case some data is sent
	 * over this endpoint, at the moment return to higher layer.
	 */
	return -ENOTSUP;
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

		return radio_api->set_filter(ieee802154_dev,
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

		return radio_api->set_filter(ieee802154_dev,
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

		return radio_api->set_filter(ieee802154_dev,
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

		hexdump(">", buf->data, buf->len);

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
	.usb_device_description = wpanusb_desc,
	.cb_usb_status = wpanusb_status_cb,
	.interface = {
		.class_handler = wpanusb_class_handler,
		.custom_handler = wpanusb_custom_handler,
		.vendor_handler = wpanusb_vendor_handler,
		.vendor_data = buffer,
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

int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_buf *frag;

	SYS_LOG_DBG("Got data, pkt %p, frags->len %d",
		    pkt, net_pkt_get_len(pkt));

	frag = net_buf_frag_last(pkt->frags);

	/* Linux requires LQI to be put at the beginning of the buffer */
	memmove(frag->data+1, frag->data, frag->len);
	frag->data[0] = net_pkt_ieee802154_lqi(pkt);

	/**
	 * Add length 1 byte, do not forget to reserve it
	 */
	net_buf_push_u8(frag, net_pkt_get_len(pkt) - 1);

	hexdump("<", frag->data, net_pkt_get_len(pkt));

	try_write(WPANUSB_ENDP_BULK_IN, frag->data, net_pkt_get_len(pkt));

	net_pkt_unref(pkt);

	return 0;
}

static int shell_cmd_help(int argc, char *argv[])
{
	/* Keep the commands in alphabetical order */
	printk("wpanusb help\n");

	return 0;
}

struct shell_cmd commands[] = {
	{ "help", shell_cmd_help, "help" },
	{ NULL, NULL }
};

void main(void)
{
	wpanusb_start(&__dev);

	SYS_LOG_INF("Start");

#if DYNAMIC_REGISTER
	ieee802154_dev = ieee802154_register_raw();
#else
	ieee802154_dev = device_get_binding(CONFIG_IEEE802154_CC2520_DRV_NAME);
	if (!ieee802154_dev) {
		SYS_LOG_ERR("Cannot get CC250 device");
		return;
	}
#endif

	/* Initialize net_pkt */
	net_pkt_init();

	/* Initialize transmit queue */
	init_tx_queue();

	radio_api = (struct ieee802154_radio_api *)ieee802154_dev->driver_api;

	/* TODO: Initialize more */

	SYS_LOG_DBG("radio_api %p initialized", radio_api);

	SHELL_REGISTER("wpan", commands);
}
