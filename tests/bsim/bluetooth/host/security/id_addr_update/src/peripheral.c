/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "zephyr/bluetooth/addr.h"
#include "zephyr/bluetooth/bluetooth.h"
#include "zephyr/bluetooth/conn.h"
#include "zephyr/toolchain/gcc.h"

#include <stdint.h>
#include <string.h>

static void verify_equal_address(struct bt_conn *conn_a, struct bt_conn *conn_b)
{
	int err;
	struct bt_conn_info info_a;
	struct bt_conn_info info_b;

	err = bt_conn_get_info(conn_a, &info_a);
	ASSERT(!err, "Unexpected info_a result.");

	err = bt_conn_get_info(conn_b, &info_b);
	ASSERT(!err, "Unexpected info_b result.");

	ASSERT(bt_addr_le_eq(info_a.le.dst, info_b.le.dst),
	       "Conn A address is not equal with the conn B address");
}

void peripheral(void)
{
	bs_bt_utils_setup();

	int id_a;
	int id_b;

	struct bt_conn *conn_a;
	struct bt_conn *conn_b;

	/* Create two identities that will simultaneously connect with the same central peer. */
	id_a = bt_id_create(NULL, NULL);
	ASSERT(id_a >= 0, "bt_id_create id_a failed (err %d)\n", id_a);

	id_b = bt_id_create(NULL, NULL);
	ASSERT(id_b >= 0, "bt_id_create id_b failed (err %d)\n", id_b);

	/* Connect with the first identity. */
	advertise_connectable(id_a);
	wait_connected(&conn_a);

	/* Send battery notification on the first connection. */
	wait_bas_ccc_subscription();
	bas_notify(conn_a);

	/* Connect with the second identity. */
	advertise_connectable(id_b);
	wait_connected(&conn_b);

	/* Wait for the pairing completed callback on the second identity. */
	wait_pairing_completed();

	/* Both connections should relate to the identity address of the same Central peer. */
	verify_equal_address(conn_a, conn_b);

	/* Send notification after identity address resolution to the first connection object. */
	bas_notify(conn_a);

	/* Disconnect the first identity. */
	wait_disconnected();
	clear_conn(conn_a);

	/* Disconnect the second identity. */
	wait_disconnected();
	clear_conn(conn_b);

	PASS("PASS\n");
}
