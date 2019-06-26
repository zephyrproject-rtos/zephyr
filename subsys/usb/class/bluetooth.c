/*
 * Wireless / Bluetooth USB class
 *
 * Copyright (c) 2018 Intel Corporation
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
LOG_MODULE_REGISTER(usb_bluetooth);

static K_FIFO_DEFINE(rx_queue);
static K_FIFO_DEFINE(tx_queue);

/* HCI command buffers */
#define CMD_BUF_SIZE BT_BUF_RX_SIZE
NET_BUF_POOL_DEFINE(tx_pool, CONFIG_BT_HCI_CMD_COUNT, CMD_BUF_SIZE,
		    sizeof(u8_t), NULL);

/* ACL data TX buffers */
#if defined(CONFIG_BT_CTLR_TX_BUFFERS)
#define ACL_BUF_COUNT CONFIG_BT_CTLR_TX_BUFFERS
#else
#define ACL_BUF_COUNT 4
#endif

#if defined(CONFIG_BT_CTLR_TX_BUFFER_SIZE)
#define BT_L2CAP_MTU (CONFIG_BT_CTLR_TX_BUFFER_SIZE - BT_L2CAP_HDR_SIZE)
#else
#define BT_L2CAP_MTU 64
#endif

/* Data size needed for ACL buffers */
#define BT_BUF_ACL_SIZE BT_L2CAP_BUF_SIZE(BT_L2CAP_MTU)
NET_BUF_POOL_DEFINE(acl_tx_pool, ACL_BUF_COUNT, BT_BUF_ACL_SIZE,
		    sizeof(u8_t), NULL);

#define BLUETOOTH_INT_EP_ADDR		0x81
#define BLUETOOTH_OUT_EP_ADDR		0x02
#define BLUETOOTH_IN_EP_ADDR		0x82

/* HCI RX/TX threads */
static K_THREAD_STACK_DEFINE(rx_thread_stack, 512);
static struct k_thread rx_thread_data;
static K_THREAD_STACK_DEFINE(tx_thread_stack, 512);
static struct k_thread tx_thread_data;

struct usb_bluetooth_config {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_int_ep;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
} __packed;

USBD_CLASS_DESCR_DEFINE(primary, 0)
	struct usb_bluetooth_config bluetooth_cfg = {
	/* Interface descriptor 0 */
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 3,
		.bInterfaceClass = WIRELESS_DEVICE_CLASS,
		.bInterfaceSubClass = RF_SUBCLASS,
		.bInterfaceProtocol = BLUETOOTH_PROTOCOL,
		.iInterface = 0,
	},

	/* Interrupt Endpoint */
	.if0_int_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = BLUETOOTH_INT_EP_ADDR,
		.bmAttributes = USB_DC_EP_INTERRUPT,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_BLUETOOTH_INT_EP_MPS),
		.bInterval = 0x01,
	},

	/* Data Endpoint OUT */
	.if0_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = BLUETOOTH_OUT_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_BLUETOOTH_BULK_EP_MPS),
		.bInterval = 0x01,
	},

	/* Data Endpoint IN */
	.if0_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = BLUETOOTH_IN_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_BLUETOOTH_BULK_EP_MPS),
		.bInterval = 0x01,
	},
};

#define HCI_INT_EP_IDX			0
#define HCI_OUT_EP_IDX			1
#define HCI_IN_EP_IDX			2

static struct usb_ep_cfg_data bluetooth_ep_data[] = {
	{
		.ep_cb = usb_transfer_ep_callback,
		.ep_addr = BLUETOOTH_INT_EP_ADDR,
	},
	{
		.ep_cb = usb_transfer_ep_callback,
		.ep_addr = BLUETOOTH_OUT_EP_ADDR,
	},
	{
		.ep_cb = usb_transfer_ep_callback,
		.ep_addr = BLUETOOTH_IN_EP_ADDR,
	},
};

static void hci_rx_thread(void)
{
	LOG_DBG("Start USB Bluetooth thread");

	while (true) {
		struct net_buf *buf;

		buf = net_buf_get(&rx_queue, K_FOREVER);

		switch (bt_buf_get_type(buf)) {
		case BT_BUF_EVT:
			usb_transfer_sync(
				bluetooth_ep_data[HCI_INT_EP_IDX].ep_addr,
				buf->data, buf->len,
				USB_TRANS_WRITE);
			break;
		case BT_BUF_ACL_IN:
			usb_transfer_sync(
				bluetooth_ep_data[HCI_IN_EP_IDX].ep_addr,
				buf->data, buf->len,
				USB_TRANS_WRITE);
			break;
		default:
			LOG_ERR("Unknown type %u", bt_buf_get_type(buf));
			break;
		}

		net_buf_unref(buf);
	}
}

