/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/bluetooth/services/hogp_device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hogp_device, CONFIG_BT_HOGP_DEVICE_LOG_LEVEL);

#define HIDS_PERM_READ  (BT_GATT_PERM_READ_ENCRYPT)
#define HIDS_PERM_WRITE (BT_GATT_PERM_WRITE_ENCRYPT)
#define HIDS_PERM_RW    (HIDS_PERM_READ | HIDS_PERM_WRITE)

#include "hogp_internal.h"

/* Static UUID variables (must outlive GATT registration) */
static struct bt_uuid_16 uuid_protocol_mode =
	BT_UUID_INIT_16(BT_UUID_HIDS_PROTOCOL_MODE_VAL);
static struct bt_uuid_16 uuid_report_map =
	BT_UUID_INIT_16(BT_UUID_HIDS_REPORT_MAP_VAL);
static struct bt_uuid_16 uuid_report =
	BT_UUID_INIT_16(BT_UUID_HIDS_REPORT_VAL);
static struct bt_uuid_16 uuid_hids_info =
	BT_UUID_INIT_16(BT_UUID_HIDS_INFO_VAL);
static struct bt_uuid_16 uuid_ctrl_point =
	BT_UUID_INIT_16(BT_UUID_HIDS_CTRL_POINT_VAL);
static struct bt_uuid_16 uuid_gatt_primary =
	BT_UUID_INIT_16(BT_UUID_GATT_PRIMARY_VAL);
static struct bt_uuid_16 uuid_gatt_chrc =
	BT_UUID_INIT_16(BT_UUID_GATT_CHRC_VAL);
static struct bt_uuid_16 uuid_gatt_ccc =
	BT_UUID_INIT_16(BT_UUID_GATT_CCC_VAL);

#if defined(CONFIG_BT_HOGP_DEVICE_BOOT)
static struct bt_uuid_16 uuid_boot_kb_in =
	BT_UUID_INIT_16(BT_UUID_HIDS_BOOT_KB_IN_REPORT_VAL);
static struct bt_uuid_16 uuid_boot_kb_out =
	BT_UUID_INIT_16(BT_UUID_HIDS_BOOT_KB_OUT_REPORT_VAL);
static struct bt_uuid_16 uuid_boot_mouse_in =
	BT_UUID_INIT_16(BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT_VAL);
#endif

/* Module context */
static const struct bt_hogp_device_cb *callbacks;
static bool conn_cb_inited;
static struct hogp_device_hid_svc hid_svc;
static struct hogp_device_conn connections[CONFIG_BT_HOGP_DEVICE_MAX_CONNECTIONS];


static int find_report_index(uint8_t report_id, uint8_t report_type)
{
	for (int i = 0; i < hid_svc.reports_cnt; i++) {
		if (hid_svc.reports[i].id == report_id &&
		    hid_svc.reports[i].type == report_type) {
			return i;
		}
	}
	return -1;
}

/* GATT callbacks */

static ssize_t read_protocol_mode(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				&hid_svc.protocol_mode,
				sizeof(hid_svc.protocol_mode));
}

static ssize_t write_protocol_mode(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len,
				   uint16_t offset, uint8_t flags)
{
	const uint8_t *val = buf;

	if (len != 1 || offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (*val > BT_HID_PROTOCOL_REPORT) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	hid_svc.protocol_mode = *val;

	if (callbacks && callbacks->set_protocol) {
		callbacks->set_protocol(conn, hid_svc.protocol_mode);
	}

	return len;
}

static ssize_t read_report_map(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				hid_svc.report_map, hid_svc.report_map_len);
}

static ssize_t read_hid_info(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				hid_svc.hid_info, sizeof(hid_svc.hid_info));
}

static ssize_t write_ctrl_point(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len,
				uint16_t offset, uint8_t flags)
{
	const uint8_t *val = buf;

	if (len != 1 || offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (*val > BT_HOGP_DEVICE_CTRL_EXIT_SUSPEND) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	if (callbacks && callbacks->ctrl_point) {
		callbacks->ctrl_point(conn, *val);
	}

	return len;
}

static ssize_t read_report_ref(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	struct hogp_device_report_ref *ref = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, ref,
				sizeof(*ref));
}

