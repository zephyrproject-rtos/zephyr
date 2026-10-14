/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Data path and callback contract, asserted from the target.
 *
 * The write-start and read-continuation callbacks are deliberately not
 * asserted: the API contract they would encode cannot be met by a
 * conformant I3C driver (MIPI I3C Basic v1.0 5.1.2.3.3 makes the SDR write
 * T-Bit a parity bit, not a per-byte ACK).  These could be added in the
 * future for IPs which support them
 */

#include "test_common.h"

#include <zephyr/drivers/i3c/target_device.h>
#include <zephyr/sys/printk.h>

/**
 * @brief Assert the recorded write payload equals @p want, byte for byte.
 */
static void assert_payload(const uint8_t *want, size_t len)
{
	uint8_t got[TARGET_BUF_MAX];
	size_t n = event_payload(got, sizeof(got));

	if (n != len) {
		event_dump();
	}
	zassert_equal(n, len, "received %zu bytes, expected %zu", n, len);

	for (size_t i = 0; i < len; i++) {
		if (got[i] != want[i]) {
			event_dump();
		}
		zassert_equal(got[i], want[i], "byte %zu: got 0x%02x want 0x%02x", i, got[i],
			      want[i]);
	}
}

/*
 * Bytes the controller writes arrive intact and in order.
 *
 * Per target_device.h, write_received_cb() is passed "the byte received
 * by the controller".
 */
ZTEST(dual_board_loopback_target, test_write_payload_integrity)
{
	static const uint8_t seeds[] = {0x00U, 0xFFU};
	static const size_t len = 16U;
	uint8_t pattern[TARGET_BUF_MAX];
	int rc;
	int a_rc = -1;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	for (size_t s = 0; s < ARRAY_SIZE(seeds); s++) {
		event_reset();
		fill_pattern(pattern, len, seeds[s]);

		rc = ctrl_write(pattern, len, &a_rc);
		zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
		zassert_ok(a_rc, "Board A write failed (%d)", a_rc);

		rc = event_wait_count(EVENT_WRITE_BYTE, len, CB_SETTLE);
		zassert_ok(rc, "seed 0x%02x: only %zu of %zu bytes arrived", seeds[s],
			   event_count(EVENT_WRITE_BYTE), len);

		assert_payload(pattern, len);
	}
}

/*
 * Exactly one completion per write transaction, ordered after the
 * last data byte.
 *
 * Per target_device.h, stop_cb() is "called when a stop condition is
 * observed after a start condition addressed to a particular device".
 */
ZTEST(dual_board_loopback_target, test_write_completion_signalled)
{
	static const size_t len = 8U;
	uint8_t pattern[TARGET_BUF_MAX];
	size_t last_byte_idx = 0;
	size_t stop_idx = 0;
	bool saw_stop = false;
	int rc;
	int a_rc = -1;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	fill_pattern(pattern, len, 0x00);

	rc = ctrl_write(pattern, len, &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "Board A write failed (%d)", a_rc);

	rc = event_wait_count(EVENT_STOP, 1U, CB_SETTLE);
	zassert_ok(rc, "no completion within timeout");

	zassert_equal(event_count(EVENT_STOP), 1U, "expected 1 completion, got %zu",
		      event_count(EVENT_STOP));

	for (size_t i = 0; i < event_stored(); i++) {
		const struct event_entry *ev = event_at(i);

		if (ev->kind == (uint8_t)EVENT_WRITE_BYTE) {
			last_byte_idx = i;
		} else if (ev->kind == (uint8_t)EVENT_STOP) {
			stop_idx = i;
			saw_stop = true;
		}
	}

	zassert_true(saw_stop, "completion not present in the event log");
	if (stop_idx <= last_byte_idx) {
		event_dump();
	}
	zassert_true(stop_idx > last_byte_idx,
		     "completion at index %zu precedes the last data byte at %zu", stop_idx,
		     last_byte_idx);
}

/*
 * A read returns exactly the staged bytes, in order.
 *
 * Per MIPI I3C Basic v1.0 5.1.2.3.4, "the Slave controls the number of
 * data Words it returns".  Verified at both ends: Board A reports what
 * it saw on the bus, which is the only evidence that matters.
 */
