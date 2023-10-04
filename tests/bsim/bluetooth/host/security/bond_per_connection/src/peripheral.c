/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/toolchain.h>

#include <stdint.h>
#include <string.h>

void peripheral(void)
{
	bs_bt_utils_setup();

	int id_a;
	int id_b;

	id_a = bt_id_create(NULL, NULL);
	ASSERT(id_a >= 0, "bt_id_create id_a failed (err %d)\n", id_a);

	id_b = bt_id_create(NULL, NULL);
	ASSERT(id_b >= 0, "bt_id_create id_b failed (err %d)\n", id_b);

	printk("== Bonding id a - global bondable mode ==\n");
	BUILD_ASSERT(CONFIG_BT_BONDABLE, "CONFIG_BT_BONDABLE must be enabled by default.");
	enable_bt_conn_set_bondable(false);
	advertise_connectable(id_a, NULL);
	wait_connected();
	/* Central should bond here and trigger a disconnect. */
	wait_disconnected();
	TAKE_FLAG(flag_pairing_complete);
	TAKE_FLAG(flag_bonded);
	unpair(id_a);
	clear_g_conn();

	printk("== Bonding id a - bond per-connection true ==\n");
	enable_bt_conn_set_bondable(true);
	set_bondable(true);
	advertise_connectable(id_a, NULL);
	wait_connected();
	/* Central should bond here and trigger a disconnect. */
	wait_disconnected();
	TAKE_FLAG(flag_pairing_complete);
	TAKE_FLAG(flag_bonded);
	clear_g_conn();

	printk("== Bonding id b - bond per-connection false ==\n");
	set_bondable(false);
	advertise_connectable(id_b, NULL);
	wait_connected();
	/* Central should pair without bond here and trigger a disconnect. */
	wait_disconnected();
	TAKE_FLAG(flag_pairing_complete);
	TAKE_FLAG(flag_not_bonded);

	PASS("PASS\n");
}
