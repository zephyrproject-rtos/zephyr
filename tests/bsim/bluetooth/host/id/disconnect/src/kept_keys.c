/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <testlib/adv.h>
#include <testlib/conn.h>
#include <testlib/scan.h>
#include <testlib/security.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>

#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#define DUT_NAME   "kept keys dut"
#define OTHER_NAME "kept keys other"

static void test_setup(void)
{
	int err;

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Failed to open the backchannel (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable failed (err %d)", err);
}

/* The only key slot, given up by the first peer, now holds the keys of another one */
static void dut_bond_other_peer_and_unpair_first(const bt_addr_le_t *first)
{
	struct bt_conn *conn = NULL;
	bt_addr_le_t other;
	int err;

	bt_testlib_conn_wait_free();

	/* The peer advertises with its other identity */
	bk_sync_send();

	err = bt_testlib_scan_find_name(&other, OTHER_NAME);
	TEST_ASSERT(err == 0, "The other peer was not found (err %d)", err);

	err = bt_testlib_connect(&other, &conn);
	TEST_ASSERT(err == 0, "Connecting failed (err %d)", err);

	err = bt_testlib_secure(conn, BT_SECURITY_L2);
	TEST_ASSERT(err == 0, "Pairing failed (err %d)", err);
	TEST_ASSERT(bt_le_bond_exists(BT_ID_DEFAULT, &other), "The other peer is not bonded");

	err = bt_testlib_disconnect(&conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Disconnecting failed (err %d)", err);

	err = bt_unpair(BT_ID_DEFAULT, first);
	TEST_ASSERT(err == 0, "bt_unpair failed (err %d)", err);

	TEST_ASSERT(bt_le_bond_exists(BT_ID_DEFAULT, &other),
		    "bt_unpair removed another peer's bond");
}

static void dut_first_connection(struct bt_conn **kept, bt_addr_le_t *first)
{
	int err;

	err = bt_testlib_adv_conn(kept, BT_ID_DEFAULT, DUT_NAME);
	TEST_ASSERT(err == 0, "Advertising failed (err %d)", err);

	bt_addr_le_copy(first, bt_conn_get_dst(*kept));

	err = bt_testlib_wait_disconnected(*kept);
	TEST_ASSERT(err == 0, "Waiting for the disconnection failed (err %d)", err);
}

/* The keys of a peer that did not bond go with the disconnection, not with the kept object */
void test_kept_keys_not_bonded_dut_main(void)
{
	struct bt_conn *kept = NULL;
	bt_addr_le_t first;

	TEST_START("kept_keys_not_bonded_dut");

	test_setup();

	bt_set_bondable(false);
	dut_first_connection(&kept, &first);
	bt_set_bondable(true);

	dut_bond_other_peer_and_unpair_first(&first);

	bt_conn_drop(&kept);

	/* The peer ends the simulation once it has been told the DUT is done */
	TEST_PASS("kept_keys_not_bonded_dut");

	bk_sync_send();
}

/* Unpairing clears the keys that the kept object and a newer connection both refer to */
void test_kept_keys_unpaired_dut_main(void)
{
	struct bt_conn *kept = NULL;
	struct bt_conn *live = NULL;
	bt_addr_le_t first;
	int err;

	TEST_START("kept_keys_unpaired_dut");

	test_setup();

	dut_first_connection(&kept, &first);

	err = bt_testlib_adv_conn(&live, BT_ID_DEFAULT, DUT_NAME);
	TEST_ASSERT(err == 0, "Advertising failed (err %d)", err);

	/* The peer has encrypted the connection */
	bk_sync_wait();

	err = bt_unpair(BT_ID_DEFAULT, &first);
	TEST_ASSERT(err == 0, "bt_unpair failed (err %d)", err);

	err = bt_testlib_wait_disconnected(live);
	TEST_ASSERT(err == 0, "Waiting for the disconnection failed (err %d)", err);

	bt_conn_drop(&live);

	/* The peer has given up its only key slot too */
	bk_sync_wait();

	dut_bond_other_peer_and_unpair_first(&first);

	bt_conn_drop(&kept);

	/* The peer ends the simulation once it has been told the DUT is done */
	TEST_PASS("kept_keys_unpaired_dut");

	bk_sync_send();
}

static void peer_connect_and_secure(struct bt_conn **conn, bt_addr_le_t *dut)
{
	int err;

	err = bt_testlib_scan_find_name(dut, DUT_NAME);
	TEST_ASSERT(err == 0, "The DUT was not found (err %d)", err);

	err = bt_testlib_connect(dut, conn);
	TEST_ASSERT(err == 0, "Connecting failed (err %d)", err);

	err = bt_testlib_secure(*conn, BT_SECURITY_L2);
	TEST_ASSERT(err == 0, "Securing the connection failed (err %d)", err);
}

static void peer_disconnect(struct bt_conn **conn)
{
	int err;

	err = bt_testlib_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Disconnecting failed (err %d)", err);
}

static void peer_be_other_peer(void)
{
	struct bt_conn *conn = NULL;
	int err;
	int id;

	/* The DUT is ready to connect */
	bk_sync_wait();

	id = bt_id_create(NULL, NULL);
	TEST_ASSERT(id > BT_ID_DEFAULT, "Creating an identity failed (err %d)", id);

	err = bt_testlib_adv_conn(&conn, id, OTHER_NAME);
	TEST_ASSERT(err == 0, "Advertising failed (err %d)", err);

	err = bt_testlib_wait_disconnected(conn);
	TEST_ASSERT(err == 0, "Waiting for the disconnection failed (err %d)", err);

	bt_conn_drop(&conn);

	/* The DUT has checked the result */
	bk_sync_wait();
}

void test_kept_keys_not_bonded_peer_main(void)
{
	struct bt_conn *conn = NULL;
	bt_addr_le_t dut;

	TEST_START("kept_keys_not_bonded_peer");

	test_setup();

	peer_connect_and_secure(&conn, &dut);
	peer_disconnect(&conn);

	peer_be_other_peer();

	TEST_PASS_AND_EXIT("kept_keys_not_bonded_peer");
}

void test_kept_keys_unpaired_peer_main(void)
{
	struct bt_conn *conn = NULL;
	bt_addr_le_t dut;
	int err;

	TEST_START("kept_keys_unpaired_peer");

	test_setup();

	peer_connect_and_secure(&conn, &dut);
	peer_disconnect(&conn);

	peer_connect_and_secure(&conn, &dut);
	bk_sync_send();

	err = bt_testlib_wait_disconnected(conn);
	TEST_ASSERT(err == 0, "Waiting for the disconnection failed (err %d)", err);

	bt_conn_drop(&conn);

	err = bt_unpair(BT_ID_DEFAULT, &dut);
	TEST_ASSERT(err == 0, "bt_unpair failed (err %d)", err);

	bk_sync_send();

	peer_be_other_peer();

	TEST_PASS_AND_EXIT("kept_keys_unpaired_peer");
}
