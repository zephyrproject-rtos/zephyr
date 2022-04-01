/** @file
 *  @brief Immediate Alert Service implementation
 */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/init.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/net/buf.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/services/ias.h>

#define LOG_LEVEL CONFIG_BT_IAS_LOG_LEVEL
LOG_MODULE_REGISTER(ias);

#define BT_IAS_ALERT_LVL_LEN 1

#if defined(CONFIG_BT_IAS_SEC_AUTH)
#define IAS_ALERT_LEVEL_PERM BT_GATT_PERM_WRITE_AUTHEN
#elif defined(CONFIG_BT_IAS_SEC_ENC)
#define IAS_ALERT_LEVEL_PERM BT_GATT_PERM_WRITE_ENCRYPT
#else
#define IAS_ALERT_LEVEL_PERM BT_GATT_PERM_WRITE
#endif

struct alerting_device {
	enum bt_ias_alert_lvl alert_level;
};

static struct alerting_device devices[CONFIG_BT_MAX_CONN];
static enum bt_ias_alert_lvl curr_lvl;

static void set_alert_level(void)
{
	enum bt_ias_alert_lvl alert_level;

	alert_level = devices[0].alert_level;
	for (int i = 1; i < CONFIG_BT_MAX_CONN; i++) {
		if (alert_level < devices[i].alert_level) {
			alert_level = devices[i].alert_level;
		}
	}

	if (curr_lvl == alert_level) {
		return;
	}

	if (alert_level == BT_IAS_ALERT_LVL_HIGH_ALERT) {
		STRUCT_SECTION_FOREACH(bt_ias_cb, cb) {
			if (cb->high_alert) {
				cb->high_alert();
			}
		}
		LOG_DBG("High alert");
	} else if (alert_level == BT_IAS_ALERT_LVL_MILD_ALERT) {
		STRUCT_SECTION_FOREACH(bt_ias_cb, cb) {
			if (cb->mild_alert) {
				cb->mild_alert();
			}
		}
		LOG_DBG("Mild alert");
	} else {
		STRUCT_SECTION_FOREACH(bt_ias_cb, cb) {
			if (cb->no_alert) {
				cb->no_alert();
			}
		}
		LOG_DBG("No alert");
	}
	curr_lvl = alert_level;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	devices[bt_conn_index(conn)].alert_level = BT_IAS_ALERT_LVL_NO_ALERT;
	set_alert_level();
}

int bt_ias_local_alert_stop(void)
{
	if (curr_lvl == BT_IAS_ALERT_LVL_NO_ALERT) {
		return -EALREADY;
	}

	for (int idx = 0; idx < CONFIG_BT_MAX_CONN; idx++) {
		devices[idx].alert_level = BT_IAS_ALERT_LVL_NO_ALERT;
	}
	curr_lvl = BT_IAS_ALERT_LVL_NO_ALERT;
	set_alert_level();

	return 0;
}

static ssize_t bt_ias_write_alert_lvl(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct net_buf_simple data;
	enum bt_ias_alert_lvl alert_val;

	if (offset > 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != BT_IAS_ALERT_LVL_LEN) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	net_buf_simple_init_with_data(&data, (void *)buf, len);
	alert_val = net_buf_simple_pull_u8(&data);
	devices[bt_conn_index(conn)].alert_level = alert_val;

	if (alert_val < BT_IAS_ALERT_LVL_NO_ALERT || alert_val > BT_IAS_ALERT_LVL_HIGH_ALERT) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}
	set_alert_level();

	return len;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

/* Immediate Alert Service Declaration */
BT_GATT_SERVICE_DEFINE(ias_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_IAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_ALERT_LEVEL, BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       IAS_ALERT_LEVEL_PERM, NULL,
			       bt_ias_write_alert_lvl, NULL));
