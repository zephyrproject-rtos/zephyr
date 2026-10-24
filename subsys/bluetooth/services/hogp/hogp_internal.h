/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HOGP_INTERNAL_H_
#define HOGP_INTERNAL_H_

#include <zephyr/bluetooth/services/hogp_device.h>
#if defined(CONFIG_BT_HOGP_HOST)
#include <zephyr/bluetooth/services/hogp_host.h>
#endif
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>

/* HID Control Point commands (HIDS) */
#define BT_HOGP_DEVICE_CTRL_SUSPEND      0
#define BT_HOGP_DEVICE_CTRL_EXIT_SUSPEND 1

/* HOGP Operation mode (shared by Device and Host) */
#define BT_HOGP_MODE_DEFAULT    0x00  /* GATT only */
#define BT_HOGP_MODE_SCI        0x01  /* GATT + SCI */
#define BT_HOGP_MODE_ISO        0x02  /* GATT + ISO CIS (Hybrid) */
#define BT_HOGP_MODE_ISO_SCI    0x03  /* ISO CIS + SCI */

#if defined(CONFIG_BT_HOGP_DEVICE)
/* Device internal definitions */

#define REPORT_MAP_MAX_SIZE          512
#define HID_INFO_VAL_SIZE            4

#define HID_INFO_BCDHID_OFFSET       0
#define HID_INFO_COUNTRY_CODE_OFFSET 2
#define HID_INFO_FLAGS_OFFSET        3

#define HIDS_FIXED_ATTR_COUNT 9

/* Boot Protocol: KB Input(3) + KB Output(2) + Mouse Input(3) = 8 attrs */
#if defined(CONFIG_BT_HOGP_DEVICE_BOOT)
#define HIDS_BOOT_ATTR_COUNT 8
#else
#define HIDS_BOOT_ATTR_COUNT 0
#endif

#define HOGP_DEVICE_MAX_ATTRS \
	(HIDS_FIXED_ATTR_COUNT + HIDS_BOOT_ATTR_COUNT + \
	 4 * CONFIG_BT_HOGP_DEVICE_MAX_REPORTS)

struct hogp_device_report_ref {
	uint8_t id;
	uint8_t type;
};

struct hogp_device_conn {
	struct bt_conn *conn;
	uint8_t current_mode;
	uint8_t pending_report_id;
	uint8_t pending_report_type;
	/* ISO/SCI fields reserved for Phase 2 */
};

struct hogp_device_hid_svc {
	struct bt_gatt_attr attrs[HOGP_DEVICE_MAX_ATTRS];
	uint16_t attr_count;
	struct bt_gatt_service svc;
	uint8_t protocol_mode;
	uint8_t report_map[REPORT_MAP_MAX_SIZE];
	uint16_t report_map_len;
	uint8_t hid_info[HID_INFO_VAL_SIZE];
	struct bt_hogp_device_report reports[CONFIG_BT_HOGP_DEVICE_MAX_REPORTS];
	uint8_t reports_cnt;
	uint8_t report_indices[CONFIG_BT_HOGP_DEVICE_MAX_REPORTS];
	struct hogp_device_report_ref report_refs[CONFIG_BT_HOGP_DEVICE_MAX_REPORTS];
	uint8_t input_ntf_enabled;
	struct bt_gatt_ccc_managed_user_data ccc_data[CONFIG_BT_HOGP_DEVICE_MAX_REPORTS];
};
#endif /* CONFIG_BT_HOGP_DEVICE */

#if defined(CONFIG_BT_HOGP_HOST)
/* Host internal definitions */

#define BT_HOGP_HOST_MAX_EXT_REPORTS 4
#define BT_HOGP_HOST_MAX_BAT_INSTANCES 3

