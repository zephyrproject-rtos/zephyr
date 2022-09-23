/*
 * Copyright (c) 2016-2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_USB_DEVICE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wpanusb);

#include <zephyr/usb/usb_device.h>
#include <usb_descriptor.h>

#include <zephyr/net/buf.h>
#include <zephyr/net/ieee802154_radio.h>
#include <ieee802154/ieee802154_frame.h>
#include <net_private.h>

#include "wpanusb.h"

#if IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(8)
#endif

#define WPANUSB_SUBCLASS	0
#define WPANUSB_PROTOCOL	0

/* Max packet size for endpoints */
#if IS_ENABLED(CONFIG_USB_DC_HAS_HS_SUPPORT)
#define WPANUSB_BULK_EP_MPS		512
#else
#define WPANUSB_BULK_EP_MPS		64
#endif

#define WPANUSB_IN_EP_IDX		0

static struct ieee802154_radio_api *radio_api;
static const struct device *const ieee802154_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_ieee802154));

static struct k_fifo tx_queue;

/* IEEE802.15.4 frame + 1 byte len + 1 byte LQI */
uint8_t tx_buf[IEEE802154_MAX_PHY_PACKET_SIZE + 1 + 1];

/**
 * Stack for the tx thread.
 */
static K_THREAD_STACK_DEFINE(tx_stack, 1024);
static struct k_thread tx_thread_data;

#define INITIALIZER_IF(num_ep, iface_class)				\
	{								\
		.bLength = sizeof(struct usb_if_descriptor),		\
		.bDescriptorType = USB_DESC_INTERFACE,			\
		.bInterfaceNumber = 0,					\
		.bAlternateSetting = 0,					\
		.bNumEndpoints = num_ep,				\
		.bInterfaceClass = iface_class,				\
		.bInterfaceSubClass = 0,				\
		.bInterfaceProtocol = 0,				\
		.iInterface = 0,					\
	}

#define INITIALIZER_IF_EP(addr, attr, mps, interval)			\
	{								\
		.bLength = sizeof(struct usb_ep_descriptor),		\
		.bDescriptorType = USB_DESC_ENDPOINT,			\
		.bEndpointAddress = addr,				\
		.bmAttributes = attr,					\
		.wMaxPacketSize = sys_cpu_to_le16(mps),			\
		.bInterval = interval,					\
	}

USBD_CLASS_DESCR_DEFINE(primary, 0) struct {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_in_ep;
} __packed wpanusb_desc = {
	.if0 = INITIALIZER_IF(1, USB_BCC_VENDOR),
	.if0_in_ep = INITIALIZER_IF_EP(AUTO_EP_IN, USB_DC_EP_BULK,
				       WPANUSB_BULK_EP_MPS, 0),
};

/* Describe EndPoints configuration */
static struct usb_ep_cfg_data wpanusb_ep[] = {
	{
		.ep_cb = usb_transfer_ep_callback,
		.ep_addr = AUTO_EP_IN,
	},
};

static void wpanusb_status_cb(struct usb_cfg_data *cfg,
			      enum usb_dc_status_code status,
			      const uint8_t *param)
{
	ARG_UNUSED(param);
	ARG_UNUSED(cfg);

	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_ERROR:
		LOG_DBG("USB device error");
		break;
	case USB_DC_RESET:
		LOG_DBG("USB device reset detected");
		break;
	case USB_DC_CONNECTED:
		LOG_DBG("USB device connected");
		break;
	case USB_DC_CONFIGURED:
		LOG_DBG("USB device configured");
		break;
	case USB_DC_DISCONNECTED:
		LOG_DBG("USB device disconnected");
		break;
	case USB_DC_SUSPEND:
		LOG_DBG("USB device suspended");
		break;
	case USB_DC_RESUME:
		LOG_DBG("USB device resumed");
		break;
	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("USB unknown state");
		break;
		}
}

/**
 * Vendor handler is executed in the ISR context, queue data for
 * later processing
 */
static int wpanusb_vendor_handler(struct usb_setup_packet *setup,
				  int32_t *len, uint8_t **data)
{
	struct net_pkt *pkt;

	if (usb_reqtype_is_to_host(setup)) {
		return -ENOTSUP;
	}

	/* Maximum 2 bytes are added to the len */
	pkt = net_pkt_alloc_with_buffer(NULL, *len + 2, AF_UNSPEC, 0,
					K_NO_WAIT);
	if (!pkt) {
		return -ENOMEM;
	}

	net_pkt_write_u8(pkt, setup->bRequest);

	/* Add seq to TX */
	if (setup->bRequest == TX) {
		net_pkt_write_u8(pkt, setup->wIndex);
	}

	net_pkt_write(pkt, *data, *len);

	LOG_DBG("pkt %p len %u seq %u", pkt, *len, setup->wIndex);

	k_fifo_put(&tx_queue, pkt);

	return 0;
}

USBD_DEFINE_CFG_DATA(wpanusb_config) = {
	.usb_device_description = NULL,
	.interface_descriptor = &wpanusb_desc.if0,
	.cb_usb_status = wpanusb_status_cb,
	.interface = {
		.vendor_handler = wpanusb_vendor_handler,
		.class_handler = NULL,
		.custom_handler = NULL,
	},
	.num_endpoints = ARRAY_SIZE(wpanusb_ep),
	.endpoint = wpanusb_ep,
};

