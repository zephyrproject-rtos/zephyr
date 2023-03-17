/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <gatt/services.h>

#include "edtt_driver.h"
#include "bs_tracing.h"
#include "commands.h"

#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_CTS_VAL)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
		      0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static int service_set;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
	} else {
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Security changed: %s level %u\n", addr, level);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void service_setup(int set)
{
	if (set == service_set) {
		printk("Ignored request to change GATT services set to #%d - "
			"already selected!\n", set);
		return;
	}
	switch (service_set) {
	case 0:
		break;
	case 1:
		service_c_2_1_remove();
		service_f_1_remove();
		service_c_1_1_remove();
		service_b_5_1_remove();
		service_b_2_1_remove();
		service_b_1_1_remove();
		service_b_3_1_remove();
		service_b_4_1_remove();
		service_a_1_remove();
		service_d_1_remove();
		break;
	case 2:
		service_e_2_remove();
		service_b_5_2_remove();
		service_b_2_2_remove();
		service_b_3_2_remove();
		service_a_2_remove();
		service_b_1_2_remove();
		service_d_2_remove();
		service_b_4_2_remove();
		service_c_1_2_remove();
		service_c_2_2_remove();
		break;
	case 3:
		service_e_3_remove();
		service_c_2_3_remove();
		service_b_2_3_remove();
		service_c_1_3_remove();
		service_a_3_remove();
		service_b_3_3_remove();
		service_b_4_3_remove();
		service_b_5_3_remove();
		service_d_3_remove();
		service_b_1_3_remove();
		break;
	default:
		break;
	}

	switch (set) {
	case 0:
		break;
	case 1:
		service_d_1_init();
		service_a_1_init();
		service_b_4_1_init();
		service_b_3_1_init();
		service_b_1_1_init();
		service_b_2_1_init();
		service_b_5_1_init();
		service_c_1_1_init();
		service_f_1_init();
		service_c_2_1_init();
		break;
	case 2:
		service_c_2_2_init();
		service_c_1_2_init();
		service_b_4_2_init();
		service_d_2_init();
		service_b_1_2_init();
		service_a_2_init();
		service_b_3_2_init();
		service_b_2_2_init();
		service_b_5_2_init();
		service_e_2_init();
		break;
	case 3:
		service_b_1_3_init();
		service_d_3_init();
		service_b_5_3_init();
		service_b_4_3_init();
		service_b_3_3_init();
		service_a_3_init();
		service_c_1_3_init();
		service_b_2_3_init();
		service_c_2_3_init();
		service_e_3_init();
		break;
	default:
		break;
	}
	service_set = set;
	printk("Switched to GATT services set to #%d\n", set);
}

static void service_notify(void)
{
	switch (service_set) {
	case 0:
		break;
	case 1:
		service_b_3_1_value_v6_notify();
		break;
	case 2:
		service_b_3_2_value_v6_notify();
		break;
	case 3:
		service_b_3_3_value_v6_notify();
		break;
	default:
		break;
	}
}

static void service_indicate(void)
{
	switch (service_set) {
	case 0:
		break;
	case 1:
		break;
	case 2:
		service_b_3_2_value_v6_indicate();
		break;
	case 3:
		service_b_3_3_value_v6_indicate();
		break;
	default:
		break;
	}
}

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	service_setup(1);

	printk("GATT Services initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

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

/**
 * @brief Clean out excess bytes from the input buffer
 */
static void read_excess_bytes(uint16_t size)
{
	if (size > 0) {
		uint8_t buffer[size];

		edtt_read((uint8_t *)buffer, size, EDTTT_BLOCK);
		printk("command size wrong! (%u extra bytes removed)", size);
	}
}

/**
 * @brief Switch GATT Service Set
 */
static void switch_service_set(uint16_t size)
{
	uint16_t response = sys_cpu_to_le16(CMD_GATT_SERVICE_SET_RSP);
	uint8_t  set;

	if (size > 0) {
		edtt_read((uint8_t *)&set, sizeof(set), EDTTT_BLOCK);
		service_setup((int)set);
		size -= sizeof(set);
	}
	read_excess_bytes(size);
	size = 0;

	edtt_write((uint8_t *)&response, sizeof(response), EDTTT_BLOCK);
	edtt_write((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
}

/**
 * @brief Send Notifications from GATT Service Set
 */
static void handle_service_notify(uint16_t size)
{
	uint16_t response = sys_cpu_to_le16(CMD_GATT_SERVICE_NOTIFY_RSP);

	service_notify();
	read_excess_bytes(size);
	size = 0;

	edtt_write((uint8_t *)&response, sizeof(response), EDTTT_BLOCK);
	edtt_write((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
}

/**
 * @brief Send Indications from GATT Service Set
 */
static void handle_service_indicate(uint16_t size)
{
	uint16_t response = sys_cpu_to_le16(CMD_GATT_SERVICE_INDICATE_RSP);

	service_indicate();
	read_excess_bytes(size);
	size = 0;

	edtt_write((uint8_t *)&response, sizeof(response), EDTTT_BLOCK);
	edtt_write((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
}

void main(void)
{
	int err;
	uint16_t command;
	uint16_t size;

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_conn_auth_cb_register(&auth_cb_display);

	/**
	 * Initialize and start EDTT system
	 */
#if defined(CONFIG_ARCH_POSIX)
	enable_edtt_mode();
	set_edtt_autoshutdown(true);
#endif
	edtt_start();

	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	while (1) {
		/**
		 * Wait for a command to arrive - then read and execute command
		 */
		edtt_read((uint8_t *)&command, sizeof(command), EDTTT_BLOCK);
		command = sys_le16_to_cpu(command);
		edtt_read((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
		size = sys_le16_to_cpu(size);
		bs_trace_raw_time(4, "command 0x%04X received (size %u)\n",
				command, size);

		switch (command) {
		case CMD_GATT_SERVICE_SET_REQ:
			switch_service_set(size);
			break;
		case CMD_GATT_SERVICE_NOTIFY_REQ:
			handle_service_notify(size);
			break;
		case CMD_GATT_SERVICE_INDICATE_REQ:
			handle_service_indicate(size);
			break;
		default:
			break;
		}
	}
}
