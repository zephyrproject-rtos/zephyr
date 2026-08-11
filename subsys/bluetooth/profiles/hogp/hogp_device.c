/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hogp_device.h>
#include <zephyr/bluetooth/services/hids.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "services/hids/hids_internal.h"

LOG_MODULE_REGISTER(bt_hogp_device, CONFIG_BT_HOGP_DEVICE_LOG_LEVEL);

/* HOGP v1.1, Section 7.1: the HID Device should use the Peripheral Security
 * Request procedure to inform the HID Host of its security requirements. All
 * HID Service characteristics require an encrypted link.
 */
static void hogp_device_request_security(struct bt_conn *conn)
{
	int err;

	if (bt_conn_get_security(conn) >= BT_SECURITY_L2) {
		return;
	}

	err = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (err != 0) {
		LOG_WRN("Failed to request security (err %d)", err);
	}
}

static void hogp_device_connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;

	if (err != 0U || !bt_hids_is_registered()) {
		return;
	}

	/* HOGP v1.1, Section 2.4: the HID Device uses the GAP Peripheral role */
	if (bt_conn_get_info(conn, &info) < 0 || info.type != BT_CONN_TYPE_LE ||
	    info.role != BT_CONN_ROLE_PERIPHERAL) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_HOGP_DEVICE_SECURITY_REQUEST)) {
		hogp_device_request_security(conn);
	}
}

BT_CONN_CB_DEFINE(hogp_device_conn_cb) = {
	.connected = hogp_device_connected,
};

int bt_hogp_device_register(const struct bt_hogp_device_register_param *param)
{
	int err;

	if (param == NULL) {
		return -EINVAL;
	}

	err = bt_hids_register(&param->hids);
	if (err != 0) {
		LOG_ERR("Failed to register the HID Service (err %d)", err);
		return err;
	}

	LOG_DBG("HOGP Device registered");

	return 0;
}

int bt_hogp_device_unregister(void)
{
	return bt_hids_unregister();
}

int bt_hogp_device_send_report(struct bt_conn *conn, uint8_t report_id,
			       const uint8_t *data, uint16_t len,
			       bt_gatt_complete_func_t func, void *user_data)
{
	return bt_hids_send_report(conn, report_id, data, len, func, user_data);
}

int bt_hogp_device_get_protocol_mode(struct bt_conn *conn,
				     enum bt_hid_protocol_mode *mode)
{
	return bt_hids_get_protocol_mode(conn, mode);
}

int bt_hogp_device_get_suspend_state(struct bt_conn *conn, bool *suspended)
{
	return bt_hids_get_suspend_state(conn, suspended);
}
