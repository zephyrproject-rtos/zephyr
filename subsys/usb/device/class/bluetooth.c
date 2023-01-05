/*
 * Wireless / Bluetooth USB class
 *
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/usb/usb_device.h>
#include <usb_descriptor.h>

#include <zephyr/net/buf.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_raw.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_bluetooth, CONFIG_USB_DEVICE_LOG_LEVEL);

#define USB_RF_SUBCLASS			0x01
#define USB_BLUETOOTH_PROTOCOL		0x01

static K_FIFO_DEFINE(rx_queue);
static K_FIFO_DEFINE(tx_queue);

#define BLUETOOTH_INT_EP_ADDR		0x81
#define BLUETOOTH_OUT_EP_ADDR		0x02
#define BLUETOOTH_IN_EP_ADDR		0x82

/* HCI RX/TX threads */
static K_KERNEL_STACK_DEFINE(rx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static struct k_thread rx_thread_data;
static K_KERNEL_STACK_DEFINE(tx_thread_stack, 512);
static struct k_thread tx_thread_data;

/* HCI USB state flags */
static bool configured;
/*
 * Shared variable between bluetooth_status_cb() and hci_tx_thread(),
 * where hci_tx_thread() has read-only access to it.
 */
static atomic_t suspended;

static uint8_t ep_out_buf[USB_MAX_FS_BULK_MPS];

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
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 3,
		.bInterfaceClass = USB_BCC_WIRELESS_CONTROLLER,
		.bInterfaceSubClass = USB_RF_SUBCLASS,
		.bInterfaceProtocol = USB_BLUETOOTH_PROTOCOL,
		.iInterface = 0,
	},

	/* Interrupt Endpoint */
	.if0_int_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = BLUETOOTH_INT_EP_ADDR,
		.bmAttributes = USB_DC_EP_INTERRUPT,
		.wMaxPacketSize = sys_cpu_to_le16(USB_MAX_FS_INT_MPS),
		.bInterval = 0x01,
	},

	/* Data Endpoint OUT */
	.if0_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = BLUETOOTH_OUT_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(USB_MAX_FS_BULK_MPS),
		.bInterval = 0x01,
	},

	/* Data Endpoint IN */
	.if0_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = BLUETOOTH_IN_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(USB_MAX_FS_BULK_MPS),
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

