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
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(webusb);

#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_device.h>
#include <usb_descriptor.h>

#include "webusb.h"

/* Max packet size for Bulk endpoints */
#if defined(CONFIG_USB_DC_HAS_HS_SUPPORT)
#define WEBUSB_BULK_EP_MPS		512
#else
#define WEBUSB_BULK_EP_MPS		64
#endif

/* Number of interfaces */
#define WEBUSB_NUM_ITF			0x01
/* Number of Endpoints in the custom interface */
#define WEBUSB_NUM_EP			0x02

#define WEBUSB_IN_EP_IDX		0
#define WEBUSB_OUT_EP_IDX		1

static struct webusb_req_handlers *req_handlers;

uint8_t rx_buf[WEBUSB_BULK_EP_MPS];

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
	struct usb_ep_descriptor if0_out_ep;
} __packed webusb_desc = {
	.if0 = INITIALIZER_IF(WEBUSB_NUM_EP, USB_BCC_VENDOR),
	.if0_in_ep = INITIALIZER_IF_EP(AUTO_EP_IN, USB_DC_EP_BULK,
				       WEBUSB_BULK_EP_MPS, 0),
	.if0_out_ep = INITIALIZER_IF_EP(AUTO_EP_OUT, USB_DC_EP_BULK,
					WEBUSB_BULK_EP_MPS, 0),
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
			     int32_t *len, uint8_t **data)
{
	LOG_DBG("");

	/* Call the callback */
	if (req_handlers && req_handlers->custom_handler) {
		return req_handlers->custom_handler(pSetup, len, data);
	}

	return -EINVAL;
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
			     int32_t *len, uint8_t **data)
{
	/* Call the callback */
	if (req_handlers && req_handlers->vendor_handler) {
		return req_handlers->vendor_handler(pSetup, len, data);
	}

	return -EINVAL;
}

/**
 * @brief Register Custom and Vendor request callbacks
 *
 * This function registers Custom and Vendor request callbacks
 * for handling the device requests.
 *
 * @param [in] handlers Pointer to WebUSB request handlers structure
 */
void webusb_register_request_handlers(struct webusb_req_handlers *handlers)
{
	req_handlers = handlers;
}

static void webusb_write_cb(uint8_t ep, int size, void *priv)
{
	LOG_DBG("ep %x size %u", ep, size);
}

static void webusb_read_cb(uint8_t ep, int size, void *priv)
{
	struct usb_cfg_data *cfg = priv;

	LOG_DBG("cfg %p ep %x size %u", cfg, ep, size);

	if (size <= 0) {
		goto done;
	}

	usb_transfer(cfg->endpoint[WEBUSB_IN_EP_IDX].ep_addr, rx_buf, size,
		     USB_TRANS_WRITE, webusb_write_cb, cfg);
done:
	usb_transfer(ep, rx_buf, sizeof(rx_buf), USB_TRANS_READ,
		     webusb_read_cb, cfg);
}

/**
 * @brief Callback used to know the USB connection status
 *
 * @param status USB device status code.
 */
static void webusb_dev_status_cb(struct usb_cfg_data *cfg,
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
		webusb_read_cb(cfg->endpoint[WEBUSB_OUT_EP_IDX].ep_addr,
			       0, cfg);
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
		.ep_cb = usb_transfer_ep_callback,
		.ep_addr = AUTO_EP_IN
	},
	{
		.ep_cb	= usb_transfer_ep_callback,
		.ep_addr = AUTO_EP_OUT
	}
};

USBD_DEFINE_CFG_DATA(webusb_config) = {
	.usb_device_description = NULL,
	.interface_descriptor = &webusb_desc.if0,
	.cb_usb_status = webusb_dev_status_cb,
	.interface = {
		.class_handler = NULL,
		.custom_handler = webusb_custom_handle_req,
		.vendor_handler = webusb_vendor_handle_req,
	},
	.num_endpoints = ARRAY_SIZE(webusb_ep_data),
	.endpoint = webusb_ep_data
};
