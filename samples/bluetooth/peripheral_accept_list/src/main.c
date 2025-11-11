/*
 * Copyright (c) 2022 Michal Morsisko
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

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
static int bond_count;

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
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
	} else {
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected
};

static void add_bonded_addr_to_filter_list(const struct bt_bond_info *info, void *data)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_le_filter_accept_list_add(&info->addr);
	bt_addr_le_to_str(&info->addr, addr_str, sizeof(addr_str));
	printk("Added %s to advertising accept filter list\n", addr_str);
	bond_count++;
}

static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	bond_count = 0;
	bt_foreach_bond(BT_ID_DEFAULT, add_bonded_addr_to_filter_list, NULL);

	adv_param = *BT_LE_ADV_CONN_FAST_1;

	/* If we have got at least one bond, activate the filter */
	if (bond_count) {
		/* BT_LE_ADV_OPT_FILTER_CONN is required to activate accept filter list,
		 * BT_LE_ADV_OPT_FILTER_SCAN_REQ will prevent sending scan response data to
		 * devices, that are not on the accept filter list
		 */
		adv_param.options |= BT_LE_ADV_OPT_FILTER_CONN | BT_LE_ADV_OPT_FILTER_SCAN_REQ;
	}

	err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
	} else {
		printk("Advertising successfully started\n");
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

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	bt_ready();
	bt_conn_auth_info_cb_register(&bt_conn_auth_info);

	while (1) {
		k_sleep(K_FOREVER);
	}
	return 0;
}
