/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>

#include "test_descriptor.h"

const struct test_device_descriptor test_desc = {
	.dev = {
		.bLength = 18,
		.bDescriptorType = 1,
		.bcdUSB = 0x0200,
		.bDeviceClass = USB_BCC_MISCELLANEOUS,
		.bDeviceSubClass = 0x02,
		.bDeviceProtocol = 0x01, /* Interface Association */
		.bMaxPacketSize0 = 64,
		.idVendor = 0x2fe3,
		.idProduct = 0x0000,
		.bcdDevice = 0x0001,
		.iManufacturer = 0,
		.iProduct = 0,
		.iSerialNumber = 0,
		.bNumConfigurations = 1,
	},
	.cfg = {
		/* Configuration descriptor */
		.desc = {
			.bLength = 9,
			.bDescriptorType = 2,
			.wTotalLength =
				sys_cpu_to_le16(sizeof(struct test_configuration_descriptor)),
			.bNumInterfaces = 2,
			.bConfigurationValue = 1,
			.iConfiguration = 0,
			.bmAttributes = 0xe0,
			.bMaxPower = 0,
		},
		.foo_func = {
			/* Interface Association descriptor */
			.iad = {
				.bLength = sizeof(struct usb_association_descriptor),
				.bDescriptorType = USB_DESC_INTERFACE_ASSOC,
				.bFirstInterface = 0,
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
			.if0_out_ep = {
				.bLength = sizeof(struct usb_ep_descriptor),
				.bDescriptorType = USB_DESC_ENDPOINT,
				.bEndpointAddress = 0x01,
				.bmAttributes = USB_EP_TYPE_BULK,
				.wMaxPacketSize = sys_cpu_to_le16(64U),
				.bInterval = 0x00,
			},

			/* Endpoint IN */
			.if0_in_ep = {
				.bLength = sizeof(struct usb_ep_descriptor),
				.bDescriptorType = USB_DESC_ENDPOINT,
				.bEndpointAddress = 0x81,
				.bmAttributes = USB_EP_TYPE_BULK,
				.wMaxPacketSize = sys_cpu_to_le16(64U),
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
		},
	},

	/* Nil descriptor */
	.nil = {0},
};

struct usb_device test_udev0 = {
	.cfg_desc = (void *)&test_desc.cfg,
	.ifaces = {
		{.dhp = (void *)&test_desc.cfg.foo_func.if0},
		{.dhp = (void *)&test_desc.cfg.foo_func.if1_alt0},
	},
};
