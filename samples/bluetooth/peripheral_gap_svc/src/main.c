/*
 * Copyright (c) 2025 Koppel Electronic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/gatt.h>

/* -----------------------------------------------------------------------------
 * Local implementation of GAP service
 */

static ssize_t read_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	const char *name = bt_get_name();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, name, strlen(name));
}

static ssize_t write_name(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags)
{
	/* adding one to fit the terminating null character */
	char value[CONFIG_BT_DEVICE_NAME_MAX + 1] = {};

	if (offset > 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len > CONFIG_BT_DEVICE_NAME_MAX) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value, buf, len);

	value[len] = '\0';

	/* Check if the name starts with capital letter
	 * Note:
	 * This is not the Bluetooth Core requirement, but just a simple demonstration of how
	 * the application can validate the written value and reject it if it doesn't
	 * meet the criteria.
	 */
	if (value[0] < 'A' || value[0] > 'Z') {
		printk("Rejected name change to \"%s\": must start with capital letter\n", value);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	if (bt_set_name(value) != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	printk("Name changed to \"%s\"\n", value);

	return len;
}

static ssize_t read_appearance(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	uint16_t appearance = sys_cpu_to_le16(bt_get_appearance());

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &appearance, sizeof(appearance));
}

BT_GATT_SERVICE_DEFINE(app_gap_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_GAP),
	/* Note: Name can be overwritten without any security requirements */
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_DEVICE_NAME,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_name, write_name, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_APPEARANCE,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_appearance, NULL, NULL),
);

/* End of local implementation of GAP service
 * ---------------------------------------------------------------------------
 */

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err != BT_HCI_ERR_SUCCESS) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}

	printk("Connected: %s\n", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);
}

static int start_advertising(void)
{
	int err;
	const char *name = bt_get_name();
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_DATA(BT_DATA_NAME_COMPLETE, name, strlen(name)),
	};

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		printk("Advertising failed to start (err %d)\n", err);
	}

	return err;
}

static void recycled(void)
{
	start_advertising();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = recycled,
};

int main(void)
{
	int err;

	printk("Sample - Bluetooth Peripheral GAP service\n");

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Failed to enable bluetooth: %d\n", err);
		return err;
	}

	err = start_advertising();
	if (err != 0) {
		return err;
	}

	printk("Initialization complete\n");

	return 0;
}
