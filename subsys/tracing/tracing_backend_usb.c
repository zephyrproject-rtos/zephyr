/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>
#include <sys/atomic.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <tracing_core.h>
#include <tracing_buffer.h>
#include <tracing_backend.h>

#define USB_TRANSFER_ONGOING               1
#define USB_TRANSFER_FREE                  0

#define TRACING_IF_IN_EP_ADDR              0x81
#define TRACING_IF_OUT_EP_ADDR             0x01

struct usb_device_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_out_ep;
} __packed;

static volatile int transfer_state;
static enum usb_dc_status_code usb_device_status = USB_DC_UNKNOWN;

USBD_CLASS_DESCR_DEFINE(primary, 0) struct usb_device_desc dev_desc = {
	/*
	 * Interface descriptor 0
	 */
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = CUSTOM_CLASS,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},

	/*
	 * Data Endpoint IN
	 */
	.if0_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = TRACING_IF_IN_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(CONFIG_TRACING_USB_MPS),
		.bInterval = 0x00,
	},

	/*
	 * Data Endpoint OUT
	 */
	.if0_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = TRACING_IF_OUT_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(CONFIG_TRACING_USB_MPS),
		.bInterval = 0x00,
	},
};

static void dev_status_cb(struct usb_cfg_data *cfg,
			  enum usb_dc_status_code status,
			  const uint8_t *param)
{
	ARG_UNUSED(cfg);
	ARG_UNUSED(param);

	usb_device_status = status;
}

static void tracing_ep_out_cb(uint8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	uint8_t *cmd = NULL;
	uint32_t bytes_to_read, length;

	usb_read(ep, NULL, 0, &bytes_to_read);

	while (bytes_to_read) {
		length = tracing_cmd_buffer_alloc(&cmd);
		if (cmd) {
			length = MIN(length, bytes_to_read);
			usb_read(ep, cmd, length, NULL);
			tracing_cmd_handle(cmd, length);

			bytes_to_read -= length;
		}
	}

	/*
	 * send ZLP to sync with host receive thread
	 */
	usb_write(TRACING_IF_IN_EP_ADDR, NULL, 0, NULL);
}

static void tracing_ep_in_cb(uint8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	ARG_UNUSED(ep);
	ARG_UNUSED(ep_status);

	transfer_state = USB_TRANSFER_FREE;
}

static struct usb_ep_cfg_data ep_cfg[] = {
	{
		.ep_cb = tracing_ep_out_cb,
		.ep_addr = TRACING_IF_OUT_EP_ADDR,
	},
	{
		.ep_cb = tracing_ep_in_cb,
		.ep_addr = TRACING_IF_IN_EP_ADDR,
	},
};

USBD_CFG_DATA_DEFINE(primary, tracing_backend_usb)
	struct usb_cfg_data tracing_backend_usb_config = {
	.usb_device_description = NULL,
	.interface_descriptor = &dev_desc.if0,
	.cb_usb_status = dev_status_cb,
	.interface = {
		.class_handler = NULL,
		.custom_handler = NULL,
		.vendor_handler = NULL,
	},
	.num_endpoints = ARRAY_SIZE(ep_cfg),
	.endpoint = ep_cfg,
};

static void tracing_backend_usb_output(const struct tracing_backend *backend,
				       uint8_t *data, uint32_t length)
{
	int ret = 0;
	uint32_t bytes;

	while (length > 0) {
		transfer_state = USB_TRANSFER_ONGOING;

		/*
		 * make sure every USB tansfer no need ZLP at all
		 * because we are in lowest priority thread content
		 * there are no deterministic time between real USB
		 * packet and ZLP
		 */
		ret = usb_write(TRACING_IF_IN_EP_ADDR, data,
				length >= CONFIG_TRACING_USB_MPS ?
				CONFIG_TRACING_USB_MPS - 1 : length, &bytes);
		if (ret) {
			continue;
		}

		data += bytes;
		length -= bytes;

		while (transfer_state == USB_TRANSFER_ONGOING) {
		}
	}
}

const struct tracing_backend_api tracing_backend_usb_api = {
	.output = tracing_backend_usb_output
};

TRACING_BACKEND_DEFINE(tracing_backend_usb, tracing_backend_usb_api);