ZTEST(dual_board_loopback_target, test_read_returns_staged_bytes)
{
	static const size_t len = 12U;
	uint8_t staged[TARGET_BUF_MAX];
	uint8_t got[TARGET_BUF_MAX];
	int rc;
	int a_rc = -1;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	fill_pattern(staged, len, 0x31);

	rc = i3c_target_tx_write(i3c_dev, staged, (uint16_t)len, 0);
	zassert_equal(rc, (int)len, "staging %zu bytes accepted only %d", len, rc);

	memset(got, 0, sizeof(got));
	rc = ctrl_read(len, got, sizeof(got), &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "Board A read failed (%d)", a_rc);

	for (size_t i = 0; i < len; i++) {
		zassert_equal(got[i], staged[i], "byte %zu: Board A read 0x%02x, staged 0x%02x", i,
			      got[i], staged[i]);
	}
}

/*
 * Exactly one completion per read transaction.
 */
ZTEST(dual_board_loopback_target, test_read_completion_signalled)
{
	static const size_t len = 8U;
	uint8_t staged[TARGET_BUF_MAX];
	uint8_t got[TARGET_BUF_MAX];
	int rc;
	int a_rc = -1;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	fill_pattern(staged, len, 0x40);

	rc = target_stage_read(staged, len);
	zassert_ok(rc, "staging failed (%d)", rc);

	rc = ctrl_read(len, got, sizeof(got), &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "Board A read failed (%d)", a_rc);

	rc = event_wait_count(EVENT_STOP, 1U, CB_SETTLE);
	zassert_ok(rc, "no completion within timeout");

	zassert_equal(event_count(EVENT_STOP), 1U, "expected 1 completion, got %zu",
		      event_count(EVENT_STOP));
}

/*
 * A read with nothing staged is reported, and the target still
 * serves the next read correctly.
 *
 * Per DW databook 6.1.9 (INTR_STATUS bit 11) the DW IP raises the read
 * request only "when CMDQ is empty", so this is the underrun path.  The
 * requirement under test is that the application is told and the target
 * stays usable, not that any particular byte appears on the bus.
 */
ZTEST(dual_board_loopback_target, test_unstaged_read_is_reported)
{
	static const size_t len = 4U;
	uint8_t staged[TARGET_BUF_MAX];
	uint8_t got[TARGET_BUF_MAX];
	int rc;
	int a_rc = -1;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	/* Nothing staged: expect the underrun notification. */
	rc = ctrl_read(len, got, sizeof(got), &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);

	rc = event_wait_count(EVENT_READ_REQUESTED, 1U, CB_SETTLE);
	zassert_ok(rc, "unstaged read was not reported to the application");

	/* Recovery: a properly staged read must still work afterwards. */
	event_reset();
	fill_pattern(staged, len, 0x5A);

	rc = target_stage_read(staged, len);
	zassert_ok(rc, "staging after underrun failed (%d)", rc);

	memset(got, 0, sizeof(got));
	rc = ctrl_read(len, got, sizeof(got), &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "read after underrun failed (%d)", a_rc);

	for (size_t i = 0; i < len; i++) {
		zassert_equal(got[i], staged[i], "byte %zu: got 0x%02x want 0x%02x after underrun",
			      i, got[i], staged[i]);
	}
}

/*
 * Traffic addressed elsewhere leaves this target untouched.
 *
 * The negative control is the second half: the same counters must move
 * for a write that is addressed to us, otherwise the first half would
 * pass against a dead event log.
 */
ZTEST(dual_board_loopback_target, test_unaddressed_traffic_ignored)
{
	static const size_t len = 4U;
	uint8_t pattern[TARGET_BUF_MAX];
	uint8_t foreign_addr;
	size_t before;
	int rc;
	int a_rc = -1;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	fill_pattern(pattern, len, 0x6C);

	/* Any address that is neither ours nor the bench's permanent target. */
	foreign_addr = (target_da != 0x20U) ? 0x20U : 0x21U;

	before = event_total();
	rc = ctrl_write_addr(foreign_addr, pattern, len, &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);

	/* Board A's own return code is not asserted: an unowned address is
	 * expected to NACK. What matters is that we stayed silent.
	 */
	k_msleep(50);
	if (event_total() != before) {
		event_dump();
	}
	zassert_equal(event_total(), before,
		      "target reacted to traffic for 0x%02x: %zu new events", foreign_addr,
		      event_total() - before);

	/* Negative control: the log is live. */
	event_reset();
	rc = ctrl_write(pattern, len, &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "control write failed (%d)", a_rc);

	rc = event_wait_count(EVENT_WRITE_BYTE, len, CB_SETTLE);
	zassert_ok(rc, "control write produced no events: the previous check proved nothing");
}

