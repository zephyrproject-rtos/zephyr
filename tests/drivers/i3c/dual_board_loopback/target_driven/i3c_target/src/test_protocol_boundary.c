/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Protocol boundaries, asserted from the target.
 *
 * These are the edges of legal framing rather than errors: a read the
 * controller ends early, a read longer than the target staged, transfers with
 * no data phase at all, the smallest MWL the spec allows, and two messages in
 * one transfer separated by a Repeated START.  Every one of them is something
 * a conformant controller may do, so the target has to cope with all of them.
 *
 * The recurring oracle is MIPI I3C Basic v1.0 5.1.2.3.4: "In I3C SDR [...]
 * the Slave controls the number of data Words it returns; but it also allows
 * the I3C Master to abort the Read prematurely when necessary."  Ending a
 * read early is the controller exercising a right the spec grants it, so the
 * target may not treat it as an error.
 *
 * What is asserted throughout is that the target stays in service and that
 * the bytes it did deliver are the right ones.  Where the spec leaves the
 * outcome open - what a zero-length transfer reports to the application, for
 * instance - the case asserts survival only.
 */

#include "test_common.h"

#define PROBE_LEN 8U

/* Staged length for the abort sweep; the read offsets run 1..STAGED_LEN-1. */
#define STAGED_LEN 16U

/* MIPI I3C Basic v1.0 5.1.9.3.5: "The minimum value that Max Write Length
 * can be set to is 8."
 */
#define MWL_FLOOR 8U

/**
 * @brief Stage @p len patterned bytes for the controller to read.
 *
 * @param seed Pattern seed, distinct per call so a stale buffer cannot pass.
 */
static void stage_pattern(uint8_t *pattern, size_t len, uint8_t seed)
{
	int rc;

	fill_pattern(pattern, len, seed);
	rc = target_stage_read(pattern, len);
	zassert_ok(rc, "staging %zu bytes failed (%d)", len, rc);
}

/**
 * @brief Drop whatever a previous case left staged, so this one starts empty.
 *
 * Staging is i3c_target_tx_write(), which appends to the target's TX FIFO, and
 * an aborted read leaves the bytes it did not take behind.  Rebinding the
 * callback table does not flush them - i3c_target_unregister() drops the
 * binding and nothing else - so this only puts the callbacks in a known
 * state; clearing the residue is the driver's job when it sees the read end
 * early.
 */
static void reset_target(void)
{
	(void)target_bind(&target_cfg);
}

/*
 * Per MIPI I3C Basic v1.0 5.1.2.3.4: the mechanism "allows the I3C Master to
 * abort the Read prematurely when necessary", and on the Repeated START that
 * ends it "the Slave shall not drive SDA after that".
 *
 * Aborting a read at every offset leaves the target usable.
 *
 * test_aborted_read_leaves_target_usable proved this at one offset.  If any
 * part of the post-abort recovery is length-dependent - a FIFO that only
 * wedges when it still holds an odd number of bytes, say - one offset cannot
 * show it and a sweep can.
 *
 * Usability is the whole assertion.  The bytes the aborted read returns are
 * deliberately not checked: what the target has queued depends on the residue
 * the previous abort left behind, and no requirement covers it.
 */
ZTEST(dual_board_loopback_target, test_read_abort_at_every_offset)
{
	uint8_t pattern[TARGET_BUF_MAX];
	uint8_t got[TARGET_BUF_MAX];
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	reset_target();

	rc = ctrl_round_trip(0x11U, PROBE_LEN);
	zassert_ok(rc, "baseline round trip failed (%d)", rc);

	for (size_t off = 1U; off < STAGED_LEN; off++) {
		int a_rc = -1;

		stage_pattern(pattern, STAGED_LEN, (uint8_t)(0x20U + off));

		memset(got, 0, sizeof(got));
		rc = ctrl_read(off, got, sizeof(got), &a_rc);
		zassert_ok(rc, "offset %zu: harness: no reply from Board A (%d)", off, rc);
		ctrl_assert_bus_outcome("aborted read", a_rc);

		rc = ctrl_round_trip((uint8_t)(0x60U + off), PROBE_LEN);
		zassert_ok(rc, "target unusable after a read aborted at offset %zu of %u (%d)",
			   off, STAGED_LEN, rc);
	}

	/* 15 aborts have left the FIFO full of residue; do not hand that to
	 * the next case.
	 */
	reset_target();
}

