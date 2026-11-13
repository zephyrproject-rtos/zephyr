/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/controller.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "babblekit/testcase.h"
#include "bstests.h"

/* Upper bound for cancelling the initiator and failing to restart it */
#define ATTEMPT_END_MAX K_SECONDS(1)

static K_SEM_DEFINE(attempt_end_sem, 0, 1);
static uint8_t attempt_err;

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	attempt_err = err;
	k_sem_give(&attempt_end_sem);
}

static void test_auto_identity_adv(void)
{
	/* A public identity address, so that the legacy advertiser does not use the random
	 * address that the initiator's RPA is set in
	 */
	static const uint8_t public_addr[BT_ADDR_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	/* A device that does not advertise, so that the procedure is still running */
	static const bt_addr_le_t peer_addr = {BT_ADDR_LE_RANDOM,
					       {{0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0xCF}}};
	static struct bt_conn_cb conn_cb = {
		.connected = connected,
	};
	struct bt_le_oob oob;
	int err;

	bt_ctlr_set_public_addr(public_addr);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Failed to register connection callbacks (err %d)", err);

	err = bt_le_filter_accept_list_add(&peer_addr);
	TEST_ASSERT(err == 0, "Failed to add the peer to the accept list (err %d)", err);

	err = bt_conn_le_create_auto(BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT);
	TEST_ASSERT(err == 0, "Failed to start connecting (err %d)", err);

	/* While it is enabled, the Controller refuses a new random address, both for the rotation
	 * and for the restart of the initiator that the rotation cancels
	 */
	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, NULL, 0, NULL, 0);
	TEST_ASSERT(err == 0, "Failed to start advertising (err %d)", err);

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);
	TEST_ASSERT(err == 0, "Failed to rotate the RPAs (err %d)", err);

	TEST_ASSERT(k_sem_take(&attempt_end_sem, ATTEMPT_END_MAX) == 0,
		    "The connection procedure did not end");
	TEST_ASSERT(attempt_err == BT_HCI_ERR_UNSPECIFIED, "The procedure ended with 0x%02x",
		    attempt_err);

	/* The ended procedure can be started again once the random address can be set */
	err = bt_le_adv_stop();
	TEST_ASSERT(err == 0, "Failed to stop advertising (err %d)", err);
	err = bt_conn_le_create_auto(BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT);
	TEST_ASSERT(err == 0, "Failed to start connecting again (err %d)", err);

	TEST_PASS_AND_EXIT("PASS");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "auto_identity_adv",
		.test_descr =
			"The automatic connection establishment procedure ends with an error "
			"when its initiator cannot be restarted after an RPA rotation next to "
			"a legacy identity advertiser, and can then be started again",
		.test_main_f = test_auto_identity_adv,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_auto_identity_adv_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
