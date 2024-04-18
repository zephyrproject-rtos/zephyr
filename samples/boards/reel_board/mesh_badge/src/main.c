/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <string.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/drivers/sensor.h>

#include "mesh.h"
#include "board.h"

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static ssize_t read_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	const char *value = bt_get_name();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

static ssize_t write_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset,
			  uint8_t flags)
{
	char name[CONFIG_BT_DEVICE_NAME_MAX];
	int err;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len >= CONFIG_BT_DEVICE_NAME_MAX) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(name, buf, len);
	name[len] = '\0';

	err = bt_set_name(name);
	if (err) {
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	board_refresh_display();

	return len;
}

static const struct bt_uuid_128 name_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0));

static const struct bt_uuid_128 name_enc_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

#define CPF_FORMAT_UTF8 0x19

static const struct bt_gatt_cpf name_cpf = {
	.format = CPF_FORMAT_UTF8,
};

/* Vendor Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(name_svc,
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&name_uuid),
	BT_GATT_CHARACTERISTIC(&name_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT,
			       read_name, write_name, NULL),
	BT_GATT_CUD("Badge Name", BT_GATT_PERM_READ),
	BT_GATT_CPF(&name_cpf),
);

static void passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char buf[20];

	snprintk(buf, sizeof(buf), "Passkey:\n%06u", passkey);

	printk("%s\n", buf);
	board_show_text(buf, false, K_FOREVER);
}

static void passkey_cancel(struct bt_conn *conn)
{
	printk("Cancel\n");
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	printk("Pairing Complete\n");
	board_show_text("Pairing Complete", false, K_SECONDS(2));
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	printk("Pairing Failed (%d)\n", reason);
	board_show_text("Pairing Failed", false, K_SECONDS(2));
}

const struct bt_conn_auth_cb auth_cb = {
	.passkey_display = passkey_display,
	.cancel = passkey_cancel,
};

static struct bt_conn_auth_info_cb auth_info_cb = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	printk("Connected (err 0x%02x)\n", err);

	if (err) {
		board_show_text("Connection failed", false, K_SECONDS(2));
	} else {
		board_show_text("Connected", false, K_FOREVER);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);

	if (strcmp(CONFIG_BT_DEVICE_NAME, bt_get_name()) &&
	    !mesh_is_initialized()) {
		/* Mesh will take over advertising control */
		bt_le_adv_stop();
		mesh_start();
	} else {
		board_show_text("Disconnected", false, K_SECONDS(2));
	}
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = mesh_init();
	if (err) {
		printk("Initializing mesh failed (err %d)\n", err);
		return;
	}

	printk("Mesh initialized\n");

	bt_conn_auth_cb_register(&auth_cb);
	bt_conn_auth_info_cb_register(&auth_info_cb);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	if (!mesh_is_initialized()) {
		/* Start advertising */
		err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
		if (err) {
			printk("Advertising failed to start (err %d)\n", err);
			return;
		}
	} else {
		printk("Already provisioned\n");
	}

	board_refresh_display();

	printk("Board started\n");
}

int main(void)
{
	int err;

	err = board_init();
	if (err) {
		printk("board init failed (err %d)\n", err);
		return 0;
	}

	printk("Starting Board Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	err = periphs_init();
	if (err) {
		printk("peripherals initialization failed (err %d)\n", err);
		return 0;
	}
	return 0;
}
