/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils.h"
#include "gatt_utils.h"

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/settings/settings.h>
#include <zephyr/toolchain.h>

#include <stdint.h>
#include <string.h>

void client_round_0(void)
{
	struct bt_conn *conn;

	printk("start round 0...........\n");

	conn = connect_as_peripheral();
	printk("connected: conn %p\n", conn);

	gatt_discover();
	activate_robust_caching();
	/* subscribe to the SC indication, so we don't have to ATT read to
	 * become change-aware.
	 */
	gatt_subscribe_to_service_changed(true);
	read_test_char(true);

	/* We should normally wait until we are bonded to write the CCC / CF
	 * characteristics, but here we bond after the fact on purpose, to
	 * simulate a client that has this exact behavior.
	 * The CCC and CF should still persist on reboot.
	 */
	wait_bonded();

	disconnect(conn);
}

void client_round_1(void)
{
	struct bt_conn *conn;

	printk("start round 1...........\n");

	conn = connect_as_peripheral();
	printk("connected: conn %p\n", conn);
	wait_secured();

	/* server should remember we are change-aware */
	read_test_char(true);

	disconnect(conn);
}

void client_round_2(void)
{
	struct bt_conn *conn;

	printk("start round 2...........\n");

	conn = connect_as_peripheral();
	printk("connected: conn %p\n", conn);
	wait_secured();

	/* We are change-unaware. wait until the Service Changed indication is
	 * received, that should then make us change-aware.
	 */
	wait_for_sc_indication();
	read_test_char(true);

	/* We sleep just enough so that the server's `delayed store` work item
	 * is executed. We still trigger a disconnect, even though the server
	 * device will be unresponsive for this round.
	 */
	k_sleep(K_MSEC(CONFIG_BT_SETTINGS_DELAYED_STORE_MS));

	disconnect(conn);
}

void client_round_3(void)
{
	struct bt_conn *conn;

	printk("start round 3...........\n");

	conn = connect_as_peripheral();
	printk("connected: conn %p\n", conn);
	wait_secured();

	/* server should remember we are change-aware */
	read_test_char(true);

	/* Unsubscribe from the SC indication.
	 *
	 * In the next round, we will be change-unaware, so the first ATT read
	 * will fail, but the second one will succeed and we will be marked as
	 * change-aware again.
	 */
	gatt_subscribe_to_service_changed(false);

	disconnect(conn);
}

void client_round_4(void)
{
	struct bt_conn *conn;

	printk("start round 4...........\n");

	conn = connect_as_peripheral();
	printk("connected: conn %p\n", conn);
	wait_secured();

	/* GATT DB has changed again.
	 * Before disc: svc1
	 * After disc: svc1 + svc2
	 * At boot: svc1
	 * Expect a failure on the first read of the same GATT handle.
	 */
	read_test_char(false);
	read_test_char(true);

	disconnect(conn);
}

void client_round_5(void)
{
	printk("start round 5...........\n");
	printk("don't need to do anything, central will "
	       "not connect to us\n");
}

void client_round_6(void)
{
	struct bt_conn *conn;

	printk("start round 6...........\n");
	conn = connect_as_peripheral();
	printk("connected: conn %p\n", conn);
	wait_secured();

	/* GATT DB has changed again.
	 * Expect a failure on the first read of the same GATT handle.
	 */
	read_test_char(false);
	read_test_char(true);

	disconnect(conn);
}

void client_procedure(void)
{
	bt_enable(NULL);
	settings_load();

	client_round_0();
	client_round_1();
	client_round_2();
	client_round_3();
	client_round_4();
	client_round_5();
	client_round_6();

	PASS("PASS\n");
}
