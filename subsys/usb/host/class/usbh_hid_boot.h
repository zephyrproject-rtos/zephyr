/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/class/hid.h>
#include <zephyr/input/input.h>
#include <zephyr/usb/usbh.h>

/*
 * lmb - left mouse button
 * rmb - right mouse button
 * mmb - middle mouse button
 */
union hid_mouse_btns {
	uint8_t raw;
	struct {
		uint8_t lmb: 1;
		uint8_t rmb: 1;
		uint8_t mmb: 1;
		uint8_t: 5;
	};
} __packed;

struct hid_mouse_report {
	union hid_mouse_btns btns;
	int8_t dx;
	int8_t dy;
} __packed;

union hid_kbd_modifiers {
	uint8_t raw;
	struct {
		uint8_t l_ctrl: 1;
		uint8_t l_shift: 1;
		uint8_t l_alt: 1;
		uint8_t l_gui: 1;

		uint8_t r_ctrl: 1;
		uint8_t r_shift: 1;
		uint8_t r_alt: 1;
		uint8_t r_gui: 1;
	};
} __packed;

struct hid_kbd_report {
	union hid_kbd_modifiers modifiers;
	uint8_t: 8;
	uint8_t pressed_keys[6];
} __packed;

enum hid_boot_type {
	KEYBOARD = HID_BOOT_IFACE_CODE_KEYBOARD,
	MOUSE = HID_BOOT_IFACE_CODE_MOUSE,
};

struct hid_boot_data {
	const struct device *dev;
	struct uhc_transfer *int_xfer;
	struct usb_device *udev;
	enum hid_boot_type type;
	union {
		union hid_mouse_btns mouse_btns;
		struct hid_kbd_report kbd;
	} last_report;
	uint8_t if_idx;
};
