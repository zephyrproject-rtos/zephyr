/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * The target's recovery behaviour, observed from the target side.
 *
 * Recovery from a soft reset needs a way to reset the target block that the
 * API does not offer: i3c_recover_bus() is controller-only and returns
 * -EACCES on a target, and driving RESET_CTRL directly leaves the vendor
 * wrapper gate shut because the hook that reopens it is private to the
 * driver.
 */

#include "test_common.h"

#include <string.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/ztest.h>

#define ABORT_READ_ITERATIONS 10U

/* Spare address the bench guarantees is free; see the Kconfig help. */
#define ALT_DA CONFIG_I3C_DUAL_BOARD_LOOPBACK_TARGET_ALT_ADDR

/* Stage a pattern and have A read all of it back, verified byte for byte. */
static int probe_round_trip(uint8_t seed, size_t len, uint8_t *got)
{
	uint8_t pattern[TARGET_BUF_MAX];
	int a_rc = -1;
	int rc;

	fill_pattern(pattern, len, seed);

	rc = target_stage_read(pattern, len);
	if (rc != 0) {
		return rc;
	}

	rc = ctrl_read(len, got, TARGET_BUF_MAX, &a_rc);
	if (rc != 0) {
		return rc;
	}
	if (a_rc != 0) {
		return a_rc;
	}
	if (memcmp(got, pattern, len) != 0) {
		return -EBADMSG;
	}
	return 0;
}

/*
 * A target that has been NACKing returns to service once the condition clears.
 *
 * The NACK condition is the target's address moving out from under the
 * controller: after SETNEWDA nobody owns the old address, so transfers to it
 * are NACKed.  test_setnewda_applied proves no data arrives there but never
 * checks that the controller saw a NACK, which is the half that makes the recovery
 * meaningful -- a target that had simply gone deaf would satisfy "no bytes
 * arrived" too.
 */
ZTEST(dual_board_loopback_target, test_recover_after_nack_condition)
{
	const uint8_t probe[4] = {0xde, 0xad, 0xbe, 0xef};
	uint8_t got[TARGET_BUF_MAX];
	uint8_t old_da = target_da;
	int nack_rc = 0;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	zassert_not_equal(old_da, ALT_DA, "bench DA collides with the test address");

	/* Positive control: service works before the condition is asserted. */
	rc = probe_round_trip(0x3a, 8U, got);
	zassert_ok(rc, "baseline round trip failed (%d)", rc);

	rc = ctrl_ccc("SETNEWDA " STRINGIFY(ALT_DA), &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "SETNEWDA failed (%d)", a_rc);

	rc = ctrl_write_addr(old_da, probe, sizeof(probe), &nack_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);

	/* Clear the condition before asserting, so a failure here does not
	 * leave the bus on the wrong address for every later case.
	 */
	(void)ctrl_re_enum(&a_rc);

	zassert_not_equal(nack_rc, 0,
			  "the controller reported success writing to the vacated DA 0x%02x",
			  old_da);

	rc = probe_round_trip(0x7c, 8U, got);
	zassert_ok(rc, "target did not return to service after the NACK condition (%d)", rc);
}

/*
 * An aborted write leaves the target idle and able to serve the next
 * transaction.
 *
 * Source: MIPI I3C Basic v1.0 5.1.2.3.4, the write-side counterpart of
 * test_aborted_read_leaves_target_usable.
 *
 * Board A truncates the write with DEVICE_CTRL ABORT. Requiring 0 < arrived <
 * sent is what makes this the aborted-mid-data case rather than a write that
 * never started: a no-op abort delivers all of it, and an abort that landed
 * before the data phase delivers none. Measured 2026-09-23 over delays of
 * 5..120 us, 200 bytes sent: 17, 16, 22, 28, 43 and 57 bytes arrived, so the
 * window is wide and the delay below is not critical.
 */
#define ABORT_LEN      200U
#define ABORT_DELAY_US 50U

