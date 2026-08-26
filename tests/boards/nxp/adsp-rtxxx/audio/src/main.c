/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/mbox.h>

#include <dsp.h>
#include "testipc.h"

#define TIMEOUT_BOOT (K_MSEC(5000))
#define TIMEOUT_AUDIO_FINISH (K_MSEC(20000))
#define TIMEOUT_ECHO (K_MSEC(2000))

static void *amp_audio_setup(void)
{
	uint32_t msg;

	/* Initialise protocol */
	zassert_ok(testipc_init());

	/* Start DSP and wait for its beacon */
	dsp_start();

	zassert_ok(
		testipc_recv_timeout(&msg, TIMEOUT_BOOT),
		"DSP failed to send a beacon message in time"
	);

	zassert_equal(
		testipc_msg_get_op(msg), AMP_OP_ALIVE,
		"unexpected first message from DSP (op 0x%02x)",
		testipc_msg_get_op(msg)
	);

	return NULL;
}

ZTEST(amp_audio, test_audio_playback_capture)
{
	uint32_t msg;

	/* Start audio on the DSP */
	zassert_ok(
		testipc_send(testipc_msg_make(AMP_OP_AUDIO_START, 0)),
		"failed to command DSP to start audio"
	);

	/* Wait for it to finish */
	zassert_ok(
		testipc_recv_timeout(&msg, TIMEOUT_AUDIO_FINISH),
		"no completion from DSP - stalled/crashed"
	);
	zassert_not_equal(
		testipc_msg_get_op(msg), AMP_OP_ERROR,
		"DSP reported error (%d)",
		testipc_msg_get_payload(msg)
	);
	zassert_equal(
		testipc_msg_get_op(msg), AMP_OP_AUDIO_DONE,
		"unexpected message type"
	);

	/* Do echo */
	for (unsigned int i = 0; i < 3; i++) {
		zassert_ok(
			testipc_send(testipc_msg_make(AMP_OP_ECHO_REQ, AMP_ECHO_MAGIC)),
			"failed to send echo request"
		);

		zassert_ok(
			testipc_recv_timeout(&msg, TIMEOUT_ECHO),
			"failed to receive message (timeout?)"
		);

		zassert_equal(
			testipc_msg_get_op(msg), AMP_OP_ECHO_RESP,
			"unexpected received opcode (0x%02x)",
			testipc_msg_get_op(msg)
		);
		zassert_equal(
			testipc_msg_get_payload(msg), AMP_ECHO_MAGIC,
			"invalid echo magic (0x%02x)",
			testipc_msg_get_payload(msg)
		);
	}
}

ZTEST_SUITE(amp_audio, NULL, amp_audio_setup, NULL, NULL, NULL);
