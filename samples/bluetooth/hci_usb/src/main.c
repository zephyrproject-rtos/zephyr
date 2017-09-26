/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <device.h>
#include <uart.h>

#include <logging/sys_log.h>
#include <misc/util.h>

#include <net/buf.h>
#include <bluetooth/buf.h>
#include <bluetooth/hci_raw.h>
#include <bluetooth/l2cap.h>

#include <usb/usb_device.h>
#include <usb/usb_common.h>

#define BTUSB_BUFFER_SIZE 64

/* Max packet size for endpoints */
#define BTUSB_BULK_EP_MPS		64

#define BTUSB_ISO1_EP_MPS		0
#define BTUSB_ISO2_EP_MPS		9
#define BTUSB_ISO3_EP_MPS		17
#define BTUSB_ISO4_EP_MPS		25
#define BTUSB_ISO5_EP_MPS		33
#define BTUSB_ISO6_EP_MPS		49

/* Max packet size for Interrupt endpoints */
#define BTUSB_INTERRUPT_EP_MPS		16

/* Max Bluetooth command data size */
#define BTUSB_CLASS_MAX_DATA_SIZE	100

#define INTEL_VENDOR_ID			0x8086
#define PRODUCT_ID			0xFF02 /* TODO: Find better ID */
#define DEVICE_RELNUM			0x0100

#define BTUSB_NUM_CONF			1

#define BTUSB_NUM_ITF			2
/* Add also alternat interfaces */
#define BTUSB_NUM_ITF_TOTAL	(BTUSB_NUM_ITF + 5)

#define BTUSB_IF1_NUM_EP		3
#define BTUSB_IF2_NUM_EP		2
/* Include all endpoints, also alternate configurations */
#define BTUSB_NUM_EP		(BTUSB_IF1_NUM_EP + BTUSB_IF2_NUM_EP * 6)

#define BTUSB_ENDP_INT			0x81
#define BTUSB_ENDP_BULK_OUT		0x02
#define BTUSB_ENDP_BULK_IN		0x82
#define BTUSB_ENDP_ISO_OUT		0x03
#define BTUSB_ENDP_ISO_IN		0x83

#define BTUSB_CONF_SIZE (USB_CONFIGURATION_DESC_SIZE + \
			 (BTUSB_NUM_ITF_TOTAL * USB_INTERFACE_DESC_SIZE) + \
			 (BTUSB_NUM_EP * USB_ENDPOINT_DESC_SIZE))

/* Misc. macros */
#define LOW_BYTE(x)	((x) & 0xFF)
#define HIGH_BYTE(x)	((x) >> 8)

static struct device *btusb_dev;

#define DEV_DATA(dev) \
	((struct btusb_dev_data_t * const)(dev)->driver_data)

static K_FIFO_DEFINE(rx_queue);

/* HCI command buffers */
#define CMD_BUF_SIZE BT_BUF_RX_SIZE
NET_BUF_POOL_DEFINE(tx_pool, CONFIG_BT_HCI_CMD_COUNT, CMD_BUF_SIZE,
		    sizeof(u8_t), NULL);

#define BT_L2CAP_MTU 64
/** Data size needed for ACL buffers */
#define BT_BUF_ACL_SIZE BT_L2CAP_BUF_SIZE(BT_L2CAP_MTU)

NET_BUF_POOL_DEFINE(acl_tx_pool, 2, BT_BUF_ACL_SIZE, sizeof(u8_t), NULL);

/* Device data structure */
struct btusb_dev_data_t {
	/* USB device status code */
	enum usb_dc_status_code usb_status;
	u8_t interface_data[BTUSB_CLASS_MAX_DATA_SIZE];
	u8_t notification_sent;
};

/**
 * Bluetooth USB descriptors configuration
 */
