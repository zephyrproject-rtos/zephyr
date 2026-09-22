/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

#include <zephyr/logging/log.h>

#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(peripheral, LOG_LEVEL_DBG);

void peripheral(void)
{
	test_setup();
	advertise_connectable();
	wait_connected();

	/* Neither peer registers authentication callbacks, so both have no
	 * input and no output and the only available pairing method is Just
	 * Works, which produces an unauthenticated key. L3 and L4 are both
	 * unreachable on this link by construction.
	 *
	 * The host must say so instead of pairing anyway and reporting a lower
	 * level as success: an application that asks for L4 and is told the
	 * request was accepted has no other signal to act on.
	 *
	 * A peripheral request travels `smp_send_security_req()`.
	 */
	expect_refused_elevation(BT_SECURITY_L4);
	expect_refused_elevation(BT_SECURITY_L3);
	expect_granted_elevation(BT_SECURITY_L2);

	disconnect_and_wait();

	/* Second round: the central drives the same three attempts, over a link
	 * that carries no key from the first round.
	 */
	forget_bonds();
	forget_security_result();

	advertise_connectable();
	wait_connected();
	wait_disconnected();
	clear_g_conn();

	TEST_PASS("Unreachable security levels refused, reachable one granted");
}
