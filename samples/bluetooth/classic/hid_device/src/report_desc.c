/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include "report_desc.h"

const uint8_t hid_keyboard_report_desc[] = {
	0x05, 0x01, /* Usage Page (Generic Desktop) */
	0x09, 0x06, /* Usage (Keyboard) */
	0xa1, 0x01, /* Collection (Application) */
	0x05, 0x07, /*   Usage Page (Keyboard) */
	0x19, 0xe0, /*   Usage Minimum (224) */
	0x29, 0xe7, /*   Usage Maximum (231) */
	0x15, 0x00, /*   Logical Minimum (0) */
	0x25, 0x01, /*   Logical Maximum (1) */
	0x75, 0x01, /*   Report Size (1) */
	0x95, 0x08, /*   Report Count (8) */
	0x81, 0x02, /*   Input (Data, Variable, Absolute) */
	0x95, 0x01, /*   Report Count (1) */
	0x75, 0x08, /*   Report Size (8) */
	0x81, 0x01, /*   Input (Constant) */
	0x95, 0x06, /*   Report Count (6) */
	0x75, 0x08, /*   Report Size (8) */
	0x15, 0x00, /*   Logical Minimum (0) */
	0x25, 0x65, /*   Logical Maximum (101) */
	0x05, 0x07, /*   Usage Page (Keyboard) */
	0x19, 0x00, /*   Usage Minimum (0) */
	0x29, 0x65, /*   Usage Maximum (101) */
	0x81, 0x00, /*   Input (Data, Array) */
	0xc0,       /* End Collection */
};

const size_t hid_keyboard_report_desc_len = sizeof(hid_keyboard_report_desc);
