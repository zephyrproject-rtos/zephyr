/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils.h"
#include "main.h"
#include "gatt_utils.h"

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/settings/settings.h>
#include <zephyr/toolchain.h>

#include <stdint.h>
#include <string.h>

void set_public_addr(void)
{
	bt_addr_le_t addr = {BT_ADDR_LE_RANDOM,
			     {{0x0A, 0x89, 0x67, 0x45, 0x23, 0xC1}}};
	bt_id_create(&addr, NULL);
}

void server_round_0(void)
{
	struct bt_conn *conn;

	conn = connect_as_central();
	wait_for_client_read();

	printk("bonding\n");
	bond(conn);
}

void server_round_1(void)
{
	struct bt_conn *conn;

	/* Wait for GATT DB hash to complete. */
	k_sleep(K_SECONDS(2));
	conn = connect_as_central();
	printk("encrypting\n");
	set_security(conn, BT_SECURITY_L2);

	wait_for_client_read();
	wait_disconnected();

	printk("register second service, peer will be change-unaware\n");
	gatt_register_service_2();
	/* on-disk hash will be different when round 2 (and round 4)
	 * start, the peer will be marked as change-unaware
	 */
	k_sleep(K_MSEC(100));
}

void server_round_2(void)
{
	struct bt_conn *conn;

	conn = connect_as_central();
	printk("encrypting\n");
	set_security(conn, BT_SECURITY_L2);

	wait_for_client_read();

	/* Kill the power before the graceful disconnect, to make sure
	 * that the change-aware status has been written correctly to
	 * NVS. We still have to wait for the delayed work to be run.
	 */
	k_sleep(K_MSEC(CONFIG_BT_SETTINGS_DELAYED_STORE_MS));
}

void server_round_3(void)
{
	struct bt_conn *conn;

	conn = connect_as_central();
	printk("encrypting\n");
	set_security(conn, BT_SECURITY_L2);

	wait_for_client_read();
	wait_disconnected();

	printk("register second service, peer will be change-unaware\n");
	gatt_register_service_2();
	/* on-disk hash will be different when round 2 (and round 4)
	 * start, the peer will be marked as change-unaware
	 */
	k_sleep(K_MSEC(100));
}

void server_round_4(void)
{
	struct bt_conn *conn;

	conn = connect_as_central();
	printk("encrypting\n");
	set_security(conn, BT_SECURITY_L2);

	wait_for_client_read();
	wait_disconnected();
}

void server_round_5(void)
{
	gatt_register_service_2();

	/* sleep long enough to ensure the DB hash is stored to disk, but short
	 * enough to make sure the delayed storage work item is not executed.
	 */
	k_sleep(K_MSEC(100));
}

void server_round_6(void)
{
	struct bt_conn *conn;

	gatt_register_service_2();

	conn = connect_as_central();
	printk("encrypting\n");
	set_security(conn, BT_SECURITY_L2);

	wait_for_client_read();
	wait_disconnected();
}

/* What is being tested: since this deals with settings it's not the rounds
 * themselves, but rather the transitions that test expected behavior.
 *
 * Round 0 -> 1: test CCC / CF values written before bonding are stored to NVS
 * if the server reboots before disconnecting.
 *
 * Round 1 -> 2: test change-awareness is updated if GATT DB changes _after_ the
 * peer has disconnected. In round 2 we also make sure we receive the Service
 * Changed indication.
 *
 * Round 2 -> 3: tests `CONFIG_BT_SETTINGS_CF_STORE_ON_WRITE` does its job, and
 * writes the change-awareness before we get disconnected. Basically, this
 * transition simulates a user yanking the power of the device before it has the
 * chance to disconnect.
 *
 * Round 3 -> 4: same as (1->2), except this time we won't get the SC indication
 * (as we have unsubscribed from it). We should instead get the
 * `BT_ATT_ERR_DB_OUT_OF_SYNC` error on the first attribute read. This also
 * tests that robust GATT caching is enforced.
 *
 * Round 4 -> 5: tests change-awareness status is still written on disconnect.
 * This is a non-regression test to make sure we didn't break the previous
 * behavior.
 *
 * Round 5 -> 6: tests DFU corner case: in this case, we are on the first boot
 * of an updated firmware, that will register new services. But for some unknown
 * reason, we decide to reboot before the delayed store work item has had the
 * time to execute and store that the peers are now change-unaware. Round 6 then
 * makes sure that we are indeed change-unaware.
 */
void server_procedure(void)
{
	uint8_t round = get_test_round();

	wait_for_round_start();

	printk("Start test round: %d\n", get_test_round());

	/* Use the same public address for all instances of the central. If we
	 * don't do that, encryption (using the bond stored in NVS) will
	 * fail.
	 */
	set_public_addr();

	gatt_register_service_1();

	bt_enable(NULL);
	settings_load();

	switch (round) {
	case 0:
		server_round_0();
		break;
	case 1:
		server_round_1();
		break;
	case 2:
		server_round_2();
		break;
	case 3:
		server_round_3();
		break;
	case 4:
		server_round_4();
		break;
	case 5:
		server_round_5();
		break;
	case 6:
		server_round_6();
		break;
	default:
		FAIL("Round %d doesn't exist\n", round);
		break;
	}

	signal_next_test_round();
}