static const u8_t btusb_desc[] = {
	/* Device descriptor */
	USB_DEVICE_DESC_SIZE,		/* Descriptor size */
	USB_DEVICE_DESC,		/* Descriptor type */
	LOW_BYTE(USB_1_1),
	HIGH_BYTE(USB_1_1),		/* USB version in BCD format */
	WIRELESS_DEVICE_CLASS,		/* Class */
	RF_SUBCLASS,			/* SubClass - Interface specific */
	BLUETOOTH_PROTOCOL,		/* Protocol - Interface specific */
	MAX_PACKET_SIZE0,		/* Max Packet Size */
	LOW_BYTE(INTEL_VENDOR_ID),
	HIGH_BYTE(INTEL_VENDOR_ID),	/* Vendor Id */
	LOW_BYTE(PRODUCT_ID),
	HIGH_BYTE(PRODUCT_ID),		/* Product Id */
	LOW_BYTE(DEVICE_RELNUM),
	HIGH_BYTE(DEVICE_RELNUM),	/* Device Release Number */
	/* Index of Manufacturer String Descriptor */
	0x01,
	/* Index of Product String Descriptor */
	0x02,
	/* Index of Serial Number String Descriptor */
	0x03,
	BTUSB_NUM_CONF,			/* Number of Possible Configuration */

	/* Configuration descriptor */
	USB_CONFIGURATION_DESC_SIZE,	/* Descriptor size */
	USB_CONFIGURATION_DESC,		/* Descriptor type */
	/* Total length in bytes of data returned */
	LOW_BYTE(BTUSB_CONF_SIZE),
	HIGH_BYTE(BTUSB_CONF_SIZE),
	BTUSB_NUM_ITF,			/* Number of interfaces */
	0x01,				/* Configuration value */
	0x00,				/* Index of the Configuration string */
	USB_CONFIGURATION_ATTRIBUTES,	/* Attributes */
	MAX_LOW_POWER,			/* Max power consumption */

	/* Interface descriptor 0 */
	USB_INTERFACE_DESC_SIZE,	/* Descriptor size */
	USB_INTERFACE_DESC,		/* Descriptor type */
	0x00,				/* Interface index */
	0x00,				/* Alternate setting */
	BTUSB_IF1_NUM_EP,		/* Number of Endpoints */
	WIRELESS_DEVICE_CLASS,		/* Class */
	RF_SUBCLASS,			/* SubClass */
	BLUETOOTH_PROTOCOL,		/* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Endpoint INT */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_INT,			/* Endpoint address */
	USB_DC_EP_INTERRUPT,		/* Attributes */
	LOW_BYTE(BTUSB_INTERRUPT_EP_MPS),
	HIGH_BYTE(BTUSB_INTERRUPT_EP_MPS),/* Max packet size */
	0x01,				/* Interval */

	/* Endpoint OUT */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_BULK_OUT,		/* Endpoint address */
	USB_DC_EP_BULK,			/* Attributes */
	LOW_BYTE(BTUSB_BULK_EP_MPS),
	HIGH_BYTE(BTUSB_BULK_EP_MPS),	/* Max packet size */
	0x01,				/* Interval */

	/* Endpoint IN */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_BULK_IN,		/* Endpoint address */
	USB_DC_EP_BULK,			/* Attributes */
	LOW_BYTE(BTUSB_BULK_EP_MPS),
	HIGH_BYTE(BTUSB_BULK_EP_MPS),	/* Max packet size */
	0x01,				/* Interval */

	/* Interface descriptor 1:0 */
	USB_INTERFACE_DESC_SIZE,	/* Descriptor size */
	USB_INTERFACE_DESC,		/* Descriptor type */
	0x01,				/* Interface index */
	0x00,				/* Alternate setting */
	BTUSB_IF2_NUM_EP,		/* Number of Endpoints */
	WIRELESS_DEVICE_CLASS,		/* Class */
	RF_SUBCLASS,			/* SubClass */
	BLUETOOTH_PROTOCOL,		/* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Endpoint OUT */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_ISO_OUT,		/* Endpoint address */
	USB_DC_EP_ISOCHRONOUS,		/* Attributes */
	LOW_BYTE(BTUSB_ISO1_EP_MPS),
	HIGH_BYTE(BTUSB_ISO1_EP_MPS),	/* Max packet size */
	0x01,				/* Interval */

	/* Endpoint IN */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_ISO_IN,		/* Endpoint address */
	USB_DC_EP_ISOCHRONOUS,		/* Attributes */
	LOW_BYTE(BTUSB_ISO1_EP_MPS),
	HIGH_BYTE(BTUSB_ISO1_EP_MPS),	/* Max packet size */
	0x01,				/* Interval */

	/* Interface descriptor 1:1 */
	USB_INTERFACE_DESC_SIZE,	/* Descriptor size */
	USB_INTERFACE_DESC,		/* Descriptor type */
	0x01,				/* Interface index */
	0x01,				/* Alternate setting */
	BTUSB_IF2_NUM_EP,		/* Number of Endpoints */
	WIRELESS_DEVICE_CLASS,		/* Class */
	RF_SUBCLASS,			/* SubClass */
	BLUETOOTH_PROTOCOL,		/* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Endpoint OUT */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_ISO_OUT,		/* Endpoint address */
	USB_DC_EP_ISOCHRONOUS,		/* Attributes */
	LOW_BYTE(BTUSB_ISO2_EP_MPS),
	HIGH_BYTE(BTUSB_ISO2_EP_MPS),	/* Max packet size */
	0x01,				/* Interval */

	/* Endpoint IN */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_ISO_IN,		/* Endpoint address */
	USB_DC_EP_ISOCHRONOUS,		/* Attributes */
	LOW_BYTE(BTUSB_ISO2_EP_MPS),
	HIGH_BYTE(BTUSB_ISO2_EP_MPS),	/* Max packet size */
	0x01,				/* Interval */

	/* Interface descriptor 1:2 */
	USB_INTERFACE_DESC_SIZE,	/* Descriptor size */
	USB_INTERFACE_DESC,		/* Descriptor type */
	0x01,				/* Interface index */
	0x02,				/* Alternate setting */
	BTUSB_IF2_NUM_EP,		/* Number of Endpoints */
	WIRELESS_DEVICE_CLASS,		/* Class */
	RF_SUBCLASS,			/* SubClass */
	BLUETOOTH_PROTOCOL,		/* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Endpoint OUT */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_ISO_OUT,		/* Endpoint address */
	USB_DC_EP_ISOCHRONOUS,		/* Attributes */
	LOW_BYTE(BTUSB_ISO3_EP_MPS),
	HIGH_BYTE(BTUSB_ISO3_EP_MPS),	/* Max packet size */
	0x01,				/* Interval */

	/* Endpoint IN */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_ISO_IN,		/* Endpoint address */
	USB_DC_EP_ISOCHRONOUS,		/* Attributes */
	LOW_BYTE(BTUSB_ISO3_EP_MPS),
	HIGH_BYTE(BTUSB_ISO3_EP_MPS),	/* Max packet size */
	0x01,				/* Interval */

	/* Interface descriptor 1:3 */
	USB_INTERFACE_DESC_SIZE,	/* Descriptor size */
	USB_INTERFACE_DESC,		/* Descriptor type */
	0x01,				/* Interface index */
	0x03,				/* Alternate setting */
	BTUSB_IF2_NUM_EP,		/* Number of Endpoints */
	WIRELESS_DEVICE_CLASS,		/* Class */
	RF_SUBCLASS,			/* SubClass */
	BLUETOOTH_PROTOCOL,		/* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Endpoint OUT */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_ISO_OUT,		/* Endpoint address */
	USB_DC_EP_ISOCHRONOUS,		/* Attributes */
	LOW_BYTE(BTUSB_ISO4_EP_MPS),
	HIGH_BYTE(BTUSB_ISO4_EP_MPS),	/* Max packet size */
	0x01,				/* Interval */

	/* Endpoint IN */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_ISO_IN,		/* Endpoint address */
	USB_DC_EP_ISOCHRONOUS,		/* Attributes */
	LOW_BYTE(BTUSB_ISO4_EP_MPS),
	HIGH_BYTE(BTUSB_ISO4_EP_MPS),	/* Max packet size */
	0x01,				/* Interval */

	/* Interface descriptor 1:4 */
	USB_INTERFACE_DESC_SIZE,	/* Descriptor size */
	USB_INTERFACE_DESC,		/* Descriptor type */
	0x01,				/* Interface index */
	0x04,				/* Alternate setting */
	BTUSB_IF2_NUM_EP,		/* Number of Endpoints */
	WIRELESS_DEVICE_CLASS,		/* Class */
	RF_SUBCLASS,			/* SubClass */
	BLUETOOTH_PROTOCOL,		/* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Endpoint OUT */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_ISO_OUT,		/* Endpoint address */
	USB_DC_EP_ISOCHRONOUS,		/* Attributes */
	LOW_BYTE(BTUSB_ISO5_EP_MPS),
	HIGH_BYTE(BTUSB_ISO5_EP_MPS),	/* Max packet size */
	0x01,				/* Interval */

	/* Endpoint IN */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_ISO_IN,		/* Endpoint address */
	USB_DC_EP_ISOCHRONOUS,		/* Attributes */
	LOW_BYTE(BTUSB_ISO5_EP_MPS),
	HIGH_BYTE(BTUSB_ISO5_EP_MPS),	/* Max packet size */
	0x01,				/* Interval */

	/* Interface descriptor 1:5 */
	USB_INTERFACE_DESC_SIZE,	/* Descriptor size */
	USB_INTERFACE_DESC,		/* Descriptor type */
	0x01,				/* Interface index */
	0x05,				/* Alternate setting */
	BTUSB_IF2_NUM_EP,		/* Number of Endpoints */
	WIRELESS_DEVICE_CLASS,		/* Class */
	RF_SUBCLASS,			/* SubClass */
	BLUETOOTH_PROTOCOL,		/* Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* Endpoint OUT */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_ISO_OUT,		/* Endpoint address */
	USB_DC_EP_ISOCHRONOUS,		/* Attributes */
	LOW_BYTE(BTUSB_ISO6_EP_MPS),
	HIGH_BYTE(BTUSB_ISO6_EP_MPS),	/* Max packet size */
	0x01,				/* Interval */

	/* Endpoint IN */
	USB_ENDPOINT_DESC_SIZE,		/* Descriptor size */
	USB_ENDPOINT_DESC,		/* Descriptor type */
	BTUSB_ENDP_ISO_IN,		/* Endpoint address */
	USB_DC_EP_ISOCHRONOUS,		/* Attributes */
	LOW_BYTE(BTUSB_ISO6_EP_MPS),
	HIGH_BYTE(BTUSB_ISO6_EP_MPS),	/* Max packet size */
	0x01,				/* Interval */

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
	'Q', 0, 'U', 0, 'A', 0, 'R', 0, 'Q', 0, 'B', 0, 'T', 0,

	/* Serial Number String Descriptor "00.01" */
	0x0C,
	USB_STRING_DESC,
	'0', 0, '0', 0, '.', 0, '0', 0, '1', 0,
};