/*
 * A callback table with NULL members is handled without fault.
 */
ZTEST(dual_board_loopback_target, test_partial_callback_table)
{
	static const size_t len = 4U;
	static const struct i3c_target_callbacks partial = {
		.write_received_cb = NULL,
		.stop_cb = NULL,
	};
	struct i3c_target_config partial_cfg = {
		.callbacks = &partial,
	};
	uint8_t pattern[TARGET_BUF_MAX];
	int rc;
	int a_rc = -1;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	fill_pattern(pattern, len, 0x70);

	rc = target_bind(&partial_cfg);
	zassert_true((rc == 0) || (rc == -ENOSYS), "register(partial) failed (%d)", rc);

	rc = ctrl_write(pattern, len, &a_rc);

	/* Restore before asserting, so a failure cannot strand the suite
	 * with an empty callback table.
	 */
	(void)target_bind(&target_cfg);

	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "write with a partial callback table failed (%d)", a_rc);

	/* The full table is back: prove it by exercising it. */
	event_reset();
	rc = ctrl_write(pattern, len, &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "write after restoring callbacks failed (%d)", a_rc);

	rc = event_wait_count(EVENT_WRITE_BYTE, len, CB_SETTLE);
	zassert_ok(rc, "callbacks were not restored");
}

/*
 * Consecutive transactions do not leak state into each other.
 *
 * Regression test.  Read data is pre-staged into SLV_TX, and the driver used
 * to leave whatever the controller did not take sitting in the FIFO: the next
 * read was served the previous buffer's leftover bytes, and every read after
 * it stayed offset by the same amount.  Nothing an application can call
 * discards staged bytes, so once it started the bus needed re-enumerating.
 *
 * Each iteration carries its index in byte 0, so a recurrence shows up as
 * iteration n returning iteration n-1's data rather than as a vague count
 * mismatch.
 */
ZTEST(dual_board_loopback_target, test_back_to_back_transactions)
{
	static const size_t iterations = 20U;
	static const size_t len = 8U;
	uint8_t pattern[TARGET_BUF_MAX];
	uint8_t staged[TARGET_BUF_MAX];
	uint8_t got[TARGET_BUF_MAX];
	int rc;
	int a_rc = -1;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	for (size_t i = 0; i < iterations; i++) {
		event_reset();

		fill_pattern(pattern, len, (uint8_t)i);
		rc = ctrl_write(pattern, len, &a_rc);
		zassert_ok(rc, "iteration %zu: no reply from Board A (%d)", i, rc);
		zassert_ok(a_rc, "iteration %zu: write failed (%d)", i, a_rc);

		rc = event_wait_count(EVENT_WRITE_BYTE, len, CB_SETTLE);
		zassert_ok(rc, "iteration %zu: only %zu of %zu bytes arrived", i,
			   event_count(EVENT_WRITE_BYTE), len);
		assert_payload(pattern, len);

		zassert_equal(event_count(EVENT_STOP), 1U,
			      "iteration %zu: expected 1 completion, got %zu", i,
			      event_count(EVENT_STOP));

		fill_pattern(staged, len, (uint8_t)(0x80U + i));
		rc = target_stage_read(staged, len);
		zassert_ok(rc, "iteration %zu: staging failed (%d)", i, rc);

		memset(got, 0, sizeof(got));
		rc = ctrl_read(len, got, sizeof(got), &a_rc);
		zassert_ok(rc, "iteration %zu: no reply from Board A (%d)", i, rc);
		zassert_ok(a_rc, "iteration %zu: read failed (%d)", i, a_rc);

		for (size_t b = 0; b < len; b++) {
			zassert_equal(got[b], staged[b],
				      "iteration %zu byte %zu: got 0x%02x want 0x%02x "
				      "(stale data from a previous transaction?)",
				      i, b, got[b], staged[b]);
		}
	}
}
