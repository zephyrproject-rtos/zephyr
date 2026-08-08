/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bluetooth HID common definitions.
 *
 * Transport-agnostic HID constants shared by Classic HID (HIDP),
 * Bluetooth LE HID Service (HIDS), and HID over GATT Profile (HOGP).
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HIDS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HIDS_H_

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_hid Bluetooth HID
 *
 * @since 4.2
 * @version 0.1.0
 *
 * @ingroup bluetooth
 * @{
 */

/**
 * @brief HID Report Type values.
 *
 * Defined in HID Service Specification (HIDS v1.0), Section 2.2,
 * Report Reference Characteristic Descriptor.
 */
enum bt_hid_report_type {
	/** Input Report */
	BT_HID_REPORT_TYPE_INPUT   = 0x01U,
	/** Output Report */
	BT_HID_REPORT_TYPE_OUTPUT  = 0x02U,
	/** Feature Report */
	BT_HID_REPORT_TYPE_FEATURE = 0x03U,
};

/**
 * @brief HID Protocol Mode values.
 *
 * Defined in HID Service Specification (HIDS v1.0), Section 2.6,
 * Protocol Mode Characteristic.
 */
enum bt_hid_protocol_mode {
	/** Boot Protocol Mode */
	BT_HID_PROTOCOL_BOOT   = 0x00U,
	/** Report Protocol Mode */
	BT_HID_PROTOCOL_REPORT = 0x01U,
};

/**
 * @brief HID Information flags.
 *
 * Defined in HID Service Specification (HIDS v1.0), Section 2.8,
 * HID Information Characteristic.
 */
enum bt_hid_info_flags {
	/** Device supports remote wake */
	BT_HID_INFO_FLAG_REMOTE_WAKE          = BIT(0),
	/** Device is normally connectable */
	BT_HID_INFO_FLAG_NORMALLY_CONNECTABLE = BIT(1),
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HIDS_H_ */