/* TODO: move to standard utils */
static void hexdump(const char *str, const u8_t *packet, size_t length)
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

static void btusb_int_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	SYS_LOG_DBG("ep %x status %d", ep, ep_status);
}

/* EP Bulk OUT handler, used to read the data received from the Host */
static void btusb_bulk_out(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	struct net_buf *buf;
	u32_t len, read = 0;
	u8_t tmp[4];

	SYS_LOG_DBG("ep %x status %d", ep, ep_status);

	/* Read number of butes to read */
	usb_read(ep, NULL, 0, &len);

	if (!len || len > BT_BUF_ACL_SIZE) {
		SYS_LOG_ERR("Incorrect length: %u\n", len);
		return;
	}

	buf = net_buf_alloc(&acl_tx_pool, K_NO_WAIT);
	if (!buf) {
		SYS_LOG_ERR("Cannot get free buffer\n");
		return;
	}

	/**
	 * Quark SE USB controller is always storing data
	 * in the FIFOs per 32-bit words.
	 */
	while (len > buf->len) {
		usb_read(ep, tmp, 4, NULL);
		read += 4;

		if (len > read) {
			net_buf_add_mem(buf, tmp, 4);
		} else {
			u8_t remains = 4 - (read - len);

			net_buf_add_mem(buf, tmp, remains);
		}
	}

	hexdump(">", buf->data, buf->len);

	bt_buf_set_type(buf, BT_BUF_ACL_OUT);

	bt_send(buf);
}

