/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

#include "testlib/adv.h"
#include "testlib/att_read.h"
#include "testlib/att_write.h"
#include "testlib/conn.h"

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

void entrypoint_peer(void)
{
	int err;
	struct bt_conn *conn;

	/* Mark test as in progress. */
	TEST_START("peer");

	/* Initialize Bluetooth */
	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	err = bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, bt_get_name());
	TEST_ASSERT(!err, "Failed to start connectable advertising (err %d)", err);

	TEST_PASS("peer");
}
