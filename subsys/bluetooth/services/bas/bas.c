/** @file
 *  @brief GATT Battery Service
 */

/*
 * Copyright (c) 2024 Demant A/S
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/bas.h>
#include "bas_internal.h"

#define LOG_LEVEL CONFIG_BT_BAS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bas, CONFIG_BT_BAS_LOG_LEVEL);

#define BT_BAS_IDX_BATT_LVL_STATUS_CHAR_VAL 6

/* Notify/Indicate all connections */
struct bt_gatt_indicate_params ind_params;

static bt_bas_data_t bt_bas_data = {
	.battery_level = 100U,
};

static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("BAS Notifications %s", notif_enabled ? "enabled" : "disabled");
}

static void blvl_status_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
	bool ind_enabled = (value == BT_GATT_CCC_INDICATE);

	LOG_INF("BAS Notifications %s", notif_enabled ? "enabled" : "disabled");
	LOG_INF("BAS Indications %s", ind_enabled ? "enabled" : "disabled");
}

static ssize_t read_blvl(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	uint8_t lvl8 = bt_bas_data.battery_level;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &lvl8, sizeof(lvl8));
}

static ssize_t read_blvl_status(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	const bt_bas_bls_t *status = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, status, sizeof(bt_bas_bls_t));
}

static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	if (err != 0) {
		LOG_INF("Indication failed with error %d\n", err);
	} else {
		LOG_INF("Indication sent successfully\n");
	}
}

/* Constant values from the Assigned Numbers specification:
 * https://www.bluetooth.com/wp-content/uploads/Files/Specification/Assigned_Numbers.pdf?id=89
 */
static const struct bt_gatt_cpf level_cpf = {
	.format = 0x04, /* uint8 */
	.exponent = 0x0,
	.unit = 0x27AD,        /* Percentage */
	.name_space = 0x01,    /* Bluetooth SIG */
	.description = 0x0106, /* "main" */
};

BT_GATT_SERVICE_DEFINE(
	bas, BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_blvl, NULL, &bt_bas_data.battery_level),
	BT_GATT_CCC(blvl_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CPF(&level_cpf),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL_STATUS,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ,
			       read_blvl_status, NULL, &bt_bas_data.level_status),
	BT_GATT_CCC(blvl_status_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static int bas_init(void)
{
	bt_bas_bls_init(&bt_bas_data.level_status);
	return 0;
}

uint8_t bt_bas_get_battery_level(void)
{
	return bt_bas_data.battery_level;
}

int bt_bas_set_battery_level(uint8_t level)
{
	int rc;

	if (level > 100U) {
		return -EINVAL;
	}

	bt_bas_data.battery_level = level;

	rc = bt_gatt_notify(NULL, &bas.attrs[1], &level, sizeof(level));

#if defined(CONFIG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT)
	bt_bas_data.level_status.battery_level = level;
	update_battery_level_status();
#endif
	return rc == -ENOTCONN ? 0 : rc;
}

void update_battery_level_status(void)
{
	int err;

	/* Notify/Indicate all connections */
	ind_params.attr = &bas.attrs[BT_BAS_IDX_BATT_LVL_STATUS_CHAR_VAL];
	ind_params.data = &bt_bas_data.level_status;
	ind_params.len = sizeof(bt_bas_bls_t);
	ind_params.func = indicate_cb;
	err = bt_gatt_indicate(NULL, &ind_params);
	if (err) {
		LOG_DBG("Failed to send ntf/ind to all connections (err %d)\n", err);
	}
}

SYS_INIT(bas_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
