/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HOGP_TEST_COMMON_H_
#define HOGP_TEST_COMMON_H_

#include <zephyr/bluetooth/hid.h>

/* HID Report Map: Generic Desktop, Mouse (3 buttons + X + Y) */
static const uint8_t test_report_map[] = {
	0x05, 0x01,       /* Usage Page (Generic Desktop) */
	0x09, 0x02,       /* Usage (Mouse) */
	0xA1, 0x01,       /* Collection (Application) */
	0x85, 0x01,       /*   Report ID (1) */
	0x09, 0x01,       /*   Usage (Pointer) */
	0xA1, 0x00,       /*   Collection (Physical) */
	0x05, 0x09,       /*     Usage Page (Buttons) */
	0x19, 0x01,       /*     Usage Minimum (1) */
	0x29, 0x03,       /*     Usage Maximum (3) */
	0x15, 0x00,       /*     Logical Minimum (0) */
	0x25, 0x01,       /*     Logical Maximum (1) */
	0x95, 0x03,       /*     Report Count (3) */
	0x75, 0x01,       /*     Report Size (1) */
	0x81, 0x02,       /*     Input (Variable) */
	0x95, 0x01,       /*     Report Count (1) */
	0x75, 0x05,       /*     Report Size (5) */
	0x81, 0x03,       /*     Input (Constant) */
	0x05, 0x01,       /*     Usage Page (Generic Desktop) */
	0x09, 0x30,       /*     Usage (X) */
	0x09, 0x31,       /*     Usage (Y) */
	0x15, 0x81,       /*     Logical Minimum (-127) */
	0x25, 0x7F,       /*     Logical Maximum (127) */
	0x75, 0x08,       /*     Report Size (8) */
	0x95, 0x02,       /*     Report Count (2) */
	0x81, 0x06,       /*     Input (Variable, Relative) */
	0xC0,             /*   End Collection */
	0xC0,             /* End Collection */
};

/* Report definitions */
#define TEST_REPORT_ID_INPUT   1
#define TEST_REPORT_ID_OUTPUT  2
#define TEST_REPORT_ID_FEATURE 3

/* Expected HID Information */
#define TEST_BCD_HID       0x0111
#define TEST_COUNTRY_CODE  0x00
#define TEST_HID_FLAGS     0x02

/* Test data */
#define TEST_INPUT_REPORT_LEN  3  /* buttons(1) + X(1) + Y(1) */
static const uint8_t test_input_data[TEST_INPUT_REPORT_LEN] = {0x01, 0x10, 0x20};

#endif /* HOGP_TEST_COMMON_H_ */
