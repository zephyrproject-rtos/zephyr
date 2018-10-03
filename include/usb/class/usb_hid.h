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

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_HID_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_HID_H_

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
typedef void (*hid_int_ready_callback)(void);

struct hid_ops {
	hid_cb_t get_report;
	hid_cb_t get_idle;
	hid_cb_t get_protocol;
	hid_cb_t set_report;
	hid_cb_t set_idle;
	hid_cb_t set_protocol;
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

/* HID Report Definitions */

/* HID Items types */
#define ITEM_MAIN	0x0
#define ITEM_GLOBAL	0x1
#define ITEM_LOCAL	0x2

/* HID Main Items tags */
#define ITEM_TAG_INPUT		0x8
#define ITEM_TAG_OUTPUT		0x9
#define ITEM_TAG_COLLECTION	0xA
#define ITEM_TAG_COLLECTION_END	0xC

/* HID Global Items tags */
#define ITEM_TAG_USAGE_PAGE	0x0
#define ITEM_TAG_LOGICAL_MIN	0x1
#define ITEM_TAG_LOGICAL_MAX	0x2
#define ITEM_TAG_REPORT_SIZE	0x7
#define ITEM_TAG_REPORT_ID	0x8
#define ITEM_TAG_REPORT_COUNT	0x9

/* HID Local Items tags */
#define ITEM_TAG_USAGE		0x0
#define ITEM_TAG_USAGE_MIN	0x1
#define ITEM_TAG_USAGE_MAX	0x2

#define HID_ITEM(bTag, bType, bSize)	(((bTag & 0xF) << 4) | \
					 ((bType & 0x3) << 2) | (bSize & 0x3))

#define HID_MAIN_ITEM(bTag, bSize)	HID_ITEM(bTag, ITEM_MAIN, bSize)
#define HID_GLOBAL_ITEM(bTag, bSize)	HID_ITEM(bTag, ITEM_GLOBAL, bSize)
#define HID_LOCAL_ITEM(bTag, bSize)	HID_ITEM(bTag, ITEM_LOCAL, bSize)

#define HID_MI_COLLECTION		HID_MAIN_ITEM(ITEM_TAG_COLLECTION, 1)
#define HID_MI_COLLECTION_END		HID_MAIN_ITEM(ITEM_TAG_COLLECTION_END, \
						      0)
#define HID_MI_INPUT			HID_MAIN_ITEM(ITEM_TAG_INPUT, 1)
#define HID_MI_OUTPUT			HID_MAIN_ITEM(ITEM_TAG_OUTPUT, 1)

#define HID_GI_USAGE_PAGE		HID_GLOBAL_ITEM(ITEM_TAG_USAGE_PAGE, 1)
#define HID_GI_LOGICAL_MIN(size)	HID_GLOBAL_ITEM(ITEM_TAG_LOGICAL_MIN, \
							size)
#define HID_GI_LOGICAL_MAX(size)	HID_GLOBAL_ITEM(ITEM_TAG_LOGICAL_MAX, \
							size)
#define HID_GI_REPORT_SIZE		HID_GLOBAL_ITEM(ITEM_TAG_REPORT_SIZE, \
							1)
#define HID_GI_REPORT_ID		HID_GLOBAL_ITEM(ITEM_TAG_REPORT_ID, \
							1)
#define HID_GI_REPORT_COUNT		HID_GLOBAL_ITEM(ITEM_TAG_REPORT_COUNT, \
							1)

#define HID_LI_USAGE			HID_LOCAL_ITEM(ITEM_TAG_USAGE, 1)
#define HID_LI_USAGE_MIN(size)		HID_LOCAL_ITEM(ITEM_TAG_USAGE_MIN, \
						       size)
#define HID_LI_USAGE_MAX(size)		HID_LOCAL_ITEM(ITEM_TAG_USAGE_MAX, \
						       size)

/* Defined in Universal Serial Bus HID Usage Tables version 1.11 */
#define USAGE_GEN_DESKTOP		0x01
#define USAGE_GEN_KEYBOARD		0x07
#define USAGE_GEN_LEDS			0x08
#define USAGE_GEN_BUTTON		0x09

/* Generic Desktop Page usages */
#define USAGE_GEN_DESKTOP_UNDEFINED	0x00
#define USAGE_GEN_DESKTOP_POINTER	0x01
#define USAGE_GEN_DESKTOP_MOUSE		0x02
#define USAGE_GEN_DESKTOP_JOYSTICK	0x04
#define USAGE_GEN_DESKTOP_GAMEPAD	0x05
#define USAGE_GEN_DESKTOP_KEYBOARD	0x06
#define USAGE_GEN_DESKTOP_KEYPAD	0x07
#define USAGE_GEN_DESKTOP_X		0x30
#define USAGE_GEN_DESKTOP_Y		0x31
#define USAGE_GEN_DESKTOP_WHEEL		0x38

/* Collection types */
#define COLLECTION_PHYSICAL		0x00
#define COLLECTION_APPLICATION		0x01

/* Register HID device */
void usb_hid_register_device(const u8_t *desc, size_t size,
			     const struct hid_ops *op);

/* Write to hid interrupt endpoint */
int hid_int_ep_write(const u8_t *data, u32_t data_len, u32_t *bytes_ret);

/* Read from hid interrupt endpoint */
int hid_int_ep_read(u8_t *data, u32_t max_data_len, u32_t *ret_bytes);

/* Initialize USB HID */
int usb_hid_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_HID_H_ */