/* EP Bulk IN handler, used to send data to the Host */
static void btusb_bulk_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	SYS_LOG_ERR("Not implemented");
}

/* EP ISO OUT handler, used to read the data received from the Host */
static void btusb_iso_out(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	SYS_LOG_ERR("Not implemented");
}

/* EP ISO IN handler, used to send data to the Host */
static void btusb_iso_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	SYS_LOG_ERR("Not implemented");
}

/* Describe EndPoints configuration */
static struct usb_ep_cfg_data btusb_ep[] = {
	{
		.ep_cb	= btusb_int_in,
		.ep_addr = BTUSB_ENDP_INT
	},
	{
		.ep_cb	= btusb_bulk_out,
		.ep_addr = BTUSB_ENDP_BULK_OUT
	},
	{
		.ep_cb = btusb_bulk_in,
		.ep_addr = BTUSB_ENDP_BULK_IN
	},
	{
		.ep_cb = btusb_iso_out,
		.ep_addr = BTUSB_ENDP_ISO_OUT
	},
	{
		.ep_cb = btusb_iso_in,
		.ep_addr = BTUSB_ENDP_ISO_IN
	},
#if 0
	/* FIXME: check endpoints */
	/* Alt 1 */
	{
		.ep_cb = btusb_iso_out,
		.ep_addr = BTUSB_ENDP_ISO_OUT
	},
	{
		.ep_cb = btusb_iso_in,
		.ep_addr = BTUSB_ENDP_ISO_IN
	},
	/* Alt 2 */
	{
		.ep_cb = btusb_iso_out,
		.ep_addr = BTUSB_ENDP_ISO_OUT
	},
	{
		.ep_cb = btusb_iso_in,
		.ep_addr = BTUSB_ENDP_ISO_IN
	},
	/* Alt 3 */
	{
		.ep_cb = btusb_iso_out,
		.ep_addr = BTUSB_ENDP_ISO_OUT
	},
	{
		.ep_cb = btusb_iso_in,
		.ep_addr = BTUSB_ENDP_ISO_IN
	},
	/* Alt 4 */
	{
		.ep_cb = btusb_iso_out,
		.ep_addr = BTUSB_ENDP_ISO_OUT
	},
	{
		.ep_cb = btusb_iso_in,
		.ep_addr = BTUSB_ENDP_ISO_IN
	},
	/* Alt 5 */
	{
		.ep_cb = btusb_iso_out,
		.ep_addr = BTUSB_ENDP_ISO_OUT
	},
	{
		.ep_cb = btusb_iso_in,
		.ep_addr = BTUSB_ENDP_ISO_IN
	},
#endif
};