static void hci_tx_thread(void)
{
	while (true) {
		struct net_buf *buf;

		buf = net_buf_get(&tx_queue, K_FOREVER);

		if (bt_send(buf)) {
			LOG_ERR("Error sending to driver");
			net_buf_unref(buf);
		}
	}
}

static void acl_read_cb(u8_t ep, int size, void *priv)
{
	struct net_buf *buf = priv;

	if (size > 0) {
		buf->len += size;
		bt_buf_set_type(buf, BT_BUF_ACL_OUT);
		net_buf_put(&tx_queue, buf);
		buf = NULL;
	}

	if (buf) {
		net_buf_unref(buf);
	}

	buf = net_buf_alloc(&acl_tx_pool, K_FOREVER);
	__ASSERT_NO_MSG(buf);

	net_buf_reserve(buf, CONFIG_BT_HCI_RESERVE);

	/* Start a new read transfer */
	usb_transfer(bluetooth_ep_data[HCI_OUT_EP_IDX].ep_addr, buf->data,
		     BT_BUF_ACL_SIZE, USB_TRANS_READ, acl_read_cb, buf);
}

static void bluetooth_status_cb(struct usb_cfg_data *cfg,
				enum usb_dc_status_code status,
				const u8_t *param)
{
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
		/* Start reading */
		acl_read_cb(bluetooth_ep_data[HCI_OUT_EP_IDX].ep_addr, 0, NULL);
		break;
	case USB_DC_DISCONNECTED:
		LOG_DBG("USB device disconnected");
		/* Cancel any transfer */
		usb_cancel_transfer(bluetooth_ep_data[HCI_INT_EP_IDX].ep_addr);
		usb_cancel_transfer(bluetooth_ep_data[HCI_IN_EP_IDX].ep_addr);
		usb_cancel_transfer(bluetooth_ep_data[HCI_OUT_EP_IDX].ep_addr);
		break;
	case USB_DC_SUSPEND:
		LOG_DBG("USB device suspended");
		break;
	case USB_DC_RESUME:
		LOG_DBG("USB device resumed");
		break;
	case USB_DC_SOF:
		break;
	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("USB unknown state");
		break;
	}
}

static int bluetooth_class_handler(struct usb_setup_packet *setup,
				   s32_t *len, u8_t **data)
{
	struct net_buf *buf;

	LOG_DBG("len %u", *len);

	if (!*len || *len > CMD_BUF_SIZE) {
		LOG_ERR("Incorrect length: %d\n", *len);
		return -EINVAL;
	}

	buf = net_buf_alloc(&tx_pool, K_NO_WAIT);
	if (!buf) {
		LOG_ERR("Cannot get free buffer\n");
		return -ENOMEM;
	}

	net_buf_reserve(buf, CONFIG_BT_HCI_RESERVE);
	bt_buf_set_type(buf, BT_BUF_CMD);

	net_buf_add_mem(buf, *data, *len);

	net_buf_put(&tx_queue, buf);

	return 0;
}

static void bluetooth_interface_config(struct usb_desc_header *head,
				       u8_t bInterfaceNumber)
{
	ARG_UNUSED(head);

	bluetooth_cfg.if0.bInterfaceNumber = bInterfaceNumber;
}

USBD_CFG_DATA_DEFINE(primary, hci) struct usb_cfg_data bluetooth_config = {
	.usb_device_description = NULL,
	.interface_config = bluetooth_interface_config,
	.interface_descriptor = &bluetooth_cfg.if0,
	.cb_usb_status = bluetooth_status_cb,
	.interface = {
		.class_handler = bluetooth_class_handler,
		.custom_handler = NULL,
		.vendor_handler = NULL,
	},
	.num_endpoints = ARRAY_SIZE(bluetooth_ep_data),
	.endpoint = bluetooth_ep_data,
};

static int bluetooth_init(struct device *dev)
{
	int ret;

	LOG_DBG("Initialization");

	ret = bt_enable_raw(&rx_queue);
	if (ret) {
		LOG_ERR("Failed to open Bluetooth raw channel: %d", ret);
		return ret;
	}

	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_THREAD_STACK_SIZEOF(rx_thread_stack),
			(k_thread_entry_t)hci_rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(8), 0, K_NO_WAIT);

	k_thread_create(&tx_thread_data, tx_thread_stack,
			K_THREAD_STACK_SIZEOF(tx_thread_stack),
			(k_thread_entry_t)hci_tx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(8), 0, K_NO_WAIT);

	return 0;
}

SYS_INIT(bluetooth_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