ZTEST(dual_board_loopback_target, test_abort_during_write)
{
	uint8_t got[TARGET_BUF_MAX];
	size_t sent = 0;
	size_t arrived;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = probe_round_trip(0x11, 8U, got);
	zassert_ok(rc, "baseline round trip failed (%d)", rc);

	event_reset();
	rc = ctrl_write_abort(ABORT_LEN, ABORT_DELAY_US, &sent, &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_equal(sent, ABORT_LEN, "Board A sent %zu bytes, expected %u", sent,
		      ABORT_LEN);

	/* Bytes are still in flight when A replies. */
	k_sleep(CB_SETTLE);
	arrived = event_count(EVENT_WRITE_BYTE);

	zassert_true(arrived > 0U, "no bytes arrived, so the write was aborted before its data");
	zassert_true(arrived < sent, "all %zu bytes arrived, so the write was never aborted",
		     arrived);

	rc = probe_round_trip(0x22, 8U, got);
	zassert_ok(rc, "target did not serve the next transaction after an aborted write (%d)",
		   rc);
}

/*
 * An aborted read leaves the target able to serve the next transaction.
 *
 * Source: MIPI I3C Basic v1.0 5.1.2.3.4 - the controller "may abort the Read
 * prematurely when necessary", and on a Repeated START "the Slave shall not
 * drive SDA after that". The spec requires the target to stop driving; it says
 * nothing about becoming unusable afterwards.
 *
 * The target block NACKs every transfer after an error until the FIFOs are
 * reset and RESUME is written (DW databook 6.1.24 SLAVE_BUSY). The driver
 * performs that recovery from the response handler, so no controller action
 * is needed to unwedge it.
 *
 * Looped rather than run once because each iteration stages a distinct
 * pattern, lets the controller take only part of it, and then checks the
 * bytes the next read returns: anything the previous aborted read left queued
 * shows up here as a mismatch. A single pass has no previous aborted read to
 * carry residue from, so it cannot make that check at all.
 *
 * Measured: nothing carries over. The abort retires the untaken bytes in
 * hardware, with no driver action and no rebinding needed.
 */
ZTEST(dual_board_loopback_target, test_aborted_read_leaves_target_usable)
{
	uint8_t got[TARGET_BUF_MAX];
	unsigned int failures = 0;
	int first_rc = 0;
	unsigned int first_iter = 0;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	/* Positive control: the round trip works before anything is aborted. */
	zassert_ok(probe_round_trip(0x10, 8U, got), "baseline round trip failed");

	for (unsigned int i = 0; i < ABORT_READ_ITERATIONS; i++) {
		uint8_t pattern[TARGET_BUF_MAX];
		int a_rc = -1;
		int rc;

		/* Stage more than A will take, so the read ends early. */
		fill_pattern(pattern, 16U, (uint8_t)(0x20 + i));
		rc = target_stage_read(pattern, 16U);
		zassert_ok(rc, "iter %u: staging failed (%d)", i, rc);

		memset(got, 0, sizeof(got));
		rc = ctrl_read(4U, got, sizeof(got), &a_rc);
		zassert_ok(rc, "iter %u: harness: no reply from Board A (%d)", i, rc);

		/* The abort itself is legal, so its rc is not the subject here.
		 * What the controller took before aborting is.
		 */
		for (size_t j = 0; j < 4U; j++) {
			zassert_equal(got[j], pattern[j],
				      "iter %u byte %zu: got 0x%02x want 0x%02x; the previous "
				      "aborted read left its untaken bytes queued",
				      i, j, got[j], pattern[j]);
		}

		rc = probe_round_trip((uint8_t)(0x40 + i), 8U, got);
		if (rc != 0) {
			if (failures == 0U) {
				first_rc = rc;
				first_iter = i;
			}
			failures++;
			/* Clear the wedge so the remaining iterations still
			 * measure the abort, not the previous failure.
			 */
			(void)ctrl_recover(&a_rc);
		}
	}

	/*
	 * Only needed if the driver failed to recover on its own; a green run
	 * must leave the bus usable without it.
	 */
	if (failures != 0U) {
		int restore_rc = -1;

		(void)ctrl_recover(&restore_rc);
		(void)ctrl_re_enum(&restore_rc);
	}

	zassert_equal(failures, 0U,
		      "%u of %u round trips failed after an aborted read; first at iter %u (%d)",
		      failures, ABORT_READ_ITERATIONS, first_iter, first_rc);
}

/*
 * The target stays usable across a controller-side bus recovery.
 *
 * Source: i3c.h i3c_recover_bus() - recovery acts on the controller and the
 * bus lines; nothing in the API says the target must be re-registered or
 * re-enumerated afterwards.
 */
ZTEST(dual_board_loopback_target, test_target_survives_bus_recover)
{
	uint8_t got[TARGET_BUF_MAX];
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	zassert_ok(probe_round_trip(0x70, 8U, got), "baseline round trip failed");

	rc = ctrl_recover(&a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "i3c_recover_bus() failed on Board A (%d)", a_rc);

	zassert_ok(probe_round_trip(0x71, 8U, got), "round trip failed after bus recovery");
}
