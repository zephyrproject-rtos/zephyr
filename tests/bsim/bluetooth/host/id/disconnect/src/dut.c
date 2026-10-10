/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <testlib/adv.h>
#include <testlib/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/printk.h>

#include "babblekit/sync.h"
#include "babblekit/testcase.h"

static struct bt_conn *connect_with_id(uint8_t id)
{
	struct bt_conn *conn = NULL;
	struct bt_conn_info info;
	int err;

	err = bt_testlib_adv_conn(&conn, id, bt_get_name());
	TEST_ASSERT(err == 0, "Advertising with identity %u failed (err %d)", id, err);
	printk("Connected with identity %u\n", id);

	err = bt_conn_get_info(conn, &info);
	TEST_ASSERT(err == 0, "bt_conn_get_info failed (err %d)", err);
	TEST_ASSERT(info.id == id, "Connection uses identity %u, expected %u", info.id, id);

	return conn;
}

static void expect_disconnect(struct bt_conn **conn, struct bt_conn *other_conn, const char *what)
{
	struct bt_conn_info info;
	int err;

	bt_testlib_wait_disconnected(*conn);
	bt_conn_drop(conn);
	printk("Disconnected after %s\n", what);

	/* A connection of another identity is left alone */
	err = bt_conn_get_info(other_conn, &info);
	TEST_ASSERT(err == 0, "bt_conn_get_info failed (err %d)", err);
	TEST_ASSERT(info.state == BT_CONN_STATE_CONNECTED,
		    "%s changed the connection of the other identity (state %d)", what, info.state);
}

void test_peripheral_main(void)
{
	struct bt_conn *other_conn;
	struct bt_conn *conn;
	int id;
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Backchannel init failed (err %d)", err);

	/* A connection of the default identity, which has to stay up throughout */
	other_conn = connect_with_id(BT_ID_DEFAULT);

	/* Deleting the identity of an unbonded connection disconnects it */
	id = bt_id_create(NULL, NULL);
	TEST_ASSERT(id > BT_ID_DEFAULT, "Identity creation failed (err %d)", id);

	conn = connect_with_id((uint8_t)id);

	err = bt_id_delete((uint8_t)id);
	TEST_ASSERT(err == 0, "bt_id_delete failed (err %d)", err);
	expect_disconnect(&conn, other_conn, "bt_id_delete");

	/* Resetting the identity of an unbonded connection disconnects it */
	id = bt_id_create(NULL, NULL);
	TEST_ASSERT(id > BT_ID_DEFAULT, "Identity creation failed (err %d)", id);

	conn = connect_with_id((uint8_t)id);

	err = bt_id_reset((uint8_t)id, NULL, NULL);
	TEST_ASSERT(err == id, "bt_id_reset returned %d, expected %d", err, id);
	expect_disconnect(&conn, other_conn, "bt_id_reset");

	/* The central ends the simulation once it has been told the peripheral is done */
	bk_sync_send();
	TEST_PASS("Peripheral done");
}
