/** @file
 *  @brief BAS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

static struct bt_gatt_ccc_cfg  blvl_ccc_cfg[CONFIG_BLUETOOTH_MAX_PAIRED] = {};
static u8_t simulate_blvl;
static u8_t battery = 100;

static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	simulate_blvl = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_blvl(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(*value));
}

/* Battery Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_PERM_READ,
			   read_blvl, NULL, &battery),
	BT_GATT_CCC(blvl_ccc_cfg, blvl_ccc_cfg_changed),
};

static struct bt_gatt_service bas_svc = BT_GATT_SERVICE(attrs);

void bas_init(void)
{
	bt_gatt_service_register(&bas_svc);
}

void bas_notify(void)
{
	if (!simulate_blvl) {
		return;
	}

	battery--;
	if (!battery) {
		/* Software eco battery charger */
		battery = 100;
	}

	bt_gatt_notify(NULL, &attrs[2], &battery, sizeof(battery));
}