static ssize_t read_report(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	uint8_t idx = *(uint8_t *)attr->user_data;

	if (callbacks && callbacks->get_report) {
		callbacks->get_report(conn, hid_svc.reports[idx].type,
				     hid_svc.reports[idx].id, len);
	}

	/* Phase 1: synchronous response with zero-length (no cached data) */
	return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
}

static ssize_t write_report(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, const void *buf,
			    uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t idx = *(uint8_t *)attr->user_data;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (callbacks && callbacks->set_report) {
		callbacks->set_report(conn, hid_svc.reports[idx].type,
				     hid_svc.reports[idx].id, buf, len);
	}

	return len;
}

#if defined(CONFIG_BT_HOGP_DEVICE_BOOT)
static ssize_t read_boot_report(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
}

static ssize_t write_boot_report(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len,
				 uint16_t offset, uint8_t flags)
{
	if (callbacks && callbacks->set_report) {
		callbacks->set_report(conn, BT_HID_REPORT_TYPE_OUTPUT,
				     0, buf, len);
	}
	return len;
}

static void boot_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("Boot CCC changed: 0x%04x", value);
}
#endif /* CONFIG_BT_HOGP_DEVICE_BOOT */

static void report_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	struct bt_gatt_ccc_managed_user_data *ccc = attr->user_data;
	int idx = (int)(ccc - hid_svc.ccc_data);

	if (idx < 0 || idx >= hid_svc.reports_cnt) {
		return;
	}

	if (value & BT_GATT_CCC_NOTIFY) {
		hid_svc.input_ntf_enabled |= BIT(idx);
	} else {
		hid_svc.input_ntf_enabled &= ~BIT(idx);
	}

	if (callbacks && callbacks->ccc_changed) {
		callbacks->ccc_changed(NULL, hid_svc.reports[idx].id,
				      hid_svc.reports[idx].type,
				      !!(value & BT_GATT_CCC_NOTIFY));
	}
}

/* Attribute table builder */

