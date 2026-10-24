/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/hogp_host.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hogp_host_sample, LOG_LEVEL_INF);

static struct bt_conn *default_conn;

static void hogp_connected(struct bt_conn *conn, int status,
			    uint8_t num_instances, uint8_t protocol_mode)
{
	if (status) {
		LOG_ERR("HOGP connect failed: %d", status);
		return;
	}

	LOG_INF("HOGP connected: %u HID instance(s), protocol=%s",
		num_instances,
		protocol_mode == BT_HID_PROTOCOL_BOOT ? "Boot" : "Report");
}

static void hogp_disconnected(struct bt_conn *conn, int reason)
{
	LOG_INF("HOGP disconnected (reason %d)", reason);
}

static void hogp_report_map(struct bt_conn *conn, uint8_t service_index,
			     const uint8_t *data, uint16_t len)
{
	LOG_INF("Report Map [svc %u]: %u bytes", service_index, len);
	LOG_HEXDUMP_DBG(data, len, "Report Map");
}

static void hogp_input_report(struct bt_conn *conn, uint8_t service_index,
			       uint8_t report_id, const uint8_t *data,
			       uint16_t len)
{
	LOG_INF("Input Report [svc %u, id %u]: %u bytes",
		service_index, report_id, len);
	LOG_HEXDUMP_INF(data, len, "Data");
}

static void hogp_get_report_result(struct bt_conn *conn, uint8_t report_id,
				    uint8_t report_type, const uint8_t *data,
				    uint16_t len, int status)
{
	if (status) {
		LOG_ERR("Get Report failed: %d", status);
		return;
	}
	LOG_INF("Get Report result: id=%u type=%u len=%u",
		report_id, report_type, len);
}

static void hogp_pnp_id(struct bt_conn *conn,
			  const struct bt_hogp_host_pnp_id *id)
{
	LOG_INF("PnP ID: vid_src=%u vid=0x%04x pid=0x%04x ver=0x%04x",
		id->vid_src, id->vid, id->pid, id->version);
}

static void hogp_hid_info(struct bt_conn *conn, uint8_t service_index,
			   uint16_t bcd_hid, uint8_t b_country_code,
			   uint8_t flags)
{
	LOG_INF("HID Info [svc %u]: bcdHID=0x%04x country=%u flags=0x%02x",
		service_index, bcd_hid, b_country_code, flags);
}

static void hogp_battery_level(struct bt_conn *conn, uint8_t bat_index,
				uint8_t level)
{
	LOG_INF("Battery [%u]: %u%%", bat_index, level);
}

static const struct bt_hogp_host_cb hogp_cb = {
	.connected = hogp_connected,
	.disconnected = hogp_disconnected,
	.report_map = hogp_report_map,
	.input_report = hogp_input_report,
	.get_report_result = hogp_get_report_result,
	.pnp_id = hogp_pnp_id,
	.hid_info = hogp_hid_info,
	.battery_level = hogp_battery_level,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			  struct net_buf_simple *ad)
{
	int err;

	if (default_conn) {
		return;
	}

	/* Filter by advertising type and look for HIDS UUID */
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	/* Simple filter: check ad data for HIDS UUID (0x1812) */
	while (ad->len > 1) {
		uint8_t len = net_buf_simple_pull_u8(ad);
		uint8_t ad_type;

		if (len == 0 || len > ad->len) {
			break;
		}

		ad_type = net_buf_simple_pull_u8(ad);
		len--;

		if (ad_type == BT_DATA_UUID16_ALL ||
		    ad_type == BT_DATA_UUID16_SOME) {
			while (len >= 2) {
				uint16_t uuid_val = net_buf_simple_pull_le16(ad);

				len -= 2;
				if (uuid_val == BT_UUID_HIDS_VAL) {
					goto connect;
				}
			}
		}

		net_buf_simple_pull(ad, len);
	}

	return;

connect:
	err = bt_le_scan_stop();
	if (err) {
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &default_conn);
	if (err) {
		LOG_ERR("Create conn failed: %d", err);
		bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed: %d", err);
		bt_conn_unref(default_conn);
		default_conn = NULL;
		bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
		return;
	}

	LOG_INF("Connected, starting security...");
	bt_conn_set_security(conn, BT_SECURITY_L2);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason 0x%02x)", reason);

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}

	bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			      enum bt_security_err err)
{
	if (err) {
		LOG_ERR("Security failed: %d", err);
		return;
	}

	LOG_INF("Security level %u, starting HOGP discovery...", level);

	int ret = bt_hogp_host_connect(conn, BT_HID_PROTOCOL_REPORT,
				       &hogp_cb);
	if (ret) {
		LOG_ERR("HOGP connect failed: %d", ret);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed: %d", err);
		return 0;
	}

	LOG_INF("Bluetooth initialized");

	bt_hogp_host_init();

	LOG_INF("Scanning for HID devices...");
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		LOG_ERR("Scan start failed: %d", err);
		return 0;
	}

	return 0;
}
