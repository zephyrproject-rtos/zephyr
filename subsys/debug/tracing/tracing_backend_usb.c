/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <usb_descriptor.h>
#include <tracing_backend.h>
#include <debug/tracing_core.h>

#define USB_TRANSFER_ONGOING               1
#define USB_TRANSFER_FREE                  0

#define TRACING_IF_IN_EP_ADDR              0x81
#define TRACING_IF_OUT_EP_ADDR             0x01

/* Max packet size for endpoints */
#define BULK_EP_MPS                        32

struct usb_device_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_out_ep;
} __packed;

static atomic_t transfer_state;
static struct tracing_packet *cmd;
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
		.wMaxPacketSize = sys_cpu_to_le16(BULK_EP_MPS),
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
		.wMaxPacketSize = sys_cpu_to_le16(BULK_EP_MPS),
		.bInterval = 0x00,
	},
};

static void dev_status_cb(struct usb_cfg_data *cfg,
				enum usb_dc_status_code status,
				const u8_t *param)
{
	ARG_UNUSED(cfg);
	ARG_UNUSED(param);

	usb_device_status = status;
}

static void tracing_ep_out_cb(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	u32_t bytes_to_read = 0;

	usb_read(ep, NULL, 0, &bytes_to_read);

	while (bytes_to_read) {
		cmd = tracing_packet_alloc();
		if (cmd) {
			cmd->direction = TRACING_IN;
			cmd->length = MIN(bytes_to_read, sizeof(cmd->buf));

			usb_read(ep, cmd->buf, cmd->length, NULL);
			cmd->buf[cmd->length] = '\0';
			bytes_to_read -= cmd->length;

			tracing_list_add_packet(cmd);
		}
	}
}

static void tracing_ep_in_cb(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	ARG_UNUSED(ep);
	ARG_UNUSED(ep_status);

	atomic_set(&transfer_state, USB_TRANSFER_FREE);
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

static inline void tracing_backend_usb_busy_wait(void)
{
	while (atomic_get(&transfer_state) == USB_TRANSFER_ONGOING) {
	}
}

static void tracing_backend_usb_output(const struct tracing_backend *backend,
					struct tracing_packet *packet)
{
	u8_t *buf = packet->buf;
	u32_t bytes, tsize = 0;
	int ret = 0, length = packet->length;

	while (length > 0) {
		atomic_set(&transfer_state, USB_TRANSFER_ONGOING);

		ret = usb_write(TRACING_IF_IN_EP_ADDR, buf, length, &bytes);

		buf += bytes;
		length -= bytes;
		tsize += bytes;

		tracing_backend_usb_busy_wait();
	}

	/*
	 * send ZLP if needed
	 */
	if (usb_dc_ep_mps(TRACING_IF_IN_EP_ADDR) &&
	    (tsize % usb_dc_ep_mps(TRACING_IF_IN_EP_ADDR) == 0)) {
		usb_write(TRACING_IF_IN_EP_ADDR, NULL, 0, NULL);

		tracing_backend_usb_busy_wait();
	}
}

static void tracing_backend_usb_init(void)
{
}

const struct tracing_backend_api tracing_backend_usb_api = {
	.init = tracing_backend_usb_init,
	.output  = tracing_backend_usb_output
};

TRACING_BACKEND_DEFINE(tracing_backend_usb, tracing_backend_usb_api);
