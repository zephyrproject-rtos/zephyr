/*
 * Copyright 2025 - 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * AMP mbox test (primary Cortex-M side).
 *
 * Exercises the shared request/response IPC layer (testipc, backed by the
 * Zephyr MBOX API over the NXP MU) against the remote HiFi4 DSP:
 *
 *   1. The DSP starts and runs - verified by waiting for the DSP's initial
 *      "alive" beacon (proving it loaded and booted).
 *   2. mbox IPC is functional and the DSP keeps running - verified by a data
 *      round-trip (echo): each successful echo also proves the DSP is still
 *      alive and processing messages, not hung or crashed.
 *
 * Every wait is bounded by a timeout. If the DSP fails to load, hangs or
 * crashes, the expected messages never arrive and the ztest assertions fail
 * instead of falsely passing.
 *
 * Note: the earlier data-less "IPI"/doorbell usecase was removed together with
 * the doorbell path in the IPC layer - the MU DATA path is the sole, reliable
 * transport now, so notifications travel as ordinary DATA words too.
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <dsp.h>
#include "testipc.h"

/* Generous per-operation timeout for DSP boot / IPC round-trips. */
#define AMP_MBOX_TIMEOUT   K_MSEC(5000)

/* Number of echo round-trips used to prove the DSP keeps running. */
#define AMP_MBOX_ECHO_ITERATIONS 3U

/* Set once the DSP's initial alive beacon has been received. */
static bool dsp_booted;

static void *amp_mbox_setup(void)
{
	uint32_t msg;

	/* Initialise the request/response IPC protocol. */
	zassert_ok(testipc_init(), "IPC init failed");

	/* Bring the DSP core up. */
	dsp_start();

	/* The DSP emits an alive beacon once it has booted. */
	zassert_ok(testipc_recv_timeout(&msg, AMP_MBOX_TIMEOUT),
		   "no alive beacon from DSP - it may have failed to start");

	zassert_equal(testipc_msg_get_op(msg), AMP_OP_ALIVE,
		      "unexpected first message from DSP (op 0x%02x)",
		      testipc_msg_get_op(msg));

	dsp_booted = true;

	return NULL;
}

/*
 * Usecase 1: the DSP starts and runs.
 *
 * Its initial alive beacon is received during suite setup; assert that it
 * arrived, proving the DSP loaded and booted.
 */
ZTEST(amp_mbox, test_dsp_alive)
{
	zassert_true(dsp_booted,
		     "DSP did not emit an alive beacon - it may have failed to start");
}

/*
 * Usecase 2: mbox IPC is functional and the DSP keeps running.
 *
 * Send a known payload to the DSP and assert it echoes exactly that value
 * back. Repeating the round-trip also proves the DSP has not hung or crashed.
 */
ZTEST(amp_mbox, test_mbox_echo)
{
	for (unsigned int i = 0; i < AMP_MBOX_ECHO_ITERATIONS; i++) {
		uint32_t msg;

		zassert_ok(
			testipc_send(testipc_msg_make(AMP_OP_ECHO_REQ, AMP_ECHO_MAGIC)),
			"mbox send (echo request) failed"
		);

		zassert_ok(testipc_recv_timeout(&msg, AMP_MBOX_TIMEOUT),
			   "no echo response from DSP");

		zassert_equal(testipc_msg_get_op(msg), AMP_OP_ECHO_RESP,
			      "unexpected opcode 0x%02x (expected echo response)",
			      testipc_msg_get_op(msg));

		zassert_equal(testipc_msg_get_payload(msg), AMP_ECHO_MAGIC,
			      "echoed payload mismatch: got 0x%06x expected 0x%06x",
			      testipc_msg_get_payload(msg), AMP_ECHO_MAGIC);
	}
}

ZTEST_SUITE(amp_mbox, NULL, amp_mbox_setup, NULL, NULL, NULL);