static int build_hids_attrs(void)
{
	static struct bt_uuid_16 hids_uuid = BT_UUID_INIT_16(BT_UUID_HIDS_VAL);
	static struct bt_uuid_16 report_ref_uuid =
		BT_UUID_INIT_16(BT_UUID_HIDS_REPORT_REF_VAL);
	static struct bt_gatt_chrc prot_mode_chrc;
	static struct bt_gatt_chrc report_map_chrc;
	static struct bt_gatt_chrc report_chrcs[CONFIG_BT_HOGP_DEVICE_MAX_REPORTS];
	static struct bt_gatt_chrc hid_info_chrc;
	static struct bt_gatt_chrc ctrl_point_chrc;
	uint16_t count;
	uint16_t idx = 0;
	uint8_t props;

	count = HIDS_FIXED_ATTR_COUNT + HIDS_BOOT_ATTR_COUNT;
	for (int i = 0; i < hid_svc.reports_cnt; i++) {
		count += 3; /* chrc decl + value + report ref */
		if (hid_svc.reports[i].type == BT_HID_REPORT_TYPE_INPUT) {
			count++; /* CCC descriptor */
		}
	}

	if (count > HOGP_DEVICE_MAX_ATTRS) {
		return -ENOMEM;
	}

	memset(hid_svc.attrs, 0, count * sizeof(struct bt_gatt_attr));

	/* Primary Service */
	hid_svc.attrs[idx++] = (struct bt_gatt_attr)
		BT_GATT_ATTRIBUTE(&uuid_gatt_primary.uuid, BT_GATT_PERM_READ,
				  bt_gatt_attr_read_service, NULL, &hids_uuid);

	/* Protocol Mode */
	prot_mode_chrc = (struct bt_gatt_chrc)BT_GATT_CHRC_INIT(
		&uuid_protocol_mode.uuid, 0,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP);
	hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
		&uuid_gatt_chrc.uuid, BT_GATT_PERM_READ,
		bt_gatt_attr_read_chrc, NULL, &prot_mode_chrc);
	hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
		&uuid_protocol_mode.uuid, HIDS_PERM_RW,
		read_protocol_mode, write_protocol_mode, NULL);

	/* Report Map */
	report_map_chrc = (struct bt_gatt_chrc)BT_GATT_CHRC_INIT(
		&uuid_report_map.uuid, 0, BT_GATT_CHRC_READ);
	hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
		&uuid_gatt_chrc.uuid, BT_GATT_PERM_READ,
		bt_gatt_attr_read_chrc, NULL, &report_map_chrc);
	hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
		&uuid_report_map.uuid, HIDS_PERM_READ,
		read_report_map, NULL, NULL);

	/* Reports */
	for (int i = 0; i < hid_svc.reports_cnt; i++) {
		hid_svc.report_indices[i] = i;
		hid_svc.report_refs[i].id = hid_svc.reports[i].id;
		hid_svc.report_refs[i].type = hid_svc.reports[i].type;

		switch (hid_svc.reports[i].type) {
		case BT_HID_REPORT_TYPE_INPUT:
			props = BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
				BT_GATT_CHRC_NOTIFY;
			break;
		case BT_HID_REPORT_TYPE_OUTPUT:
			props = BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
				BT_GATT_CHRC_WRITE_WITHOUT_RESP;
			break;
		case BT_HID_REPORT_TYPE_FEATURE:
			props = BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE;
			break;
		default:
			props = BT_GATT_CHRC_READ;
			break;
		}

		report_chrcs[i] = (struct bt_gatt_chrc)BT_GATT_CHRC_INIT(
			&uuid_report.uuid, 0, props);
		hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
			&uuid_gatt_chrc.uuid, BT_GATT_PERM_READ,
			bt_gatt_attr_read_chrc, NULL, &report_chrcs[i]);
		hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
			&uuid_report.uuid, HIDS_PERM_RW,
			read_report, write_report,
			&hid_svc.report_indices[i]);

		if (hid_svc.reports[i].type == BT_HID_REPORT_TYPE_INPUT) {
			memset(&hid_svc.ccc_data[i], 0,
			       sizeof(hid_svc.ccc_data[i]));
			hid_svc.ccc_data[i].cfg_changed = report_ccc_changed;
			hid_svc.attrs[idx++] = (struct bt_gatt_attr)
				BT_GATT_ATTRIBUTE(
					&uuid_gatt_ccc.uuid,
					HIDS_PERM_READ | HIDS_PERM_WRITE,
					bt_gatt_attr_read_ccc,
					bt_gatt_attr_write_ccc,
					&hid_svc.ccc_data[i]);
		}

		hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
			&report_ref_uuid.uuid, HIDS_PERM_READ,
			read_report_ref, NULL, &hid_svc.report_refs[i]);
	}

	/* HID Information */
	hid_info_chrc = (struct bt_gatt_chrc)BT_GATT_CHRC_INIT(
		&uuid_hids_info.uuid, 0, BT_GATT_CHRC_READ);
	hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
		&uuid_gatt_chrc.uuid, BT_GATT_PERM_READ,
		bt_gatt_attr_read_chrc, NULL, &hid_info_chrc);
	hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
		&uuid_hids_info.uuid, HIDS_PERM_READ,
		read_hid_info, NULL, NULL);

	/* HID Control Point */
	ctrl_point_chrc = (struct bt_gatt_chrc)BT_GATT_CHRC_INIT(
		&uuid_ctrl_point.uuid, 0, BT_GATT_CHRC_WRITE_WITHOUT_RESP);
	hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
		&uuid_gatt_chrc.uuid, BT_GATT_PERM_READ,
		bt_gatt_attr_read_chrc, NULL, &ctrl_point_chrc);
	hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
		&uuid_ctrl_point.uuid, HIDS_PERM_WRITE,
		NULL, write_ctrl_point, NULL);

