/*
 * Human Interface Device (HID) USB class core header
 *
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
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

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_HID_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_HID_H_

#include <usb/hid.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/* Public headers */

typedef int (*hid_cb_t)(struct usb_setup_packet *setup, s32_t *len,
			u8_t **data);
typedef void (*hid_int_ready_callback)(void);
typedef void (*hid_protocol_cb_t)(u8_t protocol);
typedef void (*hid_idle_cb_t)(u16_t report_id);

struct hid_ops {
	hid_cb_t get_report;
	hid_cb_t get_idle;
	hid_cb_t get_protocol;
	hid_cb_t set_report;
	hid_cb_t set_idle;
	hid_cb_t set_protocol;
	hid_protocol_cb_t protocol_change;
	hid_idle_cb_t on_idle;
	/*
	 * int_in_ready is an optional callback that is called when
	 * the current interrupt IN transfer has completed.  This can
	 * be used to wait for the endpoint to go idle or to trigger
	 * the next transfer.
	 */
	hid_int_ready_callback int_in_ready;
#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
	hid_int_ready_callback int_out_ready;
#endif
};

/* Register HID device */
void usb_hid_register_device(struct device *dev, const u8_t *desc, size_t size,
			     const struct hid_ops *op);

/* Write to hid interrupt endpoint */
int hid_int_ep_write(const struct device *dev, const u8_t *data, u32_t data_len,
		     u32_t *bytes_ret);

/* Read from hid interrupt endpoint */
int hid_int_ep_read(const struct device *dev, u8_t *data, u32_t max_data_len,
		    u32_t *ret_bytes);

/* Initialize USB HID */
int usb_hid_init(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_HID_H_ */
