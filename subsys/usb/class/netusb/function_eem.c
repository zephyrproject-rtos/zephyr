/*
 * Copyright (c) 2017 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_USB_DEVICE_NETWORK_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_eem);

#include <net_private.h>
#include <zephyr.h>
#include <usb_device.h>
#include <usb_common.h>

#include <net/net_pkt.h>

#include <usb_descriptor.h>
#include <class/usb_cdc.h>
#include "netusb.h"

static u8_t tx_buf[NETUSB_MTU], rx_buf[NETUSB_MTU];

struct usb_cdc_eem_config {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_out_ep;
} __packed;

USBD_CLASS_DESCR_DEFINE(primary, 0) struct usb_cdc_eem_config cdc_eem_cfg = {
	/* Interface descriptor 0 */
	/* CDC Communication interface */
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = COMMUNICATION_DEVICE_CLASS,
		.bInterfaceSubClass = EEM_SUBCLASS,
		.bInterfaceProtocol = EEM_PROTOCOL,
		.iInterface = 0,
	},

	/* Data Endpoint IN */
	.if0_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = CDC_EEM_IN_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_CDC_EEM_BULK_EP_MPS),
		.bInterval = 0x00,
	},

	/* Data Endpoint OUT */
	.if0_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = CDC_EEM_OUT_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_CDC_EEM_BULK_EP_MPS),
		.bInterval = 0x00,
	},
};

static u8_t eem_get_first_iface_number(void)
{
	return cdc_eem_cfg.if0.bInterfaceNumber;
}

#define EEM_OUT_EP_IDX		0
#define EEM_IN_EP_IDX		1

static struct usb_ep_cfg_data eem_ep_data[] = {
	{
		/* Use transfer API */
		.ep_cb = usb_transfer_ep_callback,
		.ep_addr = CDC_EEM_OUT_EP_ADDR
	},
	{
		/* Use transfer API */
		.ep_cb = usb_transfer_ep_callback,
		.ep_addr = CDC_EEM_IN_EP_ADDR
	},
};

static inline u16_t eem_pkt_size(u16_t hdr)
{
	if (hdr & BIT(15)) {
		return hdr & 0x07ff;
	} else {
		return hdr & 0x3fff;
	}
}

static int eem_send(struct net_pkt *pkt)
{
	u8_t sentinel[4] = { 0xde, 0xad, 0xbe, 0xef };
	u16_t *hdr = (u16_t *)&tx_buf[0];
	struct net_buf *frag;
	int ret, len, b_idx = 0;

	/* With EEM, it's possible to send multiple ethernet packets in one
	 * transfer, we don't do that for now.
	 */
	len = net_pkt_get_len(pkt) + sizeof(sentinel);

	/* Add EEM header */
	*hdr = sys_cpu_to_le16(0x3FFF & len);
	b_idx += sizeof(u16_t);

	/* generate transfer buffer */
	for (frag = pkt->frags; frag; frag = frag->frags) {
		memcpy(&tx_buf[b_idx], frag->data, frag->len);
		b_idx += frag->len;
	}

	/* Add crc-sentinel */
	memcpy(&tx_buf[b_idx], sentinel, sizeof(sentinel));
	b_idx += sizeof(sentinel);

	/* transfer data to host */
	ret = usb_transfer_sync(eem_ep_data[EEM_IN_EP_IDX].ep_addr,
				tx_buf, b_idx,
				USB_TRANS_WRITE);
	if (ret != b_idx) {
		LOG_ERR("Transfer failure");
		return -EIO;
	}

	return 0;
}