/*
 * Per MIPI I3C Basic v1.0 5.1.2.3.4, the target ends a read by returning the
 * ninth bit as 0: "The I3C Slave returns the ninth bit as 0 (SDA Low) to end
 * the Message", after which "The I3C Master then shall issue either a STOP
 * [...] or a Repeated START".
 *
 * A read longer than what was staged terminates cleanly.
 *
 * test_unstaged_read_is_reported covers the read with *nothing* staged.  This
 * is the partial case: the target has some data but fewer bytes than asked for,
 * so it must deliver what it has and then end the message rather than pad,
 * stall or wedge.
 */
ZTEST(dual_board_loopback_target, test_read_longer_than_staged)
{
	static const size_t staged = 4U;
	static const size_t asked = 12U;
	uint8_t pattern[TARGET_BUF_MAX];
	uint8_t got[TARGET_BUF_MAX];
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = ctrl_round_trip(0x31U, PROBE_LEN);
	zassert_ok(rc, "baseline round trip failed (%d)", rc);

	reset_target();
	stage_pattern(pattern, staged, 0x77U);

	memset(got, 0, sizeof(got));
	rc = ctrl_read(asked, got, sizeof(got), &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	ctrl_assert_bus_outcome("over-long read", a_rc);

	/* Whatever the controller was told about the short reply, the bytes
	 * that did arrive are the staged ones.
	 */
	for (size_t i = 0; i < staged; i++) {
		zassert_equal(got[i], pattern[i], "byte %zu: got 0x%02x want 0x%02x", i, got[i],
			      pattern[i]);
	}

	rc = ctrl_round_trip(0x42U, PROBE_LEN);
	zassert_ok(rc, "target unusable after a read longer than staged (%d)", rc);

	/* The underrun is this case's doing; do not leave its aftermath to the
	 * next one.
	 */
	reset_target();
}

/*
 * Per MIPI I3C Basic v1.0 5.1.9.3.5: "The minimum value that Max Write Length
 * can be set to is 8."
 *
 * The spec's floor value is accepted and takes effect.
 *
 * test_setmwl_applied proves the SETMWL mechanism at 16.  This is the boundary:
 * 8 is the smallest value the spec permits, and an implementation that clamps,
 * rejects or rounds it would be wrong there and nowhere else.
 */
ZTEST(dual_board_loopback_target, test_mwl_minimum_boundary)
{
	struct i3c_config_target cfg = {0};
	unsigned int got = 0;
	char cmd[16];
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = i3c_config_get(i3c_dev, I3C_CONFIG_TARGET, &cfg);
	zassert_ok(rc, "i3c_config_get(TARGET) failed (%d)", rc);
	zassert_not_equal(cfg.max_write_len, MWL_FLOOR,
			  "MWL is already %u, so setting the floor would prove nothing",
			  cfg.max_write_len);

	snprintk(cmd, sizeof(cmd), "SETMWL %u", MWL_FLOOR);
	rc = ctrl_ccc(cmd, &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "SETMWL %u was rejected, but it is the spec's minimum (%d)",
		   MWL_FLOOR, a_rc);

	rc = i3c_config_get(i3c_dev, I3C_CONFIG_TARGET, &cfg);
	zassert_ok(rc, "i3c_config_get(TARGET) failed (%d)", rc);
	zassert_equal(cfg.max_write_len, MWL_FLOOR, "target MWL is %u, want the floor %u",
		      cfg.max_write_len, MWL_FLOOR);

	rc = ctrl_ccc("GETMWL", &a_rc, &got);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "GETMWL failed (%d)", a_rc);
	zassert_equal(got, MWL_FLOOR, "GETMWL returned %u, want the floor %u", got,
		      MWL_FLOOR);

	/* Put it back: later cases write TARGET_BUF_MAX bytes. */
	snprintk(cmd, sizeof(cmd), "SETMWL %u", (unsigned int)TARGET_BUF_MAX);
	(void)ctrl_ccc(cmd, &a_rc, NULL);

	rc = ctrl_round_trip(0x92U, PROBE_LEN);
	zassert_ok(rc, "target unusable after the MWL floor was applied and restored (%d)", rc);
}

