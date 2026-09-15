/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_HIDS_INTERNAL_H_
#define BT_HIDS_INTERNAL_H_

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/hids.h>
#include <zephyr/sys/atomic.h>

/* HID Information characteristic value layout */
#define HID_INFO_VAL_SIZE 4

#define HID_INFO_BCDHID_OFFSET       0
#define HID_INFO_COUNTRY_CODE_OFFSET 2
#define HID_INFO_FLAGS_OFFSET        3

/* The HID Information flags this service can back. The HID Service defines two
 * more, which announce the HID SCI feature and make the HID SCI Information and
 * HID SCI Mode characteristics mandatory, and reserves the rest.
 */
#define HID_INFO_FLAGS_SUPPORTED                                                                   \
	((uint8_t)(BT_HID_INFO_FLAG_REMOTE_WAKE | BT_HID_INFO_FLAG_NORMALLY_CONNECTABLE))

#define HIDS_REPORT_COUNT                                                                          \
	(CONFIG_BT_HIDS_INPUT_REPORT_COUNT + CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT +                  \
	 CONFIG_BT_HIDS_FEATURE_REPORT_COUNT)

/* Base of each Report type in the Report context array. The Report
 * characteristics appear in the service in this order, see
 * BT_HIDS_SERVICE_DEFINITION().
 */
#define HIDS_INPUT_BASE   0
#define HIDS_OUTPUT_BASE  (HIDS_INPUT_BASE + CONFIG_BT_HIDS_INPUT_REPORT_COUNT)
#define HIDS_FEATURE_BASE (HIDS_OUTPUT_BASE + CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT)

/* Report Reference characteristic descriptor value */
struct hids_report_ref {
	uint8_t id;
	uint8_t type;
};

/* Per Report characteristic state. Each Report characteristic refers to its
 * own context through the attribute user data.
 */
struct hids_report_ctx {
	struct hids_report_ref ref;
	/* Value a Host reads from this Report.
	 *
	 * The HID Service holds the value, like every other GATT service in
	 * the tree that has a value a Host can read, so a read is answered
	 * from one place and a Read Long cannot see the value change halfway
	 * through. The application replaces it with bt_hids_report_set(), and
	 * a Host write to a writable Report replaces it as well.
	 *
	 * value_len is 0 until the value is set for the first time, and a read
	 * then returns an empty value.
	 */
	uint8_t value[CONFIG_BT_HIDS_MAX_REPORT_LEN];
	uint16_t value_len;
	/* Report value attribute, resolved when the service is registered */
	const struct bt_gatt_attr *attr;
};

/* Boot Protocol Mode Report lengths, defined by the USB HID Specification.
 * The Boot Reports have a fixed format and length, so they are not described
 * in the Report Map and have no Report Reference descriptor.
 */
#define HIDS_BOOT_REPORT_MAX_LEN BT_HIDS_BOOT_KB_IN_LEN

/* Per Boot Report characteristic state. Each Boot Report characteristic refers
 * to its own context through the attribute user data.
 */
struct hids_boot_report_ctx {
	/* Value a Host reads from this Boot Report. It is all zeroes until the
	 * application sets it, which is the idle state of both a boot keyboard
	 * and a boot mouse.
	 */
	uint8_t value[HIDS_BOOT_REPORT_MAX_LEN];
	/* Fixed length of this Boot Report */
	uint8_t len;
	/* Report value attribute, resolved when the service is registered */
	const struct bt_gatt_attr *attr;
};

/* Per-connection HID state. The Protocol Mode and the Suspend state are
 * tracked per Host, and reset to their default values every time a Host
 * connects.
 *
 * The connection pointer is atomic because the slots are claimed and released
 * from the connected and disconnected callbacks as well as from
 * bt_hids_register() and bt_hids_unregister(), which run in the context of the
 * application.
 */
struct hids_conn {
	atomic_ptr_t conn;
	enum bt_hid_protocol_mode protocol_mode;
	/* Not readable over GATT; the HID Control Point is write only */
	bool suspended;
};

struct hids_state {
	bool registered;
	const uint8_t *report_map;
	uint16_t report_map_len;
	uint8_t hid_info[HID_INFO_VAL_SIZE];
};

#endif /* BT_HIDS_INTERNAL_H_ */
