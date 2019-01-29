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

/* HID Class Descriptor Types */

#define HID_CLASS_DESCRIPTOR_HID	(REQTYPE_TYPE_CLASS << 5 | 0x01)
#define HID_CLASS_DESCRIPTOR_REPORT	(REQTYPE_TYPE_CLASS << 5 | 0x02)

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
	usb_dc_status_callback status_cb;
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

/* Protocols */
#define HID_PROTOCOL_BOOT		0x00
#define HID_PROTOCOL_REPORT		0x01

/* Example HID report descriptors */
/**
 * @brief Simple HID mouse report descriptor for n button mouse.
 *
 * @param bcnt	Button count. Allowed values from 1 to 8.
 */
#define HID_MOUSE_REPORT_DESC(bcnt) {				\
	/* USAGE_PAGE (Generic Desktop) */			\
	HID_GI_USAGE_PAGE, USAGE_GEN_DESKTOP,			\
	/* USAGE (Mouse) */					\
	HID_LI_USAGE, USAGE_GEN_DESKTOP_MOUSE,			\
	/* COLLECTION (Application) */				\
	HID_MI_COLLECTION, COLLECTION_APPLICATION,		\
		/* USAGE (Pointer) */				\
		HID_LI_USAGE, USAGE_GEN_DESKTOP_POINTER,	\
		/* COLLECTION (Physical) */			\
		HID_MI_COLLECTION, COLLECTION_PHYSICAL,		\
			/* Bits used for button signalling */	\
			/* USAGE_PAGE (Button) */		\
			HID_GI_USAGE_PAGE, USAGE_GEN_BUTTON,	\
			/* USAGE_MINIMUM (Button 1) */		\
			HID_LI_USAGE_MIN(1), 0x01,		\
			/* USAGE_MAXIMUM (Button bcnt) */	\
			HID_LI_USAGE_MAX(1), bcnt,		\
			/* LOGICAL_MINIMUM (0) */		\
			HID_GI_LOGICAL_MIN(1), 0x00,		\
			/* LOGICAL_MAXIMUM (1) */		\
			HID_GI_LOGICAL_MAX(1), 0x01,		\
			/* REPORT_SIZE (1) */			\
			HID_GI_REPORT_SIZE, 0x01,		\
			/* REPORT_COUNT (bcnt) */		\
			HID_GI_REPORT_COUNT, bcnt,		\
			/* INPUT (Data,Var,Abs) */		\
			HID_MI_INPUT, 0x02,			\
			/* Unused bits */			\
			/* REPORT_SIZE (8 - bcnt) */		\
			HID_GI_REPORT_SIZE, (8 - bcnt),		\
			/* REPORT_COUNT (1) */			\
			HID_GI_REPORT_COUNT, 0x01,		\
			/* INPUT (Cnst,Ary,Abs) */		\
			HID_MI_INPUT, 0x01,			\
			/* X and Y axis, scroll */		\
			/* USAGE_PAGE (Generic Desktop) */	\
			HID_GI_USAGE_PAGE, USAGE_GEN_DESKTOP,	\
			/* USAGE (X) */				\
			HID_LI_USAGE, USAGE_GEN_DESKTOP_X,	\
			/* USAGE (Y) */				\
			HID_LI_USAGE, USAGE_GEN_DESKTOP_Y,	\
			/* USAGE (WHEEL) */			\
			HID_LI_USAGE, USAGE_GEN_DESKTOP_WHEEL,	\
			/* LOGICAL_MINIMUM (-127) */		\
			HID_GI_LOGICAL_MIN(1), -127,		\
			/* LOGICAL_MAXIMUM (127) */		\
			HID_GI_LOGICAL_MAX(1), 127,		\
			/* REPORT_SIZE (8) */			\
			HID_GI_REPORT_SIZE, 0x08,		\
			/* REPORT_COUNT (3) */			\
			HID_GI_REPORT_COUNT, 0x03,		\
			/* INPUT (Data,Var,Rel) */		\
			HID_MI_INPUT, 0x06,			\
		/* END_COLLECTION */				\
		HID_MI_COLLECTION_END,				\
	/* END_COLLECTION */					\
	HID_MI_COLLECTION_END,					\
}


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
