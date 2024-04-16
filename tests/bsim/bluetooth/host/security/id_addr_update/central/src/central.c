/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "utils.h"
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>

#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(central, LOG_LEVEL_INF);

void central(void)
{
	bs_bt_utils_setup();

	struct bt_conn *conn_a;
	struct bt_conn *conn_b;

	/* Connect to the first identity of the peripheral. */
	LOG_INF("conn first");
	scan_connect_to_first_result();
	LOG_INF("wait conn");
	wait_connected(&conn_a);

	/* Subscribe to battery notifications and wait on the first one. */
	LOG_INF("subscribe first");
	bas_subscribe(conn_a);
	wait_bas_notification();

	/* Connect to the second identity of the peripheral. */
	LOG_INF("scan 2nd id");
	scan_connect_to_first_result();
	wait_connected(&conn_b);

	/* Establish security with the second identity and resolve identity address. */
	LOG_INF("set sec");
	set_security(conn_b, BT_SECURITY_L2);
	wait_pairing_completed();

	/* Wait for notification from the first connection after identity address resolution. */
	LOG_INF("wait notif");
	wait_bas_notification();

	/* Disconnect the first identity of the peripheral. */
	LOG_INF("discon id first");
	disconnect(conn_a);
	wait_disconnected();
	clear_conn(conn_a);

	/* Disconnect the second identity of the peripheral. */
	LOG_INF("discon id second");
	disconnect(conn_b);
	wait_disconnected();
	clear_conn(conn_b);

	PASS("PASS\n");
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "central",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = central,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