static void hci_tx_thread(void)
{
	LOG_DBG("Start USB Bluetooth thread");

	while (true) {
		struct net_buf *buf;

		buf = net_buf_get(&tx_queue, K_FOREVER);

		if (IS_ENABLED(CONFIG_USB_DEVICE_BLUETOOTH_VS_H4) &&
		    bt_hci_raw_get_mode() == BT_HCI_RAW_MODE_H4) {
			/* Force to sent over bulk if H4 is selected */
			bt_buf_set_type(buf, BT_BUF_ACL_IN);
		}

		if (atomic_get(&suspended)) {
			if (usb_wakeup_request()) {
				LOG_DBG("Remote wakeup not enabled/supported");
			}
			/*
			 * Let's wait until operation is resumed.
			 * This is independent of usb_wakeup_request() result,
			 * as long as device is suspended it should not start
			 * any transfers.
			 */
			while (atomic_get(&suspended)) {
				k_sleep(K_MSEC(1));
			}
		}

		switch (bt_buf_get_type(buf)) {
		case BT_BUF_EVT:
			usb_transfer_sync(
				bluetooth_ep_data[HCI_INT_EP_IDX].ep_addr,
				buf->data, buf->len,
				USB_TRANS_WRITE | USB_TRANS_NO_ZLP);
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

static void hci_rx_thread(void)
{
	while (true) {
		struct net_buf *buf;
		int err;

		buf = net_buf_get(&rx_queue, K_FOREVER);

		err = bt_send(buf);
		if (err) {
			LOG_ERR("Error sending to driver");
			net_buf_unref(buf);
		}
	}
}

static uint16_t hci_pkt_get_len(struct net_buf *buf,
				const uint8_t *data, size_t size)
{
	uint16_t len = 0;
	size_t hdr_len = 0;

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_CMD: {
		struct bt_hci_cmd_hdr *cmd_hdr;

		hdr_len = sizeof(*cmd_hdr);
		cmd_hdr = (struct bt_hci_cmd_hdr *)data;
		len = cmd_hdr->param_len + hdr_len;
		break;
	}
	case BT_BUF_ACL_OUT: {
		struct bt_hci_acl_hdr *acl_hdr;

		hdr_len = sizeof(*acl_hdr);
		acl_hdr = (struct bt_hci_acl_hdr *)data;
		len = sys_le16_to_cpu(acl_hdr->len) + hdr_len;
		break;
	}
	case BT_BUF_ISO_OUT: {
		struct bt_hci_iso_hdr *iso_hdr;

		hdr_len = sizeof(*iso_hdr);
		iso_hdr = (struct bt_hci_iso_hdr *)data;
		len = bt_iso_hdr_len(sys_le16_to_cpu(iso_hdr->len)) + hdr_len;
		break;
	}
	default:
		LOG_ERR("Unknown bt buffer type");
		return 0;
	}

	return (size < hdr_len) ? 0 : len;
}

static void acl_read_cb(uint8_t ep, int size, void *priv)
{
	static struct net_buf *buf;
	static uint16_t pkt_len;
	uint8_t *data = ep_out_buf;

	if (size == 0) {
		goto restart_out_transfer;
	}

	if (buf == NULL) {
		/*
		 * Obtain the first chunk and determine the length
		 * of the HCI packet.
		 */
		if (IS_ENABLED(CONFIG_USB_DEVICE_BLUETOOTH_VS_H4) &&
		    bt_hci_raw_get_mode() == BT_HCI_RAW_MODE_H4) {
			buf = bt_buf_get_tx(BT_BUF_H4, K_FOREVER, data, size);
			if (!buf) {
				LOG_ERR("Failed to allocate buffer");
				goto restart_out_transfer;
			}

			pkt_len = hci_pkt_get_len(buf, &data[1], size - 1);
			LOG_DBG("pkt_len %u, chunk %u", pkt_len, size);
		} else {
			buf = bt_buf_get_tx(BT_BUF_ACL_OUT, K_FOREVER,
					    data, size);
			if (!buf) {
				LOG_ERR("Failed to allocate buffer");
				goto restart_out_transfer;
			}

			pkt_len = hci_pkt_get_len(buf, data, size);
			LOG_DBG("pkt_len %u, chunk %u", pkt_len, size);
		}

		if (pkt_len == 0) {
			LOG_ERR("Failed to get packet length");
			net_buf_unref(buf);
			buf = NULL;
		}
	} else {
		if (net_buf_tailroom(buf) < size) {
			LOG_ERR("Buffer tailroom too small");
			net_buf_unref(buf);
			buf = NULL;
			goto restart_out_transfer;
		}

		/*
		 * Take over the next chunk if HCI packet is
		 * larger than USB_MAX_FS_BULK_MPS.
		 */
		net_buf_add_mem(buf, data, size);
		LOG_DBG("len %u, chunk %u", buf->len, size);
	}

	if (buf != NULL && pkt_len == buf->len) {
		net_buf_put(&rx_queue, buf);
		LOG_DBG("put");
		buf = NULL;
		pkt_len = 0;
	}

restart_out_transfer:
	usb_transfer(bluetooth_ep_data[HCI_OUT_EP_IDX].ep_addr, ep_out_buf,
		     sizeof(ep_out_buf), USB_TRANS_READ,
		     acl_read_cb, NULL);
}

static void bluetooth_status_cb(struct usb_cfg_data *cfg,
				enum usb_dc_status_code status,
				const uint8_t *param)
{
	ARG_UNUSED(cfg);
	atomic_val_t tmp;

	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_RESET:
		LOG_DBG("Device reset detected");
		configured = false;
		atomic_clear(&suspended);
		break;
	case USB_DC_CONFIGURED:
		LOG_DBG("Device configured");
		if (!configured) {
			configured = true;
			/* Start reading */
			acl_read_cb(bluetooth_ep_data[HCI_OUT_EP_IDX].ep_addr,
				    0, NULL);
		}
		break;
	case USB_DC_DISCONNECTED:
		LOG_DBG("Device disconnected");
		/* Cancel any transfer */
		usb_cancel_transfer(bluetooth_ep_data[HCI_INT_EP_IDX].ep_addr);
		usb_cancel_transfer(bluetooth_ep_data[HCI_IN_EP_IDX].ep_addr);
		usb_cancel_transfer(bluetooth_ep_data[HCI_OUT_EP_IDX].ep_addr);
		configured = false;
		atomic_clear(&suspended);
		break;
	case USB_DC_SUSPEND:
		LOG_DBG("Device suspended");
		atomic_set(&suspended, 1);
		break;
	case USB_DC_RESUME:
		tmp = atomic_clear(&suspended);
		if (tmp) {
			LOG_DBG("Device resumed from suspend");
		} else {
			LOG_DBG("Spurious resume event");
		}
		break;
	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("Unknown state");
		break;
	}
}

static uint8_t vs_read_usb_transport_mode(struct net_buf *buf)
{
	struct net_buf *rsp;
	struct bt_hci_rp_vs_read_usb_transport_mode *rp;

	rsp = bt_hci_cmd_complete_create(BT_HCI_OP_VS_READ_USB_TRANSPORT_MODE,
					 sizeof(*rp) + 2);
	rp = net_buf_add(rsp, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->num_supported_modes = 2;

	net_buf_add_u8(rsp, BT_HCI_VS_USB_H2_MODE);
	net_buf_add_u8(rsp, BT_HCI_VS_USB_H4_MODE);

	net_buf_put(&tx_queue, rsp);

	return BT_HCI_ERR_EXT_HANDLED;
}

static uint8_t vs_set_usb_transport_mode(struct net_buf *buf)
{
	struct bt_hci_cp_vs_set_usb_transport_mode *cp;
	uint8_t mode;

	cp = net_buf_pull_mem(buf, sizeof(*cp));

	switch (cp->mode) {
	case BT_HCI_VS_USB_H2_MODE:
		mode = BT_HCI_RAW_MODE_PASSTHROUGH;
		break;
	case BT_HCI_VS_USB_H4_MODE:
		mode = BT_HCI_RAW_MODE_H4;
		break;
	default:
		LOG_DBG("Invalid mode: %u", cp->mode);
		return BT_HCI_ERR_INVALID_PARAM;
	}

	LOG_DBG("mode %u", mode);

	bt_hci_raw_set_mode(mode);

	return BT_HCI_ERR_SUCCESS;
}

static struct bt_hci_raw_cmd_ext cmd_ext[] = {
	BT_HCI_RAW_CMD_EXT(BT_OCF(BT_HCI_OP_VS_READ_USB_TRANSPORT_MODE), 0,
			   vs_read_usb_transport_mode),
	BT_HCI_RAW_CMD_EXT(BT_OCF(BT_HCI_OP_VS_SET_USB_TRANSPORT_MODE),
			   sizeof(struct bt_hci_cp_vs_set_usb_transport_mode),
			   vs_set_usb_transport_mode),
};

static int bluetooth_class_handler(struct usb_setup_packet *setup,
				   int32_t *len, uint8_t **data)
{
	struct net_buf *buf;

	if (usb_reqtype_is_to_host(setup) ||
	    setup->RequestType.type != USB_REQTYPE_TYPE_CLASS) {
		return -ENOTSUP;
	}

	LOG_DBG("len %u", *len);

	buf = bt_buf_get_tx(BT_BUF_CMD, K_NO_WAIT, *data, *len);
	if (!buf) {
		LOG_ERR("Cannot get free buffer\n");
		return -ENOMEM;
	}

	net_buf_put(&rx_queue, buf);

	return 0;
}

static void bluetooth_interface_config(struct usb_desc_header *head,
				       uint8_t bInterfaceNumber)
{
	ARG_UNUSED(head);

	bluetooth_cfg.if0.bInterfaceNumber = bInterfaceNumber;
}

USBD_DEFINE_CFG_DATA(bluetooth_config) = {
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

static int bluetooth_init(const struct device *dev)
{
	int ret;

	LOG_DBG("Initialization");

	ret = bt_enable_raw(&tx_queue);
	if (ret) {
		LOG_ERR("Failed to open Bluetooth raw channel: %d", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_USB_DEVICE_BLUETOOTH_VS_H4)) {
		bt_hci_raw_cmd_ext_register(cmd_ext, ARRAY_SIZE(cmd_ext));
	}

	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(rx_thread_stack),
			(k_thread_entry_t)hci_rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(8), 0, K_NO_WAIT);

	k_thread_name_set(&rx_thread_data, "usb_bt_rx");

	k_thread_create(&tx_thread_data, tx_thread_stack,
			K_KERNEL_STACK_SIZEOF(tx_thread_stack),
			(k_thread_entry_t)hci_tx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(8), 0, K_NO_WAIT);

	k_thread_name_set(&tx_thread_data, "usb_bt_tx");

	return 0;
}

SYS_INIT(bluetooth_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
