/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_USB_HOST_SAMPLE_DESC_H_
#define ZEPHYR_TEST_USB_HOST_SAMPLE_DESC_H_

#include <stddef.h>
#include <stdint.h>

#define _LE16(n) ((n) & 0xff), ((n) >> 8)

/* bInterfaceClass of these descriptors */
#define USB_HUB_CLASSCODE 0x09

/*
 * Obtained with lsusb then verified against the
 * USB 2.0 standard's sample HUB descriptor
 */

#define TEST_HUB_DEVICE_DESCRIPTOR \
	18,		/* bLength */ \
	1,		/* bDescriptorType */ \
	_LE16(0x0200),	/* bcdUSB */ \
	0x09,		/* bDeviceClass */ \
	0x00,		/* bDeviceSubClass */ \
	0x02,		/* bDeviceProtocol */ \
	64,		/* bMaxPacketSize0 */ \
	_LE16(0x0bda),	/* idVendor */ \
	_LE16(0x5411),	/* idProduct */ \
	_LE16(0x0001),	/* bcdDevice */ \
	0,		/* iManufacturer */ \
	0,		/* iProduct */ \
	0,		/* iSerial */ \
	1,		/* bNumConfigurations */

#define TEST_HUB_CONFIG_DESCRIPTOR \
	9,		/* bLength */ \
	2,		/* bDescriptorType */ \
	_LE16(0x0029),	/* wTotalLength */ \
	1,		/* bNumInterfaces */ \
	1,		/* bConfigurationValue */ \
	0,		/* iConfiguration */ \
	0xe0,		/* bmAttributes */ \
	0,		/* MaxPower */

#define TEST_HUB_INTERFACE_ALT0_DESCRIPTOR \
	9,		/* bLength */ \
	4,		/* bDescriptorType */ \
	0,		/* bInterfaceNumber */ \
	0,		/* bAlternateSetting */ \
	1,		/* bNumEndpoints */ \
	9,		/* bInterfaceClass */ \
	0,		/* bInterfaceSubClass */ \
	1,		/* bInterfaceProtocol */ \
	0,		/* iInterface */

#define TEST_HUB_INTERFACE_ALT1_DESCRIPTOR \
	9,		/* bLength */ \
	4,		/* bDescriptorType */ \
	0,		/* bInterfaceNumber */ \
	1,		/* bAlternateSetting */ \
	1,		/* bNumEndpoints */ \
	9,		/* bInterfaceClass */ \
	0,		/* bInterfaceSubClass */ \
	2,		/* bInterfaceProtocol */ \
	0,		/* iInterface */

#define TEST_HUB_ENDPOINT_DESCRIPTOR \
	7,		/* bLength */ \
	5,		/* bDescriptorType */ \
	0x81,		/* bEndpointAddress */ \
	0x03,		/* bmAttributes */ \
	_LE16(1),	/* wMaxPacketSize */ \
	12,		/* bInterval */

#define TEST_HUB_DESCRIPTOR \
	TEST_HUB_CONFIG_DESCRIPTOR \
	TEST_HUB_INTERFACE_ALT0_DESCRIPTOR \
	TEST_HUB_ENDPOINT_DESCRIPTOR \
	TEST_HUB_INTERFACE_ALT1_DESCRIPTOR \
	TEST_HUB_ENDPOINT_DESCRIPTOR

#endif /* ZEPHYR_TEST_USB_HOST_SAMPLE_DESC_H_ */
