/** @file
 *  @brief CTS Service sample
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

static struct bt_gatt_ccc_cfg ct_ccc_cfg[CONFIG_BLUETOOTH_MAX_PAIRED] = {};
static u8_t ct[10];
static u8_t ct_update;

static void ct_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	/* TODO: Handle value */
}

static ssize_t read_ct(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		       void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(ct));
}

static ssize_t write_ct(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset,
			u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(ct)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	ct_update = 1;

	return len;
}

/* Current Time Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CTS),
	BT_GATT_CHARACTERISTIC(BT_UUID_CTS_CURRENT_TIME, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_CTS_CURRENT_TIME,
			   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   read_ct, write_ct, ct),
	BT_GATT_CCC(ct_ccc_cfg, ct_ccc_cfg_changed),
};

static void generate_current_time(u8_t *buf)
{
	u16_t year;

	/* 'Exact Time 256' contains 'Day Date Time' which contains
	 * 'Date Time' - characteristic contains fields for:
	 * year, month, day, hours, minutes and seconds.
	 */

	year = sys_cpu_to_le16(2015);
	memcpy(buf,  &year, 2); /* year */
	buf[2] = 5; /* months starting from 1 */
	buf[3] = 30; /* day */
	buf[4] = 12; /* hours */
	buf[5] = 45; /* minutes */
	buf[6] = 30; /* seconds */

	/* 'Day of Week' part of 'Day Date Time' */
	buf[7] = 1; /* day of week starting from 1 */

	/* 'Fractions 256 part of 'Exact Time 256' */
	buf[8] = 0;

	/* Adjust reason */
	buf[9] = 0; /* No update, change, etc */
}

void cts_init(void)
{
	/* Simulate current time for Current Time Service */
	generate_current_time(ct);

	bt_gatt_register(attrs, ARRAY_SIZE(attrs));
}

void cts_notify(void)
{	/* Current Time Service updates only when time is changed */
	if (!ct_update) {
		return;
	}

	ct_update = 0;
	bt_gatt_notify(NULL, &attrs[3], &ct, sizeof(ct));
}
