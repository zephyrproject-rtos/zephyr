/** @file
 *  @brief DIS Service sample
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

static const char *dis_model;
static const char *dis_manuf;

static ssize_t read_model(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, dis_model,
				 strlen(dis_model));
}

static ssize_t read_manuf(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, dis_manuf,
				 strlen(dis_manuf));
}

/* Device Information Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_model, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_manuf, NULL, NULL),
};

static struct bt_gatt_service dis_svc = BT_GATT_SERVICE(attrs);

void dis_init(const char *model, const char *manuf)
{
	dis_model = model;
	dis_manuf = manuf;

	bt_gatt_service_register(&dis_svc);
}
