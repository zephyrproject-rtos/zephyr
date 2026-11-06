/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Buffer-mode callback contract.
 *
 * Built only when CONFIG_I3C_TARGET_BUFFER_MODE is set: without it the two
 * callbacks are not even members of struct i3c_target_callbacks.
 *
 * These cases assert the contract in include/zephyr/drivers/i3c/target_device.h
 * rather than what any one driver happens to do, so a red case here is a work
 * item on the driver under test.
 */

#include "test_common.h"

#include <zephyr/drivers/i3c.h>
#include <zephyr/ztest.h>

#ifdef CONFIG_I3C_TARGET_BUFFER_MODE

/*
 * A private write arrives once, whole and in order.
 *
 * Source: target_device.h buf_write_received_cb, "invoked by the controller
 * when it completes reception of data from the source buffer to the
 * destination buffer in an ongoing write operation to the device".  One
 * completed write is therefore one callback carrying the whole payload,
 * which is what distinguishes this from the per-byte path in
 * test_write_payload_integrity.
 */
ZTEST(dual_board_loopback_target, test_buf_write_delivers_payload)
{
	uint8_t pattern[8];
	int wait_rc = -1;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	event_reset();
	target_buf_reset();

	fill_pattern(pattern, sizeof(pattern), 0x31);

	rc = target_buf_mode_enter();
	if (rc != 0) {
		(void)target_buf_mode_exit();
		zassert_ok(rc, "could not enter buffer mode (%d)", rc);
	}

	rc = ctrl_write(pattern, sizeof(pattern), &a_rc);
	if (rc == 0) {
		wait_rc = event_wait_count(EVENT_BUF_WRITE, 1U, CB_SETTLE);
	}

	(void)target_buf_mode_exit();

	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "Board A could not write (%d)", a_rc);
	zassert_ok(wait_rc, "buf_write_received_cb never fired for an %zu-byte write",
		   sizeof(pattern));

	zassert_equal(event_count(EVENT_BUF_WRITE), 1U,
		      "one write produced %zu buf callbacks, want exactly 1",
		      event_count(EVENT_BUF_WRITE));
	zassert_equal(target_buf_rx_len, sizeof(pattern), "callback reported %zu bytes, want %zu",
		      target_buf_rx_len, sizeof(pattern));
	zassert_mem_equal(target_buf_rx, pattern, sizeof(pattern),
			  "payload differs or is reordered");
}

/*
 * The reported length is exact at both edges.
 *
 * Source: target_device.h buf_write_received_cb @p len, "the length of the
 * data to be transferred".  A driver that rounded up to a FIFO word, padded,
 * or clamped short would pass test_buf_write_delivers_payload and fail here.
 * The scenario pins CONFIG_I3C_DW_TARGET_BUF_SIZE to TARGET_BUF_MAX so the
 * long case exactly fills the driver buffer; a write longer than the buffer
 * is a configuration error the driver logs, not a case this asserts.
 */
ZTEST(dual_board_loopback_target, test_buf_write_length_boundary)
{
	static const size_t lengths[] = {1U, TARGET_BUF_MAX};

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	for (size_t i = 0; i < ARRAY_SIZE(lengths); i++) {
		uint8_t pattern[TARGET_BUF_MAX];
		const size_t len = lengths[i];
		int wait_rc = -1;
		int a_rc = -1;
		int rc;

		event_reset();
		target_buf_reset();
		fill_pattern(pattern, len, (uint8_t)(0x40 + i));

		rc = target_buf_mode_enter();
		if (rc != 0) {
			(void)target_buf_mode_exit();
			zassert_ok(rc, "could not enter buffer mode (%d)", rc);
		}

		rc = ctrl_write(pattern, len, &a_rc);
		if (rc == 0) {
			wait_rc = event_wait_count(EVENT_BUF_WRITE, 1U, CB_SETTLE);
		}

		(void)target_buf_mode_exit();

		zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
		zassert_ok(a_rc, "Board A could not write %zu bytes (%d)", len, a_rc);
		zassert_ok(wait_rc, "no buf callback for the %zu-byte write", len);

		zassert_equal(target_buf_rx_len, len, "callback reported %zu bytes, want %zu",
			      target_buf_rx_len, len);
		zassert_mem_equal(target_buf_rx, pattern, len, "%zu-byte payload differs", len);
	}
}