#if defined(CONFIG_BT_HOGP_DEVICE_BOOT)
	{
		static struct bt_gatt_chrc boot_kb_in_chrc;
		static struct bt_gatt_chrc boot_kb_out_chrc;
		static struct bt_gatt_chrc boot_mouse_in_chrc;
		static struct bt_gatt_ccc_managed_user_data boot_kb_in_ccc;
		static struct bt_gatt_ccc_managed_user_data boot_mouse_in_ccc;

		/* Boot Keyboard Input Report */
		boot_kb_in_chrc = (struct bt_gatt_chrc)BT_GATT_CHRC_INIT(
			&uuid_boot_kb_in.uuid, 0,
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY);
		hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
			&uuid_gatt_chrc.uuid, BT_GATT_PERM_READ,
			bt_gatt_attr_read_chrc, NULL, &boot_kb_in_chrc);
		hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
			&uuid_boot_kb_in.uuid, HIDS_PERM_READ,
			read_boot_report, NULL, NULL);
		memset(&boot_kb_in_ccc, 0, sizeof(boot_kb_in_ccc));
		boot_kb_in_ccc.cfg_changed = boot_ccc_changed;
		hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
			&uuid_gatt_ccc.uuid,
			HIDS_PERM_READ | HIDS_PERM_WRITE,
			bt_gatt_attr_read_ccc, bt_gatt_attr_write_ccc,
			&boot_kb_in_ccc);

		/* Boot Keyboard Output Report */
		boot_kb_out_chrc = (struct bt_gatt_chrc)BT_GATT_CHRC_INIT(
			&uuid_boot_kb_out.uuid, 0,
			BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
			BT_GATT_CHRC_WRITE_WITHOUT_RESP);
		hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
			&uuid_gatt_chrc.uuid, BT_GATT_PERM_READ,
			bt_gatt_attr_read_chrc, NULL, &boot_kb_out_chrc);
		hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
			&uuid_boot_kb_out.uuid, HIDS_PERM_RW,
			read_boot_report, write_boot_report, NULL);

		/* Boot Mouse Input Report */
		boot_mouse_in_chrc = (struct bt_gatt_chrc)BT_GATT_CHRC_INIT(
			&uuid_boot_mouse_in.uuid, 0,
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY);
		hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
			&uuid_gatt_chrc.uuid, BT_GATT_PERM_READ,
			bt_gatt_attr_read_chrc, NULL, &boot_mouse_in_chrc);
		hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
			&uuid_boot_mouse_in.uuid, HIDS_PERM_READ,
			read_boot_report, NULL, NULL);
		memset(&boot_mouse_in_ccc, 0, sizeof(boot_mouse_in_ccc));
		boot_mouse_in_ccc.cfg_changed = boot_ccc_changed;
		hid_svc.attrs[idx++] = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
			&uuid_gatt_ccc.uuid,
			HIDS_PERM_READ | HIDS_PERM_WRITE,
			bt_gatt_attr_read_ccc, bt_gatt_attr_write_ccc,
			&boot_mouse_in_ccc);
	}
#endif /* CONFIG_BT_HOGP_DEVICE_BOOT */

	hid_svc.attr_count = idx;
	return 0;
}

/* Connection management */

static void hogp_dev_connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;

	if (err || !callbacks) {
		return;
	}

	if (bt_conn_get_info(conn, &info) < 0 || info.type != BT_CONN_TYPE_LE) {
		return;
	}

	for (int i = 0; i < CONFIG_BT_HOGP_DEVICE_MAX_CONNECTIONS; i++) {
		if (connections[i].conn == NULL) {
			connections[i].conn = bt_conn_ref(conn);
			if (callbacks->connected) {
				callbacks->connected(conn);
			}
			return;
		}
	}

	LOG_WRN("No free connection slot");
}

static void hogp_dev_disconnected(struct bt_conn *conn, uint8_t reason)
{
	for (int i = 0; i < CONFIG_BT_HOGP_DEVICE_MAX_CONNECTIONS; i++) {
		struct hogp_device_conn *dc = &connections[i];

		if (dc->conn == conn) {
			bt_conn_unref(dc->conn);
			dc->pending_report_id = 0;
			dc->pending_report_type = 0;
			dc->conn = NULL;
			if (callbacks && callbacks->disconnected) {
				callbacks->disconnected(conn, reason);
			}
			return;
		}
	}
}

static struct bt_conn_cb hogp_dev_conn_cb = {
	.connected = hogp_dev_connected,
	.disconnected = hogp_dev_disconnected,
};

/* Public API */

