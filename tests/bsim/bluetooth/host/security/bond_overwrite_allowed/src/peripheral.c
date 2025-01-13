/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
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

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

void peripheral(void)
{
	bs_bt_utils_setup();

	int id_a;
	int id_b;
	bt_addr_le_t central;

	id_a = bt_id_create(NULL, NULL);
	TEST_ASSERT(id_a >= 0, "bt_id_create id_a failed (err %d)", id_a);

	id_b = bt_id_create(NULL, NULL);
	TEST_ASSERT(id_b >= 0, "bt_id_create id_b failed (err %d)", id_b);

	printk("== Bonding id a ==\n");
	advertise_connectable(id_a, NULL);
	wait_connected();
	/* Central should bond here, and trigger a disconnect. */
	wait_disconnected();
	central = *bt_conn_get_dst(g_conn);
	clear_g_conn();

	printk("== Bonding id b ==\n");
	advertise_connectable(id_b, NULL);
	wait_connected();
	/* Central should bond here. */
	BUILD_ASSERT(IS_ENABLED(CONFIG_BT_SMP_ALLOW_UNAUTH_OVERWRITE), "");
	BUILD_ASSERT(IS_ENABLED(CONFIG_BT_ID_UNPAIR_MATCHING_BONDS), "");
	TAKE_FLAG(flag_pairing_complete);
	TEST_ASSERT(bt_addr_le_eq(bt_conn_get_dst(g_conn), &central),
		    "Test requires that central uses the same identity in both bonds.");
	/* Central should disconnect here. */
	wait_disconnected();
	clear_g_conn();

	printk("== Directed connect id b ==\n");
	advertise_connectable(id_b, &central);
	wait_connected();
	/* Central should verify that its bond with id_b works as expected. */

	TEST_PASS("PASS");
}
