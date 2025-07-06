/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/gatt.h>
#include "bas_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bas, CONFIG_BT_BAS_LOG_LEVEL);

#define BATTERY_CRITICAL_STATUS_CHAR_IDX 9

static uint8_t battery_critical_status;

void bt_bas_bcs_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool ind_enabled = (value == BT_GATT_CCC_INDICATE);

	LOG_DBG("BAS Critical status Indication %s", ind_enabled ? "enabled" : "disabled");
}

ssize_t bt_bas_bcs_read_critical_status(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					void *buf, uint16_t len, uint16_t offset)
{

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &battery_critical_status,
				 sizeof(uint8_t));
}

static void bcs_indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params,
			    uint8_t err)
{
	if (err != 0) {
		LOG_DBG("BCS Indication failed with error %u\n", err);
	} else {
		LOG_DBG("BCS Indication sent successfully\n");
	}
}

static void bt_bas_bcs_update_battery_critical_status(void)
{
	/* Indicate all connections */
	const struct bt_gatt_attr *attr = bt_bas_get_bas_attr(BATTERY_CRITICAL_STATUS_CHAR_IDX);

	if (attr) {
		uint8_t err;
		/* Indicate battery critical status to all connections */
		static struct bt_gatt_indicate_params bcs_ind_params;

		bcs_ind_params.attr = attr;
		bcs_ind_params.data = &battery_critical_status;
		bcs_ind_params.len = sizeof(battery_critical_status);
		bcs_ind_params.func = bcs_indicate_cb;
		err = bt_gatt_indicate(NULL, &bcs_ind_params);
		if (err && err != -ENOTCONN) {
			LOG_DBG("Failed to send critical status ind to all conns (err %d)\n", err);
		}
	}
}

void bt_bas_bcs_set_battery_critical_state(bool critical_state)
{
	bool current_state = (battery_critical_status & BT_BAS_BCS_BATTERY_CRITICAL_STATE) != 0;

	if (current_state == critical_state) {
		LOG_DBG("Already battery_critical_state is %d\n", critical_state);
		return;
	}

	if (critical_state) {
		battery_critical_status |= BT_BAS_BCS_BATTERY_CRITICAL_STATE;
	} else {
		battery_critical_status &= ~BT_BAS_BCS_BATTERY_CRITICAL_STATE;
	}
	bt_bas_bcs_update_battery_critical_status();
}

void bt_bas_bcs_set_immediate_service_required(bool service_required)
{
	bool current_state = (battery_critical_status & BT_BAS_BCS_IMMEDIATE_SERVICE_REQUIRED) != 0;

	if (current_state == service_required) {
		LOG_DBG("Already immediate_service_required is %d\n", service_required);
		return;
	}

	if (service_required) {
		battery_critical_status |= BT_BAS_BCS_IMMEDIATE_SERVICE_REQUIRED;
	} else {
		battery_critical_status &= ~BT_BAS_BCS_IMMEDIATE_SERVICE_REQUIRED;
	}
	bt_bas_bcs_update_battery_critical_status();
}
