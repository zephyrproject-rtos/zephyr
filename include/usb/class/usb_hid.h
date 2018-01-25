/*
 * Human Interface Device (HID) USB class core header
 *
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Human Interface Device (HID) Class public header
 *
 * Header follows Device Class Definition for Human Interface Devices (HID)
 * Version 1.11 document (HID1_11-1.pdf).
 */

#ifndef __USB_HID_H__
#define __USB_HID_H__

struct usb_hid_class_subdescriptor {
	u8_t bDescriptorType;
	u16_t wDescriptorLength;
} __packed;

struct usb_hid_descriptor {
	u8_t bLength;
	u8_t bDescriptorType;
	u16_t bcdHID;
	u8_t bCountryCode;
	u8_t bNumDescriptors;

	/*
	 * Specification says at least one Class Descriptor needs to
	 * be present (Report Descriptor).
	 */
	struct usb_hid_class_subdescriptor subdesc[1];
} __packed;

/* HID Class Specific Requests */

#define HID_GET_REPORT		0x01
#define HID_GET_IDLE		0x02
#define HID_GET_PROTOCOL	0x03
#define HID_SET_REPORT		0x09
#define HID_SET_IDLE		0x0A
#define HID_SET_PROTOCOL	0x0B

/* Public headers */

typedef int (*hid_cb_t)(struct usb_setup_packet *setup, s32_t *len,
			u8_t **data);

struct hid_ops {
	hid_cb_t get_report;
	hid_cb_t get_idle;
	hid_cb_t get_protocol;
	hid_cb_t set_report;
	hid_cb_t set_idle;
	hid_cb_t set_protocol;
};

/* Register HID device */
void usb_hid_register_device(const u8_t *desc, size_t size, struct hid_ops *op);

/* Initialize USB HID */
int usb_hid_init(void);

#endif /* __USB_HID_H__ */
