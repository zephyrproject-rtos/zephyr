/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <logging/log.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "peripherals/sensors.h"

LOG_MODULE_REGISTER(app_bt, LOG_LEVEL_DBG);

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};

static ssize_t read_temperature(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, u16_t len, u16_t offset);

static ssize_t read_humidity(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, u16_t len, u16_t offset);

static ssize_t write_temperature(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf, u16_t len, u16_t offset,
				 u8_t flags);

static struct bt_gatt_attr logger_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),

	/* Internal temperature */
	BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       read_temperature, NULL, NULL),
	BT_GATT_CUD("Temperature", BT_GATT_PERM_READ),

	/* Internal humidity */
	BT_GATT_CHARACTERISTIC(BT_UUID_HUMIDITY,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       read_humidity, NULL, NULL),
	BT_GATT_CUD("Humidity", BT_GATT_PERM_READ),

	/* External temperature */
	BT_GATT_CHARACTERISTIC(BT_UUID_HEAT_INDEX,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_WRITE,
			       NULL, write_temperature, NULL),
	BT_GATT_CUD("External Temperature", BT_GATT_PERM_READ),
};

static struct bt_gatt_service logger_svc = BT_GATT_SERVICE(logger_attrs);


static ssize_t read_temperature(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, u16_t len, u16_t offset)
{
	s16_t value = (s16_t)sensors_get_temperature();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

static ssize_t read_humidity(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, u16_t len, u16_t offset)
{
	s16_t value = (s16_t)sensors_get_humidity();

	value = value > 100 ? 255 : value;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

static ssize_t write_temperature(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf, u16_t len, u16_t offset,
				 u8_t flags)
{
	const s16_t *val = buf;
	s16_t temperature = *val;

	if (len != 2) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	LOG_DBG("Received temperature = %d", temperature);
	sensors_set_temperature_external(temperature);

	bt_gatt_notify(conn, &logger_attrs[4], buf, sizeof(*val));

	return len;
}


static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
	} else {
		LOG_WRN("Connected");
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	LOG_WRN("Disconnected (reason %u)", reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x1A, 0x18, 0x0f, 0x18),
};

static void bt_ready(int err)
{
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");

	err = bt_gatt_service_register(&logger_svc);
	if (err) {
		LOG_ERR("Services not registered (err %d)", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_INF("Advertising successfully started");
}

int app_bt_init(void)
{
	int err = bt_enable(bt_ready);

	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);

	return 0;
}



