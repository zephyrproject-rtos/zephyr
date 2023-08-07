/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "zephyr/bluetooth/addr.h"
#include "zephyr/bluetooth/conn.h"

#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>

void central(void)
{
	bs_bt_utils_setup();

	struct bt_conn *conn_a;
	struct bt_conn *conn_b;

	/* Connect to the first identity of the peripheral. */
	scan_connect_to_first_result();
	wait_connected(&conn_a);

	/* Subscribe to battery notifications and wait on the first one. */
	bas_subscribe(conn_a);
	wait_bas_notification();

	/* Connect to the second identity of the peripheral. */
	scan_connect_to_first_result();
	wait_connected(&conn_b);

	/* Establish security with the second identity and resolve identity address. */
	set_security(conn_b, BT_SECURITY_L2);
	wait_pairing_completed();

	/* Wait for notification from the first connection after identity address resolution. */
	wait_bas_notification();

	/* Disconnect the first identity of the peripheral. */
	disconnect(conn_a);
	wait_disconnected();
	clear_conn(conn_a);

	/* Disconnect the second identity of the peripheral. */
	disconnect(conn_b);
	wait_disconnected();
	clear_conn(conn_b);

	PASS("PASS\n");
}
