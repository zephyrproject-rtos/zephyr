/*
 * Copyright (c) 2015-2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief WebUSB enabled custom class driver
 *
 * This is a modified version of CDC ACM class driver
 * to support the WebUSB.
 */

#define LOG_LEVEL CONFIG_USB_DEVICE_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(webusb);

#include <misc/byteorder.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>

#include "webusb.h"

/* Max packet size for Bulk endpoints */
#define CDC_BULK_EP_MPS			64
/* Max CDC ACM class request max data size */
#define CDC_CLASS_REQ_MAX_DATA_SIZE	8
/* Number of interfaces */
#define WEBUSB_NUM_ITF			0x01
/* Number of Endpoints in the custom interface */
#define WEBUSB_NUM_EP			0x02
#define WEBUSB_ENDP_OUT			0x02
#define WEBUSB_ENDP_IN			0x83

static struct webusb_req_handlers *req_handlers;

u8_t interface_data[CDC_CLASS_REQ_MAX_DATA_SIZE];

u8_t rx_buf[64];

/* Structure representing the global USB description */
struct dev_common_descriptor {
	struct usb_device_descriptor device_descriptor;
	struct usb_cfg_descriptor cfg_descr;
	struct usb_cdc_acm_config {
		struct usb_if_descriptor if0;
		struct usb_ep_descriptor if0_in_ep;
		struct usb_ep_descriptor if0_out_ep;
	} __packed cdc_acm_cfg;
	struct usb_string_desription {
		struct usb_string_descriptor lang_descr;
		struct usb_mfr_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[0x0C - 2];
		} __packed utf16le_mfr;

		struct usb_product_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[0x0E - 2];
		} __packed utf16le_product;

		struct usb_sn_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[0x0C - 2];
		} __packed utf16le_sn;
	} __packed string_descr;
	struct usb_desc_header term_descr;
} __packed;

static struct dev_common_descriptor webusb_usb_description = {
	/* Device descriptor */
	.device_descriptor = {
		.bLength = sizeof(struct usb_device_descriptor),
		.bDescriptorType = USB_DEVICE_DESC,
		.bcdUSB = sys_cpu_to_le16(USB_2_1),
		.bDeviceClass = 0,
		.bDeviceSubClass = 0,
		.bDeviceProtocol = 0,
		.bMaxPacketSize0 = USB_MAX_CTRL_MPS,
		.idVendor = sys_cpu_to_le16((u16_t)CONFIG_USB_DEVICE_VID),
		.idProduct = sys_cpu_to_le16((u16_t)CONFIG_USB_DEVICE_PID),
		.bcdDevice = sys_cpu_to_le16(BCDDEVICE_RELNUM),
		.iManufacturer = 1,
		.iProduct = 2,
		.iSerialNumber = 3,
		.bNumConfigurations = 1,
	},
	/* Configuration descriptor */
	.cfg_descr = {
		.bLength = sizeof(struct usb_cfg_descriptor),
		.bDescriptorType = USB_CONFIGURATION_DESC,
		.wTotalLength = sizeof(struct dev_common_descriptor)
			      - sizeof(struct usb_device_descriptor)
			      - sizeof(struct usb_string_desription)
			      - sizeof(struct usb_desc_header),
		.bNumInterfaces = WEBUSB_NUM_ITF,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_CONFIGURATION_ATTRIBUTES,
		.bMaxPower = MAX_LOW_POWER,
	},
	.cdc_acm_cfg = {
		/* Interface descriptor */
		.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 0,
			.bNumEndpoints = WEBUSB_NUM_EP,
			.bInterfaceClass = CUSTOM_CLASS,
			.bInterfaceSubClass = 0,
			.bInterfaceProtocol = 0,
			.iInterface = 0,
		},
		/* First Endpoint IN */
		.if0_in_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = WEBUSB_ENDP_IN,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize = sys_cpu_to_le16(CDC_BULK_EP_MPS),
			.bInterval = 0x00,
		},
		/* Second Endpoint OUT */
		.if0_out_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = WEBUSB_ENDP_OUT,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize = sys_cpu_to_le16(CDC_BULK_EP_MPS),
			.bInterval = 0x00,
		},
	},
	.string_descr = {
		.lang_descr = {
			.bLength = sizeof(struct usb_string_descriptor),
			.bDescriptorType = USB_STRING_DESC,
			.bString = sys_cpu_to_le16(0x0409),
		},
		/* Manufacturer String Descriptor */
		.utf16le_mfr = {
			.bLength = 0x0C,
			.bDescriptorType = USB_STRING_DESC,
			.bString = {'I', 0, 'n', 0, 't', 0, 'e', 0, 'l', 0,},
		},
		/* Product String Descriptor */
		.utf16le_product = {
			.bLength = 0x0E,
			.bDescriptorType = USB_STRING_DESC,
			.bString = {'W', 0, 'e', 0, 'b', 0, 'U', 0, 'S', 0,
				    'B', 0,},
		},
		/* Serial Number String Descriptor */
		.utf16le_sn = {
			.bLength = 0x0C,
			.bDescriptorType = USB_STRING_DESC,
			.bString = {'0', 0, '0', 0, '.', 0, '0', 0, '1', 0,},
		},
	},

	.term_descr = {
		.bLength = 0,
		.bDescriptorType = 0,
	},
};

