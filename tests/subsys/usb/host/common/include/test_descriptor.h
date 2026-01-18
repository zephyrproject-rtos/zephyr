/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_USB_HOST_SAMPLE_DESC_H_
#define ZEPHYR_TEST_USB_HOST_SAMPLE_DESC_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/usbh.h>

#define FOO_TEST_VID	0x2FE3
#define FOO_TEST_PID	0x0000
#define FOO_TEST_CLASS	0xff /* vendor-specific */
#define FOO_TEST_SUB	0x11 /* arbitrary */
#define FOO_TEST_PROTO	0xff /* vendor-specific */

struct foo_func_desc {
	struct usb_association_descriptor iad;
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_if_descriptor if1_alt0;
	struct usb_if_descriptor if1_alt1;
} __packed;

struct test_configuration_descriptor {
	struct usb_cfg_descriptor desc;
	struct foo_func_desc foo_func;
} __packed;

struct test_device_descriptor {
	struct usb_device_descriptor dev;
	struct test_configuration_descriptor cfg;
	struct usb_desc_header nil;
} __packed;

extern const struct test_device_descriptor test_desc;
extern struct usb_device test_udev0;

#endif /* ZEPHYR_TEST_USB_HOST_SAMPLE_DESC_H_ */