int bt_hogp_device_register(const struct bt_hogp_device_init_param *param)
{
	int err;

	if (callbacks) {
		LOG_ERR("HOGP Device already registered");
		return -EALREADY;
	}

	if (!param || !param->report_map || param->report_map_len == 0) {
		return -EINVAL;
	}

	if (!param->reports || param->report_count == 0) {
		return -EINVAL;
	}

	if (param->report_map_len > sizeof(hid_svc.report_map)) {
		return -ENOMEM;
	}

	if (param->report_count > CONFIG_BT_HOGP_DEVICE_MAX_REPORTS) {
		return -ENOMEM;
	}

	callbacks = param->cb;
	memcpy(hid_svc.report_map, param->report_map, param->report_map_len);
	hid_svc.report_map_len = param->report_map_len;
	hid_svc.reports_cnt = param->report_count;
	memcpy(hid_svc.reports, param->reports,
	       param->report_count * sizeof(struct bt_hogp_device_report));

	hid_svc.hid_info[HID_INFO_BCDHID_OFFSET] =
		param->info.bcd_hid & 0xFF;
	hid_svc.hid_info[HID_INFO_BCDHID_OFFSET + 1] =
		(param->info.bcd_hid >> 8) & 0xFF;
	hid_svc.hid_info[HID_INFO_COUNTRY_CODE_OFFSET] =
		param->info.b_country_code;
	hid_svc.hid_info[HID_INFO_FLAGS_OFFSET] = param->info.flags;

	hid_svc.protocol_mode = BT_HID_PROTOCOL_REPORT;
	hid_svc.input_ntf_enabled = 0;

	err = build_hids_attrs();
	if (err) {
		callbacks = NULL;
		return err;
	}

	hid_svc.svc.attrs = hid_svc.attrs;
	hid_svc.svc.attr_count = hid_svc.attr_count;

	err = bt_gatt_service_register(&hid_svc.svc);
	if (err) {
		LOG_ERR("Failed to register HIDS: %d", err);
		callbacks = NULL;
		return err;
	}

	if (!conn_cb_inited) {
		bt_conn_cb_register(&hogp_dev_conn_cb);
		conn_cb_inited = true;
	}

	LOG_DBG("HOGP Device registered, %d reports", hid_svc.reports_cnt);
	return 0;
}

void bt_hogp_device_unregister(void)
{
	if (!callbacks) {
		return;
	}

	callbacks = NULL;

	for (int i = 0; i < CONFIG_BT_HOGP_DEVICE_MAX_CONNECTIONS; i++) {
		struct hogp_device_conn *dc = &connections[i];

		if (dc->conn) {
			bt_conn_disconnect(dc->conn,
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
	}

	bt_gatt_service_unregister(&hid_svc.svc);
	hid_svc.attr_count = 0;

	for (int i = 0; i < CONFIG_BT_HOGP_DEVICE_MAX_CONNECTIONS; i++) {
		if (connections[i].conn) {
			bt_conn_unref(connections[i].conn);
		}
	}
	memset(connections, 0, sizeof(connections));

	hid_svc.reports_cnt = 0;
	hid_svc.input_ntf_enabled = 0;
	hid_svc.report_map_len = 0;
}

int bt_hogp_device_disconnect(struct bt_conn *conn)
{
	if (!conn) {
		return -EINVAL;
	}

	return bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

int bt_hogp_device_send_report(struct bt_conn *conn, uint8_t report_id,
			       const uint8_t *data, uint16_t len,
			       bt_gatt_complete_func_t func, void *user_data)
{
	int rpt_idx;

	if (!callbacks) {
		return -ESRCH;
	}

	rpt_idx = find_report_index(report_id, BT_HID_REPORT_TYPE_INPUT);
	if (rpt_idx < 0) {
		return -ENOENT;
	}

	for (uint16_t i = 0; i < hid_svc.attr_count; i++) {
		if (hid_svc.attrs[i].read == read_report &&
		    hid_svc.attrs[i].user_data ==
			    &hid_svc.report_indices[rpt_idx]) {
			struct bt_gatt_notify_params params = {
				.attr = &hid_svc.attrs[i],
				.data = data,
				.len = len,
				.func = func,
				.user_data = user_data,
			};

			return bt_gatt_notify_cb(conn, &params);
		}
	}

	return -ENOENT;
}

int bt_hogp_device_get_report_response(struct bt_conn *conn,
				       uint8_t report_id,
				       uint8_t report_type,
				       const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(report_id);
	ARG_UNUSED(report_type);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	return -ENOTSUP;
}

int bt_hogp_device_report_error(struct bt_conn *conn, uint8_t error)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(error);

	return -ENOTSUP;
}

int bt_hogp_device_virtual_cable_unplug(struct bt_conn *conn)
{
	const bt_addr_le_t *dst;
	bt_addr_le_t peer;

	if (!conn) {
		return -EINVAL;
	}

	dst = bt_conn_get_dst(conn);
	if (!dst) {
		return -ENOENT;
	}

	bt_addr_le_copy(&peer, dst);
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	return bt_unpair(BT_ID_DEFAULT, &peer);
}