static void btusb_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	struct btusb_dev_data_t * const dev_data = DEV_DATA(btusb_dev);

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
		SYS_LOG_DBG("USB device supended");
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

static int btusb_class_handler(struct usb_setup_packet *setup,
			       s32_t *len, u8_t **data)
{
	struct net_buf *buf;

	if (!*len || *len > CMD_BUF_SIZE) {
		SYS_LOG_ERR("Incorrect length: %d\n", *len);
		return -EINVAL;
	}

	hexdump(">", *data, *len);

	buf = net_buf_alloc(&tx_pool, K_NO_WAIT);
	if (!buf) {
		SYS_LOG_ERR("Cannot get free buffer\n");
		return -ENOMEM;
	}

	bt_buf_set_type(buf, BT_BUF_CMD);

	net_buf_add_mem(buf, *data, *len);

	bt_send(buf);

	return 0;
}

static struct usb_cfg_data btusb_config = {
	.usb_device_description = btusb_desc,
	.cb_usb_status = btusb_status_cb,
	.interface = {
		.class_handler = btusb_class_handler,
		.custom_handler = NULL,
	},
	.num_endpoints = ARRAY_SIZE(btusb_ep),
	.endpoint = btusb_ep,
};

static int btusb_init(struct device *dev)
{
	struct btusb_dev_data_t * const dev_data = DEV_DATA(dev);
	int ret;

	btusb_config.interface.payload_data = dev_data->interface_data;
	btusb_dev = dev;

	/* Initialize the USB driver with the right configuration */
	ret = usb_set_config(&btusb_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to config USB");
		return ret;
	}

	/* Enable USB driver */
	ret = usb_enable(&btusb_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to enable USB");
		return ret;
	}

	return 0;
}

static struct btusb_dev_data_t btusb_dev_data = {
	.usb_status = USB_DC_UNKNOWN,
};

DEVICE_INIT(btusb, "btusb", &btusb_init,
	    &btusb_dev_data, NULL,
	    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

static int try_write(u8_t ep, struct net_buf *buf)
{
	while (1) {
		int ret = usb_write(ep, buf->data, buf->len, NULL);

		switch (ret) {
		case -EAGAIN:
			break;
		/* TODO: Handle other error codes */
		default:
			return ret;
		}
	}
}

void main(void)
{
	SYS_LOG_DBG("Start");

	bt_enable_raw(&rx_queue);

	while (1) {
		struct net_buf *buf;

		buf = net_buf_get(&rx_queue, K_FOREVER);

		hexdump("<", buf->data, buf->len);

		switch (bt_buf_get_type(buf)) {
		case BT_BUF_EVT:
			try_write(BTUSB_ENDP_INT, buf);
			break;
		case BT_BUF_ACL_IN:
			try_write(BTUSB_ENDP_BULK_IN, buf);
			break;
		default:
			SYS_LOG_ERR("Unknown type %u", bt_buf_get_type(buf));
			break;
		}

		net_buf_unref(buf);
	}
}
