/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_cdc.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_test_class, LOG_LEVEL_DBG);

#include "test_class.h"

/* Definition of a test class used only to test USB descriptors */

struct usbd_foo_desc {
	struct usb_association_descriptor iad;
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_fs_out_ep;
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_fs_in_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
	struct usb_if_descriptor if1_alt0;
	struct usb_if_descriptor if1_alt1;
	struct usb_desc_header nil_desc;
};

struct foo_data {
	struct usbd_class_data *c_data;
	struct usbd_foo_desc *desc;
	struct usb_desc_header **fs_desc;
	struct usb_desc_header **hs_desc;
};

static void usbd_foo_enable(struct usbd_class_data *const c_data)
{
	LOG_INF("Enabled %s", c_data->name);
}

static void usbd_foo_disable(struct usbd_class_data *const c_data)
{
	LOG_INF("Disabled %s", c_data->name);
}

static int usbd_foo_init(struct usbd_class_data *const c_data)
{
	LOG_INF("Initialized %s", c_data->name);
	return 0;
}

static void usbd_foo_shutdown(struct usbd_class_data *const c_data)
{
	LOG_INF("Shutdown %s", c_data->name);
}

static void *usbd_foo_get_desc(struct usbd_class_data *const c_data,
			       const enum usbd_speed speed)
{
	struct foo_data *const data = usbd_class_get_private(c_data);

	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}

	return data->fs_desc;
}

static struct usbd_class_api usbd_foo_api = {
	.enable = usbd_foo_enable,
	.disable = usbd_foo_disable,
	.init = usbd_foo_init,
	.shutdown = usbd_foo_shutdown,
	.get_desc = usbd_foo_get_desc,
};

static struct usbd_foo_desc foo_desc = {

	/* Interface Association descriptor */
	.iad = {
		.bLength = sizeof(struct usb_association_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE_ASSOC,
		.bFirstInterface = 1,
		.bInterfaceCount = 2,
		.bFunctionClass = FOO_TEST_CLASS,
		.bFunctionSubClass = FOO_TEST_SUB,
		.bFunctionProtocol = FOO_TEST_PROTO,
	},

	/* Interface descriptor */
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = USB_BCC_VENDOR,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},

	/* Endpoint OUT */
	.if0_fs_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x01,
		.bmAttributes = USB_EP_TYPE_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(64U),
		.bInterval = 0x00,
	},
	.if0_hs_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x01,
		.bmAttributes = USB_EP_TYPE_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(512U),
		.bInterval = 0x00,
	},

	/* Endpoint IN */
	.if0_fs_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x81,
		.bmAttributes = USB_EP_TYPE_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(64U),
		.bInterval = 0x00,
	},
	.if0_hs_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x81,
		.bmAttributes = USB_EP_TYPE_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(512U),
		.bInterval = 0x00,
	},

	/* Interface descriptor */
	.if1_alt0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 1,
		.bAlternateSetting = 0,
		.bNumEndpoints = 0,
		.bInterfaceClass = USB_BCC_VENDOR,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},

	/* Interface descriptor */
	.if1_alt1 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 1,
		.bAlternateSetting = 1,
		.bNumEndpoints = 0,
		.bInterfaceClass = USB_BCC_VENDOR,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},

	.nil_desc = {
		.bLength = 0,
		.bDescriptorType = 0,
	},
};

static struct usb_desc_header *foo_fs_desc[] = {
	(struct usb_desc_header *) &foo_desc.iad,
	(struct usb_desc_header *) &foo_desc.if0,
	(struct usb_desc_header *) &foo_desc.if0_fs_out_ep,
	(struct usb_desc_header *) &foo_desc.if0_fs_in_ep,
	(struct usb_desc_header *) &foo_desc.if1_alt0,
	(struct usb_desc_header *) &foo_desc.if1_alt1,
	(struct usb_desc_header *) &foo_desc.nil_desc,
};

static struct usb_desc_header *foo_hs_desc[] = {
	(struct usb_desc_header *) &foo_desc.iad,
	(struct usb_desc_header *) &foo_desc.if0,
	(struct usb_desc_header *) &foo_desc.if0_hs_out_ep,
	(struct usb_desc_header *) &foo_desc.if0_hs_in_ep,
	(struct usb_desc_header *) &foo_desc.if1_alt0,
	(struct usb_desc_header *) &foo_desc.if1_alt1,
	(struct usb_desc_header *) &foo_desc.nil_desc,
};

static struct foo_data foo_data;

USBD_DEFINE_CLASS(foo, &usbd_foo_api, &foo_data, NULL);

static struct foo_data foo_data = {
	.c_data = &foo,
	.desc = &foo_desc,
	.fs_desc = foo_fs_desc,
	.hs_desc = foo_hs_desc,
};
