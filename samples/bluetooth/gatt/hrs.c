/** @file
 *  @brief HRS Service sample
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

static struct bt_gatt_ccc_cfg hrmc_ccc_cfg[CONFIG_BLUETOOTH_MAX_PAIRED] = {};
static u8_t simulate_hrm;
static u8_t heartrate = 90;
static u8_t hrs_blsc;

static void hrmc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	simulate_hrm = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_blsc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &hrs_blsc,
				 sizeof(hrs_blsc));
}

/* Heart Rate Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HRS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_MEASUREMENT, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_HRS_MEASUREMENT, BT_GATT_PERM_READ, NULL,
			   NULL, NULL),
	BT_GATT_CCC(hrmc_ccc_cfg, hrmc_ccc_cfg_changed),
	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_BODY_SENSOR, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_HRS_BODY_SENSOR, BT_GATT_PERM_READ,
			   read_blsc, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_CONTROL_POINT, BT_GATT_CHRC_WRITE),
	/* TODO: Add write permission and callback */
	BT_GATT_DESCRIPTOR(BT_UUID_HRS_CONTROL_POINT, BT_GATT_PERM_READ, NULL,
			   NULL, NULL),
};

void hrs_init(u8_t blsc)
{
	hrs_blsc = blsc;

	bt_gatt_register(attrs, ARRAY_SIZE(attrs));
}

void hrs_notify(void)
{
	static u8_t hrm[2];

	/* Heartrate measurements simulation */
	if (!simulate_hrm) {
		return;
	}

	heartrate++;
	if (heartrate == 160) {
		heartrate = 90;
	}

	hrm[0] = 0x06; /* uint8, sensor contact */
	hrm[1] = heartrate;

	bt_gatt_notify(NULL, &attrs[2], &hrm, sizeof(hrm));
}