/*
 * A buffer staged for reading is returned whole, in order.
 *
 * Source: MIPI I3C Basic v1.0 5.1.2.3 plus target_device.h.  This asserts the
 * requirement, not the mechanism: buf_read_requested_cb cannot supply the data
 * for the transfer under way, because I3C forbids target clock stretching
 * (spec Table 10, "Clock Stretching by Slave" -- Not Allowed), so the target
 * must already hold the bytes when the data phase starts.  Read data is
 * therefore pre-staged with i3c_target_tx_write() in buffer mode exactly as it
 * is in byte mode, and the callback is only an underrun report
 * (test_buf_unstaged_read_is_reported).
 */
ZTEST(dual_board_loopback_target, test_buf_read_returns_staged_bytes)
{
	uint8_t staged[8];
	uint8_t got[TARGET_BUF_MAX];
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	event_reset();
	target_buf_reset();

	fill_pattern(staged, sizeof(staged), 0x51);

	rc = target_buf_mode_enter();
	if (rc != 0) {
		(void)target_buf_mode_exit();
		zassert_ok(rc, "could not enter buffer mode (%d)", rc);
	}

	rc = target_stage_read(staged, sizeof(staged));
	if (rc == 0) {
		rc = ctrl_read(sizeof(staged), got, sizeof(got), &a_rc);
	}

	(void)target_buf_mode_exit();

	zassert_ok(rc, "harness: staging or reply failed (%d)", rc);
	zassert_ok(a_rc, "Board A could not read (%d)", a_rc);
	zassert_mem_equal(got, staged, sizeof(staged),
			  "buffer mode did not return the staged bytes intact");
}

/*
 * A read with nothing staged is reported, not served with stale bytes.
 *
 * Source: target_device.h buf_read_requested_cb, "invoked by the controller
 * when the bus is ready to provide additional data by buffer", read together
 * with DW databook 6.1.9 bit 11: the DW IP raises that event only when
 * the command queue is empty, so it is the underrun case and nothing more.
 * The documented "an error return shall cause the controller to ignore bus
 * operations" cannot be asserted here -- no in-tree I3C driver consumes the
 * return value, because by the time the event fires the transfer is already
 * lost.  What is left, and what this asserts, is that the application is told:
 * the bytes the controller ends up with are the IP's business, not the
 * driver's, so they are deliberately not asserted on.
 */
ZTEST(dual_board_loopback_target, test_buf_unstaged_read_is_reported)
{
	uint8_t got[TARGET_BUF_MAX];
	size_t notes;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	event_reset();
	target_buf_reset();

	rc = target_buf_mode_enter();
	if (rc != 0) {
		(void)target_buf_mode_exit();
		zassert_ok(rc, "could not enter buffer mode (%d)", rc);
	}

	/* Nothing staged on purpose. */
	rc = ctrl_read(4U, got, sizeof(got), &a_rc);
	notes = event_count(EVENT_BUF_READ);

	(void)target_buf_mode_exit();

	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_true(notes > 0U,
		     "the underrun was not reported to the application: rc=%d, %zu reports", a_rc,
		     notes);
}

/*
 * Completion is still signalled exactly once, after the buf callback.
 *
 * Source: target_device.h stop_cb, "invoked when the bus completes a stop
 * condition".  Nothing in the API makes stop_cb conditional on the per-byte
 * path, so buffer mode must not swallow it, and the ordering is what proves
 * the buf callback belongs to the transaction the stop closes.
 */
