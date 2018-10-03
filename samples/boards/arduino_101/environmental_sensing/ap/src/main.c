/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <device.h>
#include <ipm.h>
#include <ipm/ipm_quark_se.h>
#include <misc/byteorder.h>
#include <misc/printk.h>
#include <sensor.h>
#include <zephyr.h>

#define TEMPERATURE_CUD			"Temperature"
#define HUMIDITY_CUD			"Humidity"
#define PRESSURE_CUD			"Pressure"

QUARK_SE_IPM_DEFINE(ess_ipm, 0, QUARK_SE_IPM_INBOUND);

static s16_t temp_value;
static u16_t humidity_value;
static u32_t pressure_value;

static ssize_t read_u16(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, u16_t len, u16_t offset)
{
	const u16_t *u16 = attr->user_data;
	u16_t value = sys_cpu_to_le16(*u16);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

static ssize_t read_u32(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, u16_t len, u16_t offset)
{
	const u32_t *u32 = attr->user_data;
	u32_t value = sys_cpu_to_le32(*u32);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),

	BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_u16, NULL, &temp_value),
	BT_GATT_CUD(TEMPERATURE_CUD, BT_GATT_PERM_READ),

	BT_GATT_CHARACTERISTIC(BT_UUID_HUMIDITY, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_u16, NULL,
			       &humidity_value),
	BT_GATT_CUD(HUMIDITY_CUD, BT_GATT_PERM_READ),

	BT_GATT_CHARACTERISTIC(BT_UUID_PRESSURE, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_u32, NULL,
			       &pressure_value),
	BT_GATT_CUD(PRESSURE_CUD, BT_GATT_PERM_READ),
};

static struct bt_gatt_service env_svc = BT_GATT_SERVICE(attrs);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_gatt_service_register(&env_svc);

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
}

static void sensor_ipm_callback(void *context, u32_t id, volatile void *data)
{
	volatile struct sensor_value *val = data;

	switch (id) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* resolution of 0.01 degrees Celsius */
		temp_value = val->val1 * 100 + val->val2 / 10000;
		break;
	case SENSOR_CHAN_HUMIDITY:
		/* resolution of 0.01 percent */
		humidity_value = val->val1 * 100 + val->val2 / 10000;
		break;
	case SENSOR_CHAN_PRESS:
		/* resolution of 0.1 Pa */
		pressure_value = val->val1 * 10000 + val->val2 / 100;
		break;
	default:
		break;
	}
}

void main(void)
{
	struct device *ipm;
	int rc;

	rc = bt_enable(bt_ready);
	if (rc) {
		printk("Bluetooth init failed (err %d)\n", rc);
		return;
	}

	ipm = device_get_binding("ess_ipm");
	ipm_register_callback(ipm, sensor_ipm_callback, NULL);
	ipm_set_enabled(ipm, 1);

	k_sleep(K_FOREVER);
}
