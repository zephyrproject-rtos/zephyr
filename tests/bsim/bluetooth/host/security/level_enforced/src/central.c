/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

#include <zephyr/logging/log.h>

#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(central, LOG_LEVEL_DBG);

/* No bt_conn_auth_cb is registered on either side, which is what leaves both
 * peers with no input and no output and pins the pairing method to Just Works.
 */
void central(void)
{
	test_setup();
	scan_connect_to_first_result();
	wait_connected();

	/* First round is the peripheral's. Confirming the link reached L2 here
	 * is what makes the peripheral's own control a two-sided fact.
	 */
	expect_observed_elevation(BT_SECURITY_L2);

	wait_disconnected();
	clear_g_conn();

	/* Second round is the central's. The roles do not share a code path:
	 * `bt_smp_start_security()` sends a security request as peripheral but a
	 * pairing request as central, so the guard has to hold on both. Dropping
	 * the bond first keeps the stored key from answering in its place.
	 */
	forget_bonds();
	forget_security_result();

	scan_connect_to_first_result();
	wait_connected();

	expect_refused_elevation(BT_SECURITY_L4);
	expect_refused_elevation(BT_SECURITY_L3);
	expect_granted_elevation(BT_SECURITY_L2);

	disconnect_and_wait();

	TEST_PASS("Central done");
}