enum hogp_host_state {
	HOGP_HOST_STATE_IDLE = 0,
	HOGP_HOST_STATE_DISCOVER_SERVICE,
	HOGP_HOST_STATE_DISCOVER_CHARS,
	HOGP_HOST_STATE_SET_PROTOCOL_MODE,
	HOGP_HOST_STATE_READ_HID_INFO,
	HOGP_HOST_STATE_READ_REPORT_MAP,
	HOGP_HOST_STATE_DISCOVER_REPORT_MAP_DESCS,
	HOGP_HOST_STATE_READ_EXT_REF_UUID,
	HOGP_HOST_STATE_DISCOVER_EXT_CHARS,
	HOGP_HOST_STATE_FIND_REPORT_DESCS,
	HOGP_HOST_STATE_READ_REPORT_ID_TYPE,
	HOGP_HOST_STATE_ENABLE_NOTIFICATIONS,
	HOGP_HOST_STATE_DISCOVER_DIS,
	HOGP_HOST_STATE_DISCOVER_DIS_CHARS,
	HOGP_HOST_STATE_READ_PNP_ID,
	HOGP_HOST_STATE_DISCOVER_BAS,
	HOGP_HOST_STATE_DISCOVER_BAS_CHARS,
	HOGP_HOST_STATE_DISCOVER_BAS_DESCS,
	HOGP_HOST_STATE_READ_BAT_LEVEL,
	HOGP_HOST_STATE_ENABLE_BAT_NOTIFY,
	HOGP_HOST_STATE_CONNECTED,
	HOGP_HOST_STATE_GET_REPORT,
	HOGP_HOST_STATE_SET_REPORT,
};

struct hogp_host_report {
	uint16_t value_handle;
	uint16_t end_handle;
	uint8_t properties;
	uint8_t service_index;
	uint8_t report_id;
	uint8_t report_type;
	bool boot_report;
	uint16_t ccc_handle;
	struct bt_gatt_subscribe_params sub_params;
};

struct hogp_host_ext_report {
	uint16_t desc_handle;
	uint16_t ext_ref_uuid;
	uint8_t service_index;
};

struct hogp_host_hid_svc {
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t report_map_handle;
	uint16_t report_map_end_handle;
	uint16_t hid_info_handle;
	uint16_t ctrl_point_handle;
	uint16_t protocol_mode_handle;
	uint8_t protocol_mode;
	uint16_t bcd_hid;
	uint8_t b_country_code;
	uint8_t hid_flags;
	uint16_t desc_offset;
	uint16_t desc_len;
};

struct hogp_host_bat {
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t level_handle;
	uint16_t ccc_handle;
	struct bt_gatt_subscribe_params sub_params;
};

struct hogp_host_conn {
	struct bt_conn *conn;
	const struct bt_hogp_host_cb *cb;
	enum hogp_host_state state;
	uint8_t required_protocol_mode;

	struct hogp_host_hid_svc services[CONFIG_BT_HOGP_HOST_MAX_SERVICES];
	uint8_t num_instances;

	struct hogp_host_report reports[CONFIG_BT_HOGP_HOST_MAX_REPORTS];
	uint8_t num_reports;

	struct hogp_host_ext_report ext_reports[BT_HOGP_HOST_MAX_EXT_REPORTS];
	uint8_t num_ext_reports;

	/* DIS */
	uint16_t dis_start_handle;
	uint16_t dis_end_handle;
	uint16_t pnp_id_handle;

	/* Battery Service */
	struct hogp_host_bat bats[BT_HOGP_HOST_MAX_BAT_INSTANCES];
	uint8_t num_bats;
	uint8_t bat_idx;

	uint8_t svc_idx;
	uint8_t rpt_idx;
	uint16_t tmp_handle;

	struct bt_gatt_discover_params disc_params;
	struct bt_gatt_read_params read_params;
	struct bt_gatt_write_params write_params;

	uint16_t rmap_read_offset;

	uint8_t desc_storage[CONFIG_BT_HOGP_HOST_DESC_STORAGE_LEN];
	uint16_t desc_used;

	/* Mode state */
	uint8_t current_mode;

	/* ISO/SCI fields reserved for Phase 2 */
};
#endif /* CONFIG_BT_HOGP_HOST */

#endif /* HOGP_INTERNAL_H_ */
