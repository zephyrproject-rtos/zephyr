/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define SYS_LOG_LEVEL 4
#include <misc/sys_log.h>

#include <sections.h>
#include <toolchain.h>
#include <stdio.h>
#include <string.h>

#include <device.h>
#include <uart.h>

#include <misc/util.h>

#include <net/buf.h>

#include <usb_device.h>
#include <usb_common.h>

#include <net/ieee802154_radio.h>
#include <net_private.h>

#include "wpanusb.h"

#define ATUSB_SUBCLASS	0
#define ATUSB_PROTOCOL	0

#define ATUSB_BUFFER_SIZE 64

/* Max packet size for endpoints */
#define ATUSB_BULK_EP_MPS		64

/* Max packet size for Interrupt endpoints */
#define ATUSB_INTERRUPT_EP_MPS		16

/* Max Bluetooth command data size */
#define ATUSB_CLASS_MAX_DATA_SIZE	100

#define VENDOR_ID			0x8086 /* Intel Hardware */
#define PRODUCT_ID			0xFF03 /* TODO: Find  better id */
#define DEVICE_RELNUM			0x0100

#define ATUSB_NUM_CONF			1

#define ATUSB_NUM_ITF			1

#define ATUSB_IF1_NUM_EP		1
/* Include all endpoints, also alternate configurations */
#define ATUSB_NUM_EP		(ATUSB_IF1_NUM_EP)

#define ATUSB_ENDP_BULK_IN		0x81

#define ATUSB_CONF_SIZE (USB_CONFIGURATION_DESC_SIZE + \
			 (ATUSB_NUM_ITF * USB_INTERFACE_DESC_SIZE) + \
			 (ATUSB_NUM_EP * USB_ENDPOINT_DESC_SIZE))

/* Misc. macros */
#define LOW_BYTE(x)	((x) & 0xFF)
#define HIGH_BYTE(x)	((x) >> 8)

static struct device *wpanusb_dev;

static struct ieee802154_radio_api *radio_api;
static struct device *ieee802154_dev;

static struct nano_fifo tx_queue;

/**
 * Stack for the tx fiber.
 */
static char __noinit __stack tx_fiber_stack[1024];
static nano_thread_id_t tx_fiber_id;

#define DEV_DATA(dev) \
	((struct wpanusb_dev_data_t * const)(dev)->driver_data)

/* Device data structure */
struct wpanusb_dev_data_t {
	/* USB device status code */
	enum usb_dc_status_code usb_status;
	uint8_t interface_data[ATUSB_CLASS_MAX_DATA_SIZE];
	uint8_t notification_sent;
};

/**
 * ieee802.15.4 USB descriptors configuration
 */
static const uint8_t wpanusb_desc[] = {
	/* Device descriptor */
	USB_DEVICE_DESC_SIZE,		/* Descriptor size */
	USB_DEVICE_DESC,		/* Descriptor type */
	LOW_BYTE(USB_1_1),
	HIGH_BYTE(USB_1_1),		/* USB version in BCD format */
	CUSTOM_CLASS,			/* Class */
	ATUSB_SUBCLASS,			/* SubClass - Interface specific */
	ATUSB_PROTOCOL,			/* Protocol - Interface specific */
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
	ATUSB_NUM_CONF,			/* Number of Possible Configuration */

	/* Configuration descriptor */
	USB_CONFIGURATION_DESC_SIZE,	/* Descriptor size */
	USB_CONFIGURATION_DESC,		/* Descriptor type */
	/* Total length in bytes of data returned */
	LOW_BYTE(ATUSB_CONF_SIZE),
	HIGH_BYTE(ATUSB_CONF_SIZE),
	ATUSB_NUM_ITF,			/* Number of interfaces */
	0x01,				/* Configuration value */
	0x00,				/* Index of the Configuration string */
	USB_CONFIGURATION_ATTRIBUTES,	/* Attributes */
	MAX_LOW_POWER,			/* Max power consumption */

	/* Interface descriptor 0 */
	USB_INTERFACE_DESC_SIZE,	/* Descriptor size */
	USB_INTERFACE_DESC,		/* Descriptor type */
	0x00,				/* Interface index */
	0x00,				/* Alternate setting */
	ATUSB_IF1_NUM_EP,		/* Number of Endpoints */
	CUSTOM_CLASS,			/* Class */
	ATUSB_SUBCLASS,			/* SubClass */
	ATUSB_PROTOCOL,			/* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Endpoint IN */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	ATUSB_ENDP_BULK_IN,		/* Endpoint address */
	USB_DC_EP_BULK,			/* Attributes */
	LOW_BYTE(ATUSB_BULK_EP_MPS),
	HIGH_BYTE(ATUSB_BULK_EP_MPS),	/* Max packet size */
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

/* TODO: move to standard utils */
static void hexdump(const char *str, const uint8_t *packet, size_t length)
{
	int n = 0;

	if (!length) {
		printf("%s zero-length signal packet\n", str);
		return;
	}

	while (length--) {
		if (n % 16 == 0) {
			printf("%s %08X ", str, n);
		}

		printf("%02X ", *packet++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printf("\n");
			} else {
				printf(" ");
			}
		}
	}

	if (n % 16) {
		printf("\n");
	}
}

/* EP Bulk IN handler, used to send data to the Host */
static void wpanusb_bulk_in(uint8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	SYS_LOG_DBG("");
}

/* Describe EndPoints configuration */
static struct usb_ep_cfg_data wpanusb_ep[] = {
	{
		.ep_cb = wpanusb_bulk_in,
		.ep_addr = ATUSB_ENDP_BULK_IN
	},
};

