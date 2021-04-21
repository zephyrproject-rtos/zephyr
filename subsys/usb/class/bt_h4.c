/*
 * Wireless / Bluetooth USB class
 *
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <sys/byteorder.h>

#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <usb_descriptor.h>

#include <net/buf.h>

#include <bluetooth/buf.h>
#include <bluetooth/hci_raw.h>
#include <bluetooth/l2cap.h>

#define LOG_LEVEL CONFIG_USB_DEVICE_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_bt_h4);

static K_FIFO_DEFINE(rx_queue);
static K_FIFO_DEFINE(tx_queue);

#define BT_H4_OUT_EP_ADDR               0x01
#define BT_H4_IN_EP_ADDR                0x81

#define BT_H4_OUT_EP_IDX                0
#define BT_H4_IN_EP_IDX                 1

/* HCI RX/TX threads */
static K_KERNEL_STACK_DEFINE(rx_thread_stack, 512);
static struct k_thread rx_thread_data;
static K_KERNEL_STACK_DEFINE(tx_thread_stack, 512);
static struct k_thread tx_thread_data;

/* HCI USB state flags */
static bool configured;
static bool suspended;

struct usb_bt_h4_config {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
} __packed;

USBD_CLASS_DESCR_DEFINE(primary, 0) struct usb_bt_h4_config bt_h4_cfg = {
	/* Interface descriptor 0 */
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = CUSTOM_CLASS, /* TBD */
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},

	/* Data Endpoint OUT */
	.if0_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = BT_H4_OUT_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(USB_MAX_FS_BULK_MPS),
		.bInterval = 0x01,
	},

	/* Data Endpoint IN */
	.if0_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = BT_H4_IN_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(USB_MAX_FS_BULK_MPS),
		.bInterval = 0x01,
	},
};

static struct usb_ep_cfg_data bt_h4_ep_data[] = {
	{
		.ep_cb = usb_transfer_ep_callback,
		.ep_addr = BT_H4_OUT_EP_ADDR,
	},
	{
		.ep_cb = usb_transfer_ep_callback,
		.ep_addr = BT_H4_IN_EP_ADDR,
	},
};

static void bt_h4_read(uint8_t ep, int size, void *priv)
{
	static uint8_t data[USB_MAX_FS_BULK_MPS];

	if (size > 0) {
		struct net_buf *buf;

		buf = bt_buf_get_tx(BT_BUF_H4, K_FOREVER, data, size);
		if (!buf) {
			LOG_ERR("Cannot get free TX buffer\n");
			return;
		}

		net_buf_put(&rx_queue, buf);
	}

	/* Start a new read transfer */
	usb_transfer(bt_h4_ep_data[BT_H4_OUT_EP_IDX].ep_addr, data,
		     USB_MAX_FS_BULK_MPS, USB_TRANS_READ, bt_h4_read, NULL);
}

static void hci_tx_thread(void)
{
	LOG_DBG("Start USB Bluetooth thread");

	while (true) {
		struct net_buf *buf;

		buf = net_buf_get(&tx_queue, K_FOREVER);

		usb_transfer_sync(bt_h4_ep_data[BT_H4_IN_EP_IDX].ep_addr,
				  buf->data, buf->len, USB_TRANS_WRITE);

		net_buf_unref(buf);
	}
}

static void hci_rx_thread(void)
{
	while (true) {
		struct net_buf *buf;

		buf = net_buf_get(&rx_queue, K_FOREVER);
		if (bt_send(buf)) {
			LOG_ERR("Error sending to driver");
			net_buf_unref(buf);
		}
	}
}

static void bt_h4_status_cb(struct usb_cfg_data *cfg,
			    enum usb_dc_status_code status, const uint8_t *param)
{
	ARG_UNUSED(cfg);

	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_RESET:
		LOG_DBG("Device reset detected");
		suspended = false;
		configured = false;
		break;
	case USB_DC_CONFIGURED:
		LOG_DBG("Device configured");
		if (!configured) {
			configured = true;
			/* Start reading */
			bt_h4_read(bt_h4_ep_data[BT_H4_OUT_EP_IDX].ep_addr,
				   0, NULL);
		}
		break;
	case USB_DC_DISCONNECTED:
		LOG_DBG("Device disconnected");
		/* Cancel any transfer */
		usb_cancel_transfer(bt_h4_ep_data[BT_H4_IN_EP_IDX].ep_addr);
		usb_cancel_transfer(bt_h4_ep_data[BT_H4_OUT_EP_IDX].ep_addr);
		suspended = false;
		configured = false;
		break;
	case USB_DC_SUSPEND:
		suspended = true;
		break;
	case USB_DC_RESUME:
		LOG_DBG("Device resumed");
		if (suspended) {
			LOG_DBG("from suspend");
			suspended = false;
			if (configured) {
				/* Start reading */
				bt_h4_read(bt_h4_ep_data[BT_H4_OUT_EP_IDX].ep_addr,
					   0, NULL);
			}
		} else {
			LOG_DBG("Spurious resume event");
		}
		break;
	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("Unhandled status: %u", status);
		break;
	}
}

static int bt_h4_vendor_handler(struct usb_setup_packet *setup,
				int32_t *len, uint8_t **data)
{
	LOG_DBG("Class request: bRequest 0x%x bmRequestType 0x%x len %d",
		setup->bRequest, setup->bmRequestType, *len);

	if (REQTYPE_GET_RECIP(setup->bmRequestType) != REQTYPE_RECIP_DEVICE) {
		return -ENOTSUP;
	}

	if (REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_DEVICE &&
	    setup->bRequest == 0x5b) {
		LOG_DBG("Host-to-Device, data %p", *data);
		return 0;
	}

	if ((REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_HOST) &&
	    (setup->bRequest == 0x5c)) {
		LOG_DBG("Device-to-Host, wLength %d, data %p",
			setup->wLength, *data);
		return 0;
	}

	return -ENOTSUP;
}

static void bt_h4_interface_config(struct usb_desc_header *head,
				   uint8_t bInterfaceNumber)
{
	ARG_UNUSED(head);

	bt_h4_cfg.if0.bInterfaceNumber = bInterfaceNumber;
}

USBD_CFG_DATA_DEFINE(primary, hci_h4) struct usb_cfg_data bt_h4_config = {
	.usb_device_description = NULL,
	.interface_config = bt_h4_interface_config,
	.interface_descriptor = &bt_h4_cfg.if0,
	.cb_usb_status = bt_h4_status_cb,
	.interface = {
		.class_handler = NULL,
		.custom_handler = NULL,
		.vendor_handler = bt_h4_vendor_handler,
	},
	.num_endpoints = ARRAY_SIZE(bt_h4_ep_data),
	.endpoint = bt_h4_ep_data,
};

static int bt_h4_init(const struct device *dev)
{
	int ret;

	LOG_DBG("Initialization");

	ret = bt_enable_raw(&tx_queue);
	if (ret) {
		LOG_ERR("Failed to open Bluetooth raw channel: %d", ret);
		return ret;
	}

	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(rx_thread_stack),
			(k_thread_entry_t)hci_rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(8), 0, K_NO_WAIT);

	k_thread_name_set(&rx_thread_data, "usb_bt_h4_rx");

	k_thread_create(&tx_thread_data, tx_thread_stack,
			K_KERNEL_STACK_SIZEOF(tx_thread_stack),
			(k_thread_entry_t)hci_tx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(8), 0, K_NO_WAIT);

	k_thread_name_set(&tx_thread_data, "usb_bt_h4_tx");

	return 0;
}

SYS_INIT(bt_h4_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