/**
 * @brief Custom handler for standard requests in order to
 *        catch the request and return the WebUSB Platform
 *        Capability Descriptor.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
int webusb_custom_handle_req(struct usb_setup_packet *pSetup,
			     s32_t *len, u8_t **data)
{
	LOG_DBG("");

	/* Call the callback */
	if ((req_handlers && req_handlers->custom_handler) &&
		(!req_handlers->custom_handler(pSetup, len, data)))
		return 0;

	return -ENOTSUP;
}

/**
 * @brief Handler called for WebUSB vendor specific commands.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
int webusb_vendor_handle_req(struct usb_setup_packet *pSetup,
			     s32_t *len, u8_t **data)
{
	/* Call the callback */
	if ((req_handlers && req_handlers->vendor_handler) &&
		(!req_handlers->vendor_handler(pSetup, len, data)))
		return 0;

	return -ENOTSUP;
}

/**
 * @brief Register Custom and Vendor request callbacks
 *
 * This function registers Custom and Vendor request callbacks
 * for handling the device requests.
 *
 * @param [in] handlers Pointer to WebUSB request handlers structure
 *
 * @return N/A
 */
void webusb_register_request_handlers(struct webusb_req_handlers *handlers)
{
	req_handlers = handlers;
}

static void webusb_write_cb(u8_t ep, int size, void *priv)
{
	LOG_DBG("ep %x size %u", ep, size);
}

static void webusb_read_cb(u8_t ep, int size, void *priv)
{
	LOG_DBG("ep %x size %u", ep, size);

	if (size <= 0) {
		goto done;
	}

	usb_transfer(WEBUSB_ENDP_IN, rx_buf, size, USB_TRANS_WRITE,
		     webusb_write_cb, NULL);
done:
	usb_transfer(WEBUSB_ENDP_OUT, rx_buf, sizeof(rx_buf), USB_TRANS_READ,
		     webusb_read_cb, NULL);
}

/**
 * @brief Callback used to know the USB connection status
 *
 * @param status USB device status code.
 *
 * @return  N/A.
 */
static void webusb_dev_status_cb(struct usb_cfg_data *cfg,
				 enum usb_dc_status_code status,
				 const u8_t *param)
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
		webusb_read_cb(WEBUSB_ENDP_OUT, 0, NULL);
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

/* Describe EndPoints configuration */
static struct usb_ep_cfg_data webusb_ep_data[] = {
	{
		.ep_cb	= usb_transfer_ep_callback,
		.ep_addr = WEBUSB_ENDP_OUT
	},
	{
		.ep_cb = usb_transfer_ep_callback,
		.ep_addr = WEBUSB_ENDP_IN
	}
};

/* Configuration of the CDC-ACM Device send to the USB Driver */
USBD_CFG_DATA_DEFINE(webusb) struct usb_cfg_data webusb_config = {
	.usb_device_description = (u8_t *)&webusb_usb_description,
	.cb_usb_status = webusb_dev_status_cb,
	.interface = {
		.class_handler = NULL,
		.custom_handler = webusb_custom_handle_req,
		.vendor_handler = webusb_vendor_handle_req,
		.vendor_data = interface_data,
		.payload_data = interface_data,
	},
	.num_endpoints = ARRAY_SIZE(webusb_ep_data),
	.endpoint = webusb_ep_data
};

int webusb_init(void)
{
	int ret;

	LOG_DBG("");

	/* Initialize the WebUSB driver with the right configuration */
	ret = usb_set_config(&webusb_config);
	if (ret < 0) {
		LOG_ERR("Failed to config USB");
		return ret;
	}

	/* Enable WebUSB driver */
	ret = usb_enable(&webusb_config);
	if (ret < 0) {
		LOG_ERR("Failed to enable USB");
		return ret;
	}

	return 0;
}
