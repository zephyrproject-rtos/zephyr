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
#include <bluetooth/bas.h>

static struct bt_gatt_ccc_cfg  blvl_ccc_cfg[BT_GATT_CCC_MAX] = {};
static bt_bas_subscribe_func_t subscribe_func;
static u8_t subscribed_blvl;

static u8_t blvl;

static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	subscribed_blvl = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;

	if (subscribe_func) {
		subscribe_func(subscribed_blvl);
	}
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
			   read_blvl, NULL, &blvl),
	BT_GATT_CCC(blvl_ccc_cfg, blvl_ccc_cfg_changed),
};

static struct bt_gatt_service bas_svc = BT_GATT_SERVICE(attrs);

int bt_bas_register(u8_t level, bt_bas_subscribe_func_t func)
{
	int err;

	err = bt_gatt_service_register(&bas_svc);
	if (err < 0) {
		return err;
	}

	blvl = level;
	subscribe_func = func;

	return 0;
}

void bt_bas_unregister(void)
{
	subscribe_func = NULL;
	bt_gatt_service_unregister(&bas_svc);
}

int bt_bas_set_level(u8_t level)
{
	if (level > 100) {
		return -EINVAL;
	}

	blvl = level;

	if (!subscribed_blvl) {
		return 0;
	}

	return bt_gatt_notify(NULL, &attrs[2], &level, sizeof(level));
}

int bt_bas_simulate(void)
{
	if (!subscribed_blvl) {
		return 0;
	}

	blvl--;
	if (!blvl) {
		/* Software eco blvl charger */
		blvl = 100;
	}

	return bt_gatt_notify(NULL, &attrs[2], &blvl, sizeof(blvl));
}