/*
 * Per MIPI I3C Basic v1.0 5.1.2.3.4, a read may be ended by "a Repeated START
 * as shown in Figure 40" rather than a STOP, and 5.1.2.2 notes that "The
 * Address Header following a Repeated START is always driven on SDA using
 * Push-Pull, with the exception of the ACK/NACK".
 *
 * A write and a read in one transfer, with no STOP between them.
 *
 * Every other case in this suite emits one message per STOP.  This is the
 * only one where the target sees a second address header arrive without the
 * bus having gone idle first, so it is the only one that can catch a target
 * that resynchronises on STOP rather than on the address.
 */
ZTEST(dual_board_loopback_target, test_repeated_start_chain)
{
	static const size_t wlen = 4U;
	/* A dword multiple: non-dword staged lengths are what
	 * test_read_non_dword_length covers, not the subject here.
	 */
	static const size_t rlen = 8U;
	uint8_t wpattern[TARGET_BUF_MAX];
	uint8_t rpattern[TARGET_BUF_MAX];
	uint8_t got[TARGET_BUF_MAX];
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = ctrl_round_trip(0xA1U, PROBE_LEN);
	zassert_ok(rc, "baseline round trip failed (%d)", rc);

	reset_target();
	stage_pattern(rpattern, rlen, 0xC3U);

	event_reset();
	fill_pattern(wpattern, wlen, 0xB2U);

	memset(got, 0, sizeof(got));
	rc = ctrl_xfer(wpattern, wlen, rlen, got, sizeof(got), &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "the write-then-read transfer failed (%d)", a_rc);

	/* Both halves must have landed: the write on our callbacks, the read
	 * from our staged buffer.
	 */
	rc = event_wait_count(EVENT_WRITE_BYTE, wlen, CB_SETTLE);
	zassert_ok(rc, "only %zu of %zu written bytes arrived before the Repeated START",
		   event_count(EVENT_WRITE_BYTE), wlen);

	for (size_t i = 0; i < rlen; i++) {
		zassert_equal(got[i], rpattern[i],
			      "read byte %zu after the Repeated START: got 0x%02x want 0x%02x",
			      i, got[i], rpattern[i]);
	}

	rc = ctrl_round_trip(0xD4U, PROBE_LEN);
	zassert_ok(rc, "target unusable after a Repeated START chain (%d)", rc);
}

/*
 * Per MIPI I3C Basic v1.0 5.1.2.3.4: "the Slave controls the number of data
 * Words it returns".  Having staged six, the target owes six.
 *
 * A staged read whose length is not a multiple of 4 returns every byte.
 */
ZTEST(dual_board_loopback_target, test_read_non_dword_length)
{
	static const size_t rlen = 6U;
	uint8_t pattern[TARGET_BUF_MAX];
	uint8_t got[TARGET_BUF_MAX];
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	reset_target();
	stage_pattern(pattern, rlen, 0xE7U);

	memset(got, 0, sizeof(got));
	rc = ctrl_read(rlen, got, sizeof(got), &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "the %zu-byte read failed (%d)", rlen, a_rc);

	for (size_t i = 0; i < rlen; i++) {
		zassert_equal(got[i], pattern[i],
			      "byte %zu of a %zu-byte staged read: got 0x%02x want 0x%02x; "
			      "only whole dwords reached the bus",
			      i, rlen, got[i], pattern[i]);
	}

	/* The stuck dword outlives this case; do not hand it to the next. */
	reset_target();
}