/* Decode wpanusb commands */

static int set_channel(void *data, int len)
{
	struct set_channel *req = data;

	LOG_DBG("page %u channel %u", req->page, req->channel);

	return radio_api->set_channel(ieee802154_dev, req->channel);
}

static int set_ieee_addr(void *data, int len)
{
	struct set_ieee_addr *req = data;

	LOG_DBG("len %u", len);

	if (IEEE802154_HW_FILTER &
	    radio_api->get_capabilities(ieee802154_dev)) {
		struct ieee802154_filter filter;

		filter.ieee_addr = (uint8_t *)&req->ieee_addr;

		return radio_api->filter(ieee802154_dev, true,
					 IEEE802154_FILTER_TYPE_IEEE_ADDR,
					 &filter);
	}

	return 0;
}

static int set_short_addr(void *data, int len)
{
	struct set_short_addr *req = data;

	LOG_DBG("len %u", len);


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

	LOG_DBG("len %u", len);

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
	LOG_INF("Start IEEE 802.15.4 device");

	return radio_api->start(ieee802154_dev);
}

static int stop(void)
{
	LOG_INF("Stop IEEE 802.15.4 device");

	return radio_api->stop(ieee802154_dev);
}

static int tx(struct net_pkt *pkt)
{
	uint8_t ep = wpanusb_config.endpoint[WPANUSB_IN_EP_IDX].ep_addr;
	struct net_buf *buf = net_buf_frag_last(pkt->buffer);
	uint8_t seq = net_buf_pull_u8(buf);
	int retries = 3;
	int ret;

	LOG_DBG("len %d seq %u", buf->len, seq);

	do {
		ret = radio_api->tx(ieee802154_dev, IEEE802154_TX_MODE_DIRECT,
				    pkt, buf);
	} while (ret && retries--);

	if (ret) {
		LOG_ERR("Error sending data, seq %u", seq);
		/* Send seq = 0 for unsuccessful send */
		seq = 0U;
	}

	ret = usb_transfer_sync(ep, &seq, sizeof(seq), USB_TRANS_WRITE);
	if (ret != sizeof(seq)) {
		LOG_ERR("Error sending seq");
		ret = -EINVAL;
	} else {
		ret = 0;
	}

	return ret;
}

static void tx_thread(void)
{
	LOG_DBG("Tx thread started");

	while (1) {
		uint8_t cmd;
		struct net_pkt *pkt;
		struct net_buf *buf;

		pkt = k_fifo_get(&tx_queue, K_FOREVER);
		buf = net_buf_frag_last(pkt->buffer);
		cmd = net_buf_pull_u8(buf);

		net_pkt_hexdump(pkt, ">");

		switch (cmd) {
		case RESET:
			LOG_DBG("Reset device");
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
			LOG_ERR("%x: Not handled for now", cmd);
			break;
		}

		net_pkt_unref(pkt);

		k_yield();
	}
}

static void init_tx_queue(void)
{
	/* Transmit queue init */
	k_fifo_init(&tx_queue);

	k_thread_create(&tx_thread_data, tx_stack,
			K_THREAD_STACK_SIZEOF(tx_stack),
			(k_thread_entry_t)tx_thread,
			NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
}

/**
 * Interface to the network stack, will be called when the packet is
 * received
 */
int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	size_t len = net_pkt_get_len(pkt);
	uint8_t *p = tx_buf;
	int ret;
	uint8_t ep;

	LOG_DBG("Got data, pkt %p, len %d", pkt, len);

	net_pkt_hexdump(pkt, "<");

	if (len > (sizeof(tx_buf) - 2)) {
		LOG_ERR("Too large packet");
		ret = -ENOMEM;
		goto out;
	}

	/**
	 * Add length 1 byte
	 */
	*p++ = (uint8_t)len;

	/* This is needed to work with pkt */
	net_pkt_cursor_init(pkt);

	ret = net_pkt_read(pkt, p, len);
	if (ret < 0) {
		LOG_ERR("Cannot read pkt");
		goto out;
	}

	p += len;

	/**
	 * Add LQI at the end of the packet
	 */
	*p = net_pkt_ieee802154_lqi(pkt);

	ep = wpanusb_config.endpoint[WPANUSB_IN_EP_IDX].ep_addr;

	ret = usb_transfer_sync(ep, tx_buf, len + 2,
				USB_TRANS_WRITE | USB_TRANS_NO_ZLP);
	if (ret != len + 2) {
		LOG_ERR("Transfer failure");
		ret = -EINVAL;
	}

out:
	net_pkt_unref(pkt);

	return ret;
}

enum net_verdict ieee802154_radio_handle_ack(struct net_if *iface, struct net_pkt *pkt)
{
	return NET_CONTINUE;
}

void main(void)
{
	int ret;
	LOG_INF("Starting wpanusb");

	if (!device_is_ready(ieee802154_dev)) {
		LOG_ERR("IEEE802.15.4 device not ready");
		return;
	}

	/* Initialize net_pkt */
	net_pkt_init();

	/* Initialize transmit queue */
	init_tx_queue();

	radio_api = (struct ieee802154_radio_api *)ieee802154_dev->api;

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}
	/* TODO: Initialize more */

	LOG_DBG("radio_api %p initialized", radio_api);
}
