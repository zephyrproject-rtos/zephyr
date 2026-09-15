/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HIDS_TEST_COMMON_H_
#define HIDS_TEST_COMMON_H_

#include <zephyr/bluetooth/services/hids.h>
#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/usb/class/hid.h>

/* One Report of each type, as configured by
 * CONFIG_BT_HIDS_{INPUT,OUTPUT,FEATURE}_REPORT_COUNT.
 */
#define TEST_REPORT_ID_INPUT        0x01U
#define TEST_REPORT_ID_OUTPUT       0x02U
#define TEST_REPORT_ID_FEATURE      0x03U
/* A second Feature Report, long enough to need several ATT Read Blob Requests */
#define TEST_REPORT_ID_FEATURE_LONG 0x04U

/* Mouse (Input), keyboard LEDs (Output) and a vendor defined byte (Feature).
 * The Report Map has to describe every Report the HID Service exposes.
 */
/* The indentation mirrors the HID collection nesting. */
/* clang-format off */
/* Longer than one ATT PDU at the MTU this test negotiates, so a Host has to
 * use the Read Long Characteristic Value sub-procedure to read it.
 */
#define TEST_LONG_REPORT_LEN 200

static const uint8_t test_report_map[] = {
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	HID_USAGE(HID_USAGE_GEN_DESKTOP_MOUSE),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
		HID_REPORT_ID(TEST_REPORT_ID_INPUT),
		HID_USAGE(HID_USAGE_GEN_DESKTOP_POINTER),
		HID_COLLECTION(HID_COLLECTION_PHYSICAL),
			HID_USAGE_PAGE(HID_USAGE_GEN_BUTTON),
			HID_USAGE_MIN8(1),
			HID_USAGE_MAX8(3),
			HID_LOGICAL_MIN8(0),
			HID_LOGICAL_MAX8(1),
			HID_REPORT_SIZE(1),
			HID_REPORT_COUNT(3),
			HID_INPUT(0x02),
			HID_REPORT_SIZE(5),
			HID_REPORT_COUNT(1),
			HID_INPUT(0x03),
			HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
			HID_USAGE(HID_USAGE_GEN_DESKTOP_X),
			HID_USAGE(HID_USAGE_GEN_DESKTOP_Y),
			HID_LOGICAL_MIN8(-127),
			HID_LOGICAL_MAX8(127),
			HID_REPORT_SIZE(8),
			HID_REPORT_COUNT(2),
			HID_INPUT(0x06),
		HID_END_COLLECTION,
	HID_END_COLLECTION,

	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	HID_USAGE(HID_USAGE_GEN_DESKTOP_KEYBOARD),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
		HID_REPORT_ID(TEST_REPORT_ID_OUTPUT),
		HID_USAGE_PAGE(HID_USAGE_GEN_LEDS),
		HID_USAGE_MIN8(1),
		HID_USAGE_MAX8(5),
		HID_REPORT_SIZE(1),
		HID_REPORT_COUNT(5),
		HID_OUTPUT(0x02),
		HID_REPORT_SIZE(3),
		HID_REPORT_COUNT(1),
		HID_OUTPUT(0x03),
		HID_REPORT_ID(TEST_REPORT_ID_FEATURE),
		HID_USAGE_PAGE16(0xFF00),
		HID_USAGE(0x01),
		HID_LOGICAL_MIN8(0),
		HID_LOGICAL_MAX16(0xFF, 0x00),
		HID_REPORT_SIZE(8),
		HID_REPORT_COUNT(1),
		HID_FEATURE(0x02),
		HID_REPORT_ID(TEST_REPORT_ID_FEATURE_LONG),
		HID_USAGE_PAGE16(0xFF00),
		HID_USAGE(0x02),
		HID_LOGICAL_MIN8(0),
		HID_LOGICAL_MAX16(0xFF, 0x00),
		HID_REPORT_SIZE(8),
		HID_REPORT_COUNT(TEST_LONG_REPORT_LEN),
		HID_FEATURE(0x02),
	HID_END_COLLECTION,
};
/* clang-format on */

/* HID Information the server registers and the client verifies */
#define TEST_BCD_HID      0x0111
#define TEST_COUNTRY_CODE 0x00
#define TEST_HID_FLAGS    BT_HID_INFO_FLAG_NORMALLY_CONNECTABLE

/* Input Report payload: buttons (1) + X (1) + Y (1) */
#define TEST_INPUT_REPORT_LEN 3
static const uint8_t test_input_data[TEST_INPUT_REPORT_LEN] = {0x01, 0x10, 0x20};

/* Output Report payload: the LED bitmap */
#define TEST_OUTPUT_REPORT_LEN 1
static const uint8_t test_output_data[TEST_OUTPUT_REPORT_LEN] = {0x05};

/* Feature Report payload: the vendor defined byte */
#define TEST_FEATURE_REPORT_LEN 1
static const uint8_t test_feature_data[TEST_FEATURE_REPORT_LEN] = {0x5A};

/* Boot Keyboard Output Report payload: the LED bitmap */
static const uint8_t test_boot_kb_out_data[BT_HIDS_BOOT_KB_OUT_LEN] = {0x03};

/* Boot Keyboard Input Report payload: modifiers, reserved and six key codes */
static const uint8_t test_boot_kb_in_data[BT_HIDS_BOOT_KB_IN_LEN] = {
	0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};

/* The value the server holds for the same Boot Report, which a read returns.
 * Distinct from the notified one, so that the read and the notification cannot
 * pass for one another.
 */
static const uint8_t test_boot_kb_in_stored[BT_HIDS_BOOT_KB_IN_LEN] = {
	0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};

/* Telling the three Boot Report contexts apart relies on their lengths being
 * distinct, so that a read or a length check identifies one of them.
 */
BUILD_ASSERT(BT_HIDS_BOOT_KB_IN_LEN != BT_HIDS_BOOT_KB_OUT_LEN &&
		     BT_HIDS_BOOT_KB_IN_LEN != BT_HIDS_BOOT_MOUSE_IN_LEN &&
		     BT_HIDS_BOOT_KB_OUT_LEN != BT_HIDS_BOOT_MOUSE_IN_LEN,
	     "The Boot Report lengths no longer tell the contexts apart");

#endif /* HIDS_TEST_COMMON_H_ */