static void eem_read_cb(u8_t ep, int size, void *priv)
{
	u8_t *ptr = rx_buf;

	do {
		u16_t eem_hdr, eem_size;
		struct net_pkt *pkt;
		struct net_buf *frag;

		if (size < sizeof(u16_t)) {
			break;
		}

		eem_hdr = sys_le16_to_cpu(*(u16_t *)ptr);
		eem_size = eem_pkt_size(eem_hdr);

		if (eem_size + sizeof(u16_t) > size) {
			/* eem pkt greater than transferred data */
			LOG_ERR("pkt size error");
			break;
		}

		size -= sizeof(u16_t);
		ptr += sizeof(u16_t);

		if (eem_hdr & BIT(15)) {
			/* EEM Command, do nothing for now */
			goto done;
		}

		LOG_DBG("hdr 0x%x, eem_size %d, size %d",
			eem_hdr, eem_size, size);

		if (!size || !eem_size) {
			LOG_DBG("no payload");
			break;
		}

		pkt = net_pkt_get_reserve_rx(K_FOREVER);
		if (!pkt) {
			LOG_ERR("Unable to alloc pkt\n");
			break;
		}

		frag = net_pkt_get_frag(pkt, K_FOREVER);
		if (!frag) {
			LOG_ERR("Unable to alloc fragment");
			net_pkt_unref(pkt);
			break;
		}

		net_pkt_frag_insert(pkt, frag);

		/* copy payload and discard 32-bit sentinel */
		if (!net_pkt_append_all(pkt, eem_size - 4, ptr, K_FOREVER)) {
			LOG_ERR("Unable to append pkt\n");
			net_pkt_unref(pkt);
			break;
		}

		netusb_recv(pkt);

done:
		size -= eem_size;
		ptr += eem_size;
	} while (size);

	usb_transfer(eem_ep_data[EEM_OUT_EP_IDX].ep_addr, rx_buf,
		     sizeof(rx_buf), USB_TRANS_READ, eem_read_cb, NULL);
}

static int eem_connect(bool connected)
{
	if (connected) {
		eem_read_cb(eem_ep_data[EEM_OUT_EP_IDX].ep_addr, 0, NULL);
	} else {
		/* Cancel any transfer */
		usb_cancel_transfer(eem_ep_data[EEM_OUT_EP_IDX].ep_addr);
		usb_cancel_transfer(eem_ep_data[EEM_IN_EP_IDX].ep_addr);
	}

	return 0;
}

static struct netusb_function eem_function = {
	.connect_media = eem_connect,
	.send_pkt = eem_send,
};

static inline void eem_status_interface(const u8_t *iface)
{
	LOG_DBG("");

	if (*iface != eem_get_first_iface_number()) {
		return;
	}

	netusb_enable(&eem_function);
}

static void eem_status_cb(enum usb_dc_status_code status, const u8_t *param)
{
	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_DISCONNECTED:
		LOG_DBG("USB device disconnected");
		netusb_disable();
		break;

	case USB_DC_INTERFACE:
		LOG_DBG("USB interface selected");
		eem_status_interface(param);
		break;

	case USB_DC_ERROR:
	case USB_DC_RESET:
	case USB_DC_CONNECTED:
	case USB_DC_CONFIGURED:
	case USB_DC_SUSPEND:
	case USB_DC_RESUME:
		LOG_DBG("USB unhandlded state: %d", status);
		break;

	case USB_DC_SOF:
		break;

	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("USB unknown state: %d", status);
		break;
	}
}

static void eem_interface_config(struct usb_desc_header *head,
				 u8_t bInterfaceNumber)
{
	ARG_UNUSED(head);

	cdc_eem_cfg.if0.bInterfaceNumber = bInterfaceNumber;
}

USBD_CFG_DATA_DEFINE(netusb) struct usb_cfg_data netusb_config = {
	.usb_device_description = NULL,
	.interface_config = eem_interface_config,
	.interface_descriptor = &cdc_eem_cfg.if0,
	.cb_usb_status = eem_status_cb,
	.interface = {
		.class_handler = NULL,
		.custom_handler = NULL,
		.vendor_handler = NULL,
		.payload_data = NULL,
	},
	.num_endpoints = ARRAY_SIZE(eem_ep_data),
	.endpoint = eem_ep_data,
};
