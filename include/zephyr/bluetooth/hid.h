/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bluetooth HID common definitions.
 *
 * Transport-agnostic HID constants shared by Classic HID (HIDP),
 * BLE HID Service (HIDS), and HID over GATT Profile (HOGP).
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HID_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HID_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_hid Bluetooth HID
 * @ingroup bluetooth
 * @{
 */

/** HID Report Type: Other */
#define BT_HID_REPORT_TYPE_OTHER   0
/** HID Report Type: Input */
#define BT_HID_REPORT_TYPE_INPUT   1
/** HID Report Type: Output */
#define BT_HID_REPORT_TYPE_OUTPUT  2
/** HID Report Type: Feature */
#define BT_HID_REPORT_TYPE_FEATURE 3

/** HID Protocol Mode: Boot */
#define BT_HID_PROTOCOL_BOOT   0
/** HID Protocol Mode: Report */
#define BT_HID_PROTOCOL_REPORT 1

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HID_H_ */
