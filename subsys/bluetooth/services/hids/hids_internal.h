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

/* HID Information characteristic value layout */
#define HID_INFO_VAL_SIZE            4

#define HID_INFO_BCDHID_OFFSET       0
#define HID_INFO_COUNTRY_CODE_OFFSET 2
#define HID_INFO_FLAGS_OFFSET        3

#define HIDS_REPORT_COUNT \
	(CONFIG_BT_HIDS_INPUT_REPORT_COUNT + \
	 CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT + \
	 CONFIG_BT_HIDS_FEATURE_REPORT_COUNT)

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
	/* Report value attribute, resolved when the service is registered */
	const struct bt_gatt_attr *attr;
};

/* Per-connection HID state. The Protocol Mode and the Suspend state are
 * tracked per Host, and reset to their default values every time a Host
 * connects.
 */
struct hids_conn {
	struct bt_conn *conn;
	enum bt_hid_protocol_mode protocol_mode;
	bool suspended;
};

struct hids_state {
	bool registered;
	const uint8_t *report_map;
	uint16_t report_map_len;
	uint8_t hid_info[HID_INFO_VAL_SIZE];
};

/** @brief Check whether the HID Service is registered.
 *
 * Used by the HOGP Device role to apply the profile level requirements only
 * while a HID Service is present.
 *
 * @return true if bt_hids_register() completed successfully.
 */
bool bt_hids_is_registered(void);

#endif /* BT_HIDS_INTERNAL_H_ */