ZTEST(dual_board_loopback_target, test_buf_stop_still_signalled)
{
	uint8_t pattern[8];
	size_t buf_idx = SIZE_MAX;
	size_t stop_idx = SIZE_MAX;
	int wait_rc = -1;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	event_reset();
	target_buf_reset();

	fill_pattern(pattern, sizeof(pattern), 0x71);

	rc = target_buf_mode_enter();
	if (rc != 0) {
		(void)target_buf_mode_exit();
		zassert_ok(rc, "could not enter buffer mode (%d)", rc);
	}

	rc = ctrl_write(pattern, sizeof(pattern), &a_rc);
	if (rc == 0) {
		wait_rc = event_wait_count(EVENT_STOP, 1U, CB_SETTLE);
	}

	(void)target_buf_mode_exit();

	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "Board A could not write (%d)", a_rc);
	zassert_ok(wait_rc, "stop_cb never fired in buffer mode");

	zassert_equal(event_count(EVENT_STOP), 1U, "one transaction produced %zu stops, want 1",
		      event_count(EVENT_STOP));

	for (size_t i = 0; i < event_stored(); i++) {
		const struct event_entry *ev = event_at(i);

		if ((ev->kind == EVENT_BUF_WRITE) && (buf_idx == SIZE_MAX)) {
			buf_idx = i;
		}
		if ((ev->kind == EVENT_STOP) && (stop_idx == SIZE_MAX)) {
			stop_idx = i;
		}
	}

	zassert_not_equal(buf_idx, SIZE_MAX, "no buf_write_received_cb before the stop");
	zassert_true(buf_idx < stop_idx, "stop_cb fired before the payload was delivered");
}

/*
 * A transfer is delivered by exactly one path, never both.
 *
 * Not a stated contract, but a rule every in-tree driver supporting both
 * modes enforces.  Worth pinning because in i3c_dw.c the exclusivity rests on
 * a single return after the buf callback rather than on #ifdef structure, so
 * a refactor could silently lose it.  No other buffer-mode case would notice:
 * they register a buf-only table, so the per-byte path cannot fire whatever
 * the driver does.
 *
 * Waiting on the stop rather than on the buf callback is what makes the
 * negative half sound: the per-byte callbacks run during the data phase that
 * precedes the stop, so once the stop is in the log both paths have already
 * had their chance to fire.
 */
ZTEST(dual_board_loopback_target, test_buf_path_excludes_byte_path)
{
	uint8_t pattern[8];
	size_t bytes;
	size_t bufs;
	int wait_rc = -1;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	event_reset();
	target_buf_reset();

	fill_pattern(pattern, sizeof(pattern), 0x81);

	rc = target_both_mode_enter();
	if (rc != 0) {
		(void)target_both_mode_exit();
		zassert_ok(rc, "could not register both callback paths (%d)", rc);
	}

	rc = ctrl_write(pattern, sizeof(pattern), &a_rc);
	if (rc == 0) {
		wait_rc = event_wait_count(EVENT_STOP, 1U, CB_SETTLE);
	}

	bytes = event_count(EVENT_WRITE_BYTE);
	bufs = event_count(EVENT_BUF_WRITE);

	(void)target_both_mode_exit();

	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "Board A could not write (%d)", a_rc);
	zassert_ok(wait_rc, "the transaction never completed, so neither path was observed");

	zassert_equal(bufs, 1U, "buffer path delivered the write %zu times, want exactly 1", bufs);
	zassert_equal(bytes, 0U,
		      "the write was double-handled: %zu per-byte callbacks alongside the "
		      "buffer callback, for one %zu-byte transfer",
		      bytes, sizeof(pattern));

	zassert_equal(target_buf_rx_len, sizeof(pattern), "callback reported %zu bytes, want %zu",
		      target_buf_rx_len, sizeof(pattern));
	zassert_mem_equal(target_buf_rx, pattern, sizeof(pattern),
			  "the surviving path did not deliver the payload intact");
}

#endif /* CONFIG_I3C_TARGET_BUFFER_MODE */
