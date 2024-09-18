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
#include <zephyr/sys/util.h>
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

static uint8_t battery_level = 100U;

static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("BAS Notifications %s", notif_enabled ? "enabled" : "disabled");
}

#if defined(CONFIG_BT_BAS_BLS)
static void blvl_status_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
	bool ind_enabled = (value == BT_GATT_CCC_INDICATE);

	LOG_INF("BAS Notifications %s", notif_enabled ? "enabled" : "disabled");
	LOG_INF("BAS Indications %s", ind_enabled ? "enabled" : "disabled");
}
#endif

static ssize_t read_blvl(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	uint8_t lvl8 = battery_level;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &lvl8, sizeof(lvl8));
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
			       BT_GATT_PERM_READ, read_blvl, NULL, NULL),
	BT_GATT_CCC(blvl_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CPF(&level_cpf),
#if defined(CONFIG_BT_BAS_BLS)
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL_STATUS,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_READ, bt_bas_bls_read_blvl_status, NULL, NULL),
	BT_GATT_CCC(blvl_status_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
#endif
);

static int bas_init(void)
{
	if (IS_ENABLED(CONFIG_BT_BAS_BLS)) {
		/* Initialize the Battery Level Status Module */
		bt_bas_bls_init();
		if (IS_ENABLED(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)) {
			/* Set the identifier only if BT_BAS_BLS_IDENTIFIER_PRESENT is defined */
			bt_bas_bls_set_identifier(level_cpf.description);
		}
	}
	return 0;
}

uint8_t bt_bas_get_battery_level(void)
{
	return battery_level;
}

int bt_bas_set_battery_level(uint8_t level)
{
	int rc;

	if (level > 100U) {
		return -EINVAL;
	}

	battery_level = level;

	rc = bt_gatt_notify(NULL, &bas.attrs[1], &level, sizeof(level));

	if (IS_ENABLED(CONFIG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT)) {
		bt_bas_bls_set_battery_level(level);
	}

	return rc == -ENOTCONN ? 0 : rc;
}

const struct bt_gatt_attr *bt_bas_get_bas_attr(uint16_t index)
{
	if (index < bas.attr_count) {
		return &bas.attrs[index];
	}
	return NULL;
}

SYS_INIT(bas_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
