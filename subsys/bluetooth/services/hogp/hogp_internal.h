/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_HOGP_INTERNAL_H_
#define BT_HOGP_INTERNAL_H_

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hogp_device.h>

/* HID Control Point commands (HIDS) */
#define BT_HOGP_DEVICE_CTRL_SUSPEND      0
#define BT_HOGP_DEVICE_CTRL_EXIT_SUSPEND 1

/* HOGP Operation mode (shared by Device and Host) */
#define BT_HOGP_MODE_DEFAULT    0x00U  /* GATT only */
#define BT_HOGP_MODE_SCI        0x01U  /* GATT + SCI */
#define BT_HOGP_MODE_ISO        0x02U  /* GATT + ISO CIS (Hybrid) */
#define BT_HOGP_MODE_ISO_SCI    0x03U  /* ISO CIS + SCI */

#define REPORT_MAP_MAX_SIZE          512
#define HID_INFO_VAL_SIZE            4

#define HID_INFO_BCDHID_OFFSET       0
#define HID_INFO_COUNTRY_CODE_OFFSET 2
#define HID_INFO_FLAGS_OFFSET        3

/* Primary Svc + Protocol Mode(2) + Report Map(2) + HID Info(2) + Ctrl Pt(2) */
#define HIDS_FIXED_ATTR_COUNT 9

/* Boot Protocol: KB Input(3) + KB Output(2) + Mouse Input(3) = 8 attrs */
#if defined(CONFIG_BT_HOGP_DEVICE_BOOT)
#define HIDS_BOOT_ATTR_COUNT 8
#else
#define HIDS_BOOT_ATTR_COUNT 0
#endif

/* Per report: Chrc Decl + Value + CCC (input only) + Report Ref */
#define HIDS_ATTRS_PER_REPORT 4

#define HOGP_DEVICE_MAX_ATTRS \
	(HIDS_FIXED_ATTR_COUNT + HIDS_BOOT_ATTR_COUNT + \
	 HIDS_ATTRS_PER_REPORT * CONFIG_BT_HOGP_DEVICE_MAX_REPORTS)

struct hogp_device_report_ref {
	uint8_t id;
	uint8_t type;
};

struct hogp_device_conn {
	struct bt_conn *conn;
	uint8_t current_mode;
};

struct hogp_device_hid_svc {
	struct bt_gatt_attr attrs[HOGP_DEVICE_MAX_ATTRS];
	uint16_t attr_count;
	struct bt_gatt_service svc;
	uint8_t protocol_mode;
	bool suspended;
	uint8_t report_map[REPORT_MAP_MAX_SIZE];
	uint16_t report_map_len;
	uint8_t hid_info[HID_INFO_VAL_SIZE];
	struct bt_hogp_device_report reports[CONFIG_BT_HOGP_DEVICE_MAX_REPORTS];
	uint8_t reports_cnt;
	uint8_t report_indices[CONFIG_BT_HOGP_DEVICE_MAX_REPORTS];
	struct hogp_device_report_ref report_refs[CONFIG_BT_HOGP_DEVICE_MAX_REPORTS];
	struct bt_gatt_ccc_managed_user_data ccc_data[CONFIG_BT_HOGP_DEVICE_MAX_REPORTS];
};

#endif /* BT_HOGP_INTERNAL_H_ */
