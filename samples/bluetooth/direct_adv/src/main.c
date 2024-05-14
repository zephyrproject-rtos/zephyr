/*
 * Copyright (c) 2022 Michal Morsisko
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic_builtin.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

/* Custom Service Variables */
#define BT_UUID_CUSTOM_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

static const struct bt_uuid_128 primary_service_uuid = BT_UUID_INIT_128(
	BT_UUID_CUSTOM_SERVICE_VAL);

static const struct bt_uuid_128 read_characteristic_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

static const struct bt_uuid_128 write_characteristic_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2));

static int signed_value;
static struct bt_le_adv_param adv_param;
static bt_addr_le_t bond_addr;

static ssize_t read_signed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   void *buf, uint16_t len, uint16_t offset)
{
	int *value = &signed_value;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(signed_value));
}

static ssize_t write_signed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    const void *buf, uint16_t len, uint16_t offset,
			    uint8_t flags)
{
	int *value = &signed_value;

	if (offset + len > sizeof(signed_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/* Vendor Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(primary_service,
	BT_GATT_PRIMARY_SERVICE(&primary_service_uuid),
	BT_GATT_CHARACTERISTIC(&read_characteristic_uuid.uuid,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_signed, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&write_characteristic_uuid.uuid,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       NULL, write_signed, NULL),
);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static atomic_t undirected_adv_enabled;
static struct k_sem try_start_undirected_adv_req;

static void try_start_undirected_adv(void)
{
	int err;
	struct bt_le_adv_param param;

	param = *BT_LE_ADV_CONN;
	param.options |= BT_LE_ADV_OPT_ONE_TIME;

	err = bt_le_adv_start(&param, ad, ARRAY_SIZE(ad), NULL, 0);

	if (err == -EALREADY || err == -ENOMEM) {
		/* Already running, or all connection slots taken.
		 * We must try again when upon advertiser termination
		 * (proxied by the connected callback) and the conn
		 * recycled event.
		 */
	} else if (err) {
		printk("Advertising failed to start (err %d)\n", err);
	} else {
		printk("Advertising successfully started\n");
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		printk("Connected\n");
	}

	/* The legacy advertiser terminated. */
	if (atomic_get(&undirected_adv_enabled)) {
		k_sem_give(&try_start_undirected_adv_req);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
}

static void on_conn_recycled(void)
{
	if (atomic_get(&undirected_adv_enabled)) {
		k_sem_give(&try_start_undirected_adv_req);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = on_conn_recycled,
};

static void copy_last_bonded_addr(const struct bt_bond_info *info, void *data)
{
	bt_addr_le_copy(&bond_addr, &info->addr);
}

static void bt_ready(void)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	bt_addr_le_copy(&bond_addr, BT_ADDR_LE_NONE);
	bt_foreach_bond(BT_ID_DEFAULT, copy_last_bonded_addr, NULL);

	/* Address is equal to BT_ADDR_LE_NONE if compare returns 0.
	 * This means there is no bond yet.
	 */
	if (bt_addr_le_cmp(&bond_addr, BT_ADDR_LE_NONE) != 0) {
		bt_addr_le_to_str(&bond_addr, addr, sizeof(addr));
		printk("Direct advertising to %s\n", addr);

		adv_param = *BT_LE_ADV_CONN_DIR_LOW_DUTY(&bond_addr);
		adv_param.options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;
		err = bt_le_adv_start(&adv_param, NULL, 0, NULL, 0);

		if (err) {
			printk("Advertising failed to start (err %d)\n", err);
		} else {
			printk("Advertising successfully started\n");
		}
	} else {
		atomic_set(&undirected_adv_enabled, true);
		k_sem_give(&try_start_undirected_adv_req);
	}
}

void pairing_complete(struct bt_conn *conn, bool bonded)
{
	printk("Pairing completed. Rebooting in 5 seconds...\n");

	k_sleep(K_SECONDS(5));
	sys_reboot(SYS_REBOOT_WARM);
}

static struct bt_conn_auth_info_cb bt_conn_auth_info = {
	.pairing_complete = pairing_complete
};

int main(void)
{
	int err;

	k_sem_init(&try_start_undirected_adv_req, 0, 1);

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	bt_ready();
	bt_conn_auth_info_cb_register(&bt_conn_auth_info);

	while (1) {
		k_sem_take(&try_start_undirected_adv_req, K_FOREVER);
		try_start_undirected_adv();
	}
	return 0;
}