static void wpanusb_status_cb(enum usb_dc_status_code status)
{
	struct wpanusb_dev_data_t * const dev_data = DEV_DATA(wpanusb_dev);

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
				 int32_t *len, uint8_t **data)
{
	SYS_LOG_DBG("len %d", *len);

	hexdump(">", *data, *len);

	return 0;
}

static int wpanusb_custom_handler(struct usb_setup_packet *setup,
				  int32_t *len, uint8_t **data)
{
	/**
	 * FIXME:
	 * Keep custom handler for now in a case some data is sent
	 * over this endpoint, at the moment return to higher layer.
	 */
	return -ENOTSUP;
}

static int try_write(uint8_t ep, uint8_t *data, uint16_t len)
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

	return radio_api->set_ieee_addr(ieee802154_dev,
					(uint8_t *)&req->ieee_addr);
}

static int set_short_addr(void *data, int len)
{
	struct set_short_addr *req = data;

	SYS_LOG_DBG("len %u", len);

	return radio_api->set_short_addr(ieee802154_dev, req->short_addr);
}

static int set_pan_id(void *data, int len)
{
	struct set_pan_id *req = data;

	SYS_LOG_DBG("len %u", len);

	return radio_api->set_pan_id(ieee802154_dev, req->pan_id);
}

static int start(void)
{
	SYS_LOG_DBG("");

	return radio_api->start(ieee802154_dev);
}

static int stop(void)
{
	SYS_LOG_DBG("");

	return radio_api->stop(ieee802154_dev);
}

static int tx(struct net_buf *pkt)
{
	struct net_buf *buf = net_buf_frag_last(pkt);
	uint8_t seq = net_buf_pull_u8(buf);
	int ret;

	SYS_LOG_DBG("send data len %d seq %u", buf->len, seq);

	/**
	 * TODO: get buffer and send packet
	 */

	/**
	 * Pass to ieee802154 driver nbuf packet
	 */
	ret = radio_api->tx(ieee802154_dev, pkt);
	if (!ret) {
		SYS_LOG_DBG("Send ACK for seq %u", seq);

		try_write(ATUSB_ENDP_BULK_IN, &seq, sizeof(seq));
	}

	return ret;
}

/**
 * Vendor handler is executed in the ISR context, queue data for
 * later processing
 */
static int wpanusb_vendor_handler(struct usb_setup_packet *setup,
				  int32_t *len, uint8_t **data)
{
	struct net_buf *pkt, *buf;

	pkt = net_nbuf_get_reserve_tx(0);
	buf = net_nbuf_get_reserve_data(0);
	net_buf_frag_insert(pkt, buf);

	net_buf_add_u8(buf, setup->bRequest);

	/* Add seq to TX */
	if (setup->bRequest == TX) {
		net_buf_add_u8(buf, setup->wIndex);
	}

	memcpy(net_buf_add(buf, *len), *data, *len);

	SYS_LOG_DBG("len %u seq %u", *len, setup->wIndex);

	net_buf_put(&tx_queue, pkt);

	return 0;
}

static void tx_fiber(void)
{
	SYS_LOG_INF("Tx fiber started");

	while (1) {
		uint8_t cmd;
		struct net_buf *pkt, *buf;

		pkt = net_buf_get_timeout(&tx_queue, 0, TICKS_UNLIMITED);
		buf = net_buf_frag_last(pkt);
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

		net_nbuf_unref(pkt);

		fiber_yield();
	}
}

/* TODO: FIXME: correct buffer size */
static uint8_t buffer[300];

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
	nano_fifo_init(&tx_queue);

	tx_fiber_id = fiber_start(tx_fiber_stack, sizeof(tx_fiber_stack),
				  (nano_fiber_entry_t)tx_fiber,
				  0, 0, 8, 0);
}

extern enum net_verdict ieee802154_radio_handle_ack(struct net_if *iface,
						    struct net_buf *buf)
{
	SYS_LOG_DBG("");

	/* parse on higher layer */
	return NET_CONTINUE;
}

int ieee802154_radio_send(struct net_if *iface, struct net_buf *buf)
{
	SYS_LOG_DBG("");

	return -ENOTSUP;
}

void ieee802154_init(struct net_if *iface)
{
	SYS_LOG_DBG("");
}

int net_recv_data(struct net_if *iface, struct net_buf *buf)
{
	struct net_buf *frag;

	SYS_LOG_DBG("Got data, buf %p, len %d frags->len %d",
		    buf, buf->len, net_buf_frags_len(buf));

	frag = net_buf_frag_last(buf);

	/**
	 * Add length 1 byte, do not forget to reserve it
	 */
	net_buf_push_u8(frag, net_buf_frags_len(buf) - 1);

	hexdump("<", frag->data, net_buf_frags_len(buf));

	try_write(ATUSB_ENDP_BULK_IN, frag->data, net_buf_frags_len(buf));

	net_nbuf_unref(buf);

	return 0;
}

void main(void)
{
	wpanusb_start(&__dev);

	SYS_LOG_INF("Start");

#if DYNAMIC_REGISTER
	ieee802154_dev = ieee802154_register_raw();
#else
	ieee802154_dev = device_get_binding(CONFIG_TI_CC2520_DRV_NAME);
	if (!ieee802154_dev) {
		SYS_LOG_ERR("Cannot get CC250 device");
		return;
	}
#endif

	/* Initialize nbufs */
	net_nbuf_init();

	/* Initialize transmit queue */
	init_tx_queue();

	radio_api = (struct ieee802154_radio_api *)ieee802154_dev->driver_api;

	/* TODO: Initialize more */

	SYS_LOG_DBG("radio_api %p initialized", radio_api);
}
