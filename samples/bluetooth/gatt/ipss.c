/** @file
 *  @brief IP Support Service sample
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
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

#define DEVICE_NAME		CONFIG_BLUETOOTH_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)
#define UNKNOWN_APPEARANCE	0x0000

static struct bt_gatt_attr attrs[] = {
	/* IP Support Service Declaration */
	BT_GATT_PRIMARY_SERVICE(BT_UUID_IPSS),
};

static struct bt_gatt_service ipss_svc = BT_GATT_SERVICE(attrs);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x20, 0x18),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
	} else {
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void ipss_init(void)
{
	bt_gatt_service_register(&ipss_svc);

	bt_conn_cb_register(&conn_callbacks);
}

int ipss_advertise(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		return err;
	}

	return 0;
}
