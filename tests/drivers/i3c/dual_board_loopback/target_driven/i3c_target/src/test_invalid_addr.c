/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Invalid address handling, asserted from the target.
 *
 * Per MIPI I3C Basic v1.0 5.1.10.1: "The seven Error Types summarized in
 * Table 52 shall be supported for all I3C Slave Devices."  Table 52 defines
 * error type S0 as the detection of any of 7'h3E/W, 7'h5E/W, 7'h6E/W,
 * 7'h76/W, 7'h7A/W, 7'h7C/W, 7'h7F/W or 7'h7E/R, and 5.1.10.1.1 says the
 * target "shall consider receipt of any of these restricted Addresses, or the
 * receipt of 7'h7E/R, as an error and shall then ignore the rest of the
 * signal until the HDR EXIT pattern is detected".
 *
 * What is observable from the application is narrower than that: no data may
 * be delivered as though the transfer were ours, and the target must still be
 * serving traffic afterwards.  Only the first half is a requirement.  This
 * bench emits no HDR EXIT pattern, so the target can only come back by
 * Table 52's recovery method b -- "(Optional) If both SCL and SDA stay at
 * High level for a period greater than 60 us, then enable STOP or START
 * detector to exit from the S0/S1 error situation" -- and that method is
 * marked optional.  A target implementing only mandatory method a would stay
 * deaf here and fail the recovery half of every case below without violating
 * the spec.  These cases assert it anyway because this target implements it;
 * read such a failure as "method b stopped working", not as non-conformance.

 */

#include "test_common.h"

/* Table 52, error type S0, write-side triggers. 7'h7E/R is S0_BCAST_ADDR. */
static const uint8_t s0_reserved_addrs[] = {
	0x3EU, 0x5EU, 0x6EU, 0x76U, 0x7AU, 0x7CU, 0x7FU,
};

#define S0_BCAST_ADDR 0x7EU
#define S0_PROBE_LEN  4U

/**
 * @brief Fail unless Board A actually put @p addr on the bus.
 *
 * i3c_transfer() resolves the target through the bus's attached-device list
 * and i3c_attach_i3c_device() refuses every reserved slot, so the easiest way
 * for this whole group to pass is for no transaction to occur at all.  Board
 * A works around both (see xfer_at_addr() there); -EINVAL or -EADDRNOTAVAIL
 * coming back means that workaround stopped working and the case below would
 * otherwise be asserting on an idle bus.
 *
 * A reserved address is owned by nobody, so the transfer must also fail: a
 * success would mean something ACKed it.
 */
static void assert_stimulus_reached_bus(uint8_t addr, int a_rc)
{
	zassert_not_equal(a_rc, -EINVAL,
			  "addr 0x%02x: Board A rejected the transfer locally (-EINVAL); "
			  "nothing reached the bus and this case proves nothing",
			  addr);
	zassert_not_equal(a_rc, -EADDRNOTAVAIL,
			  "addr 0x%02x: Board A could not attach the address (-EADDRNOTAVAIL); "
			  "nothing reached the bus and this case proves nothing",
			  addr);
	zassert_not_equal(a_rc, 0, "addr 0x%02x: the transfer succeeded, so something ACKed a "
				   "reserved address", addr);
}

/* Every probe in this group is the same size; ctrl_round_trip() does the work. */
static int s0_round_trip(uint8_t seed)
{
	return ctrl_round_trip(seed, S0_PROBE_LEN);
}

/*
 * Transfers allowed for the target to resynchronise after an S0 injection.
 *
 * The spec bounds the recovery in time, not in transactions: Table 52 method
 * b re-arms the STOP/START detector after 60 us of idle, so the first edge
 * the target sees afterwards is spent leaving the error state rather than
 * decoding an address.  Measured on this bench, every one of the ten
 * injections below costs exactly one transfer -- the second always succeeds.
 * The bound is deliberately a little looser than the measurement, because
 * "exactly one" is an observation about this target and not a requirement;
 * what the group asserts is that service comes back promptly and by itself.
 *
 * Instrumenting this loop to also recover Board A's controller between
 * attempts changed nothing (the recovery returned 0 and the next transfer
 * still failed), which is what rules out a controller-side halt and places
 * the lost transfer on the target.
 */
#define S0_RESYNC_XFERS 4U

/**
 * @brief Round trip until the target answers again, counting the attempts.
 *
 * @param attempts_out Attempts used, reported in the caller's failure message.
 *
 * @retval 0 The target answered within S0_RESYNC_XFERS transfers.
 */
static int s0_regain_service(uint8_t seed, size_t *attempts_out)
{
	int rc = -EIO;

	for (size_t i = 0; i < S0_RESYNC_XFERS; i++) {
		rc = s0_round_trip((uint8_t)(seed + i));
		*attempts_out = i + 1U;
		if (rc == 0) {
			break;
		}
	}

	return rc;
}

/**
 * @brief Drive @p addr and assert the target stayed silent.
 *
 * @param read true to emit @p addr with RnW=1, false for a write.
 */
static void s0_inject(uint8_t addr, bool read)
{
	static const uint8_t probe[S0_PROBE_LEN] = {0xDEU, 0xADU, 0xBEU, 0xEFU};
	uint8_t got[TARGET_BUF_MAX];
	size_t before;
	size_t reacted;
	int a_rc = -1;
	int rc;

	before = event_total();

	if (read) {
		rc = ctrl_read_addr(addr, S0_PROBE_LEN, got, sizeof(got), &a_rc);
	} else {
		rc = ctrl_write_addr(addr, probe, sizeof(probe), &a_rc);
	}
	zassert_ok(rc, "harness: no reply from Board A for 0x%02x (%d)", addr, rc);

	assert_stimulus_reached_bus(addr, a_rc);

	/* The ISR may still be draining when Board A's reply lands. */
	k_msleep(50);

	/* Read the counter once: a second read could report a different count
	 * than the one compared, and misattribute the failure.
	 */
	reacted = event_total() - before;
	if (reacted != 0U) {
		event_dump();
	}
	zassert_equal(reacted, 0U,
		      "addr 0x%02x/%c: the target reacted with %zu events; a reserved address "
		      "must not be treated as addressed",
		      addr, read ? 'R' : 'W', reacted);
}

/*
 * Per MIPI I3C Basic v1.0 5.1.10.1.1: the target "shall consider receipt of
 * any of these restricted Addresses, or the receipt of 7'h7E/R, as an error
 * and shall then ignore the rest of the signal until the HDR EXIT pattern is
 * detected".
 *
 * A write to 7'h3E is not treated as addressed, and the target is
 * still in service afterwards.
 *
 * The trailing round trip is the half that matters: "no events" alone is also
 * what a target that has died would report.
 */
ZTEST(dual_board_loopback_target, test_s0_reserved_addr_3e)
{
	size_t attempts = 0;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	/* Positive control: the event log is live before the injection. */
	rc = s0_round_trip(0x11U);
	zassert_ok(rc, "baseline round trip failed (%d)", rc);

	s0_inject(0x3EU, false);

	rc = s0_regain_service(0x22U, &attempts);
	zassert_ok(rc, "target did not return to service within %u transfers after 7'h3E/W "
		       "(%zu attempted, last %d)",
		   S0_RESYNC_XFERS, attempts, rc);
}

/*
 * Per MIPI I3C Basic v1.0 Table 52, error type S0 is the detection of any of
 * "7'h3E / W, 7'h5E / W, 7'h6E / W, 7'h76 / W, 7'h7A / W, 7'h7C / W,
 * 7'h7F / W, 7'h7E / R" -- one error type, eight triggers.
 *
 * Every write-side S0 address behaves the same way.
 *
 * A round trip between each keeps a failure attributable to one address
 * rather than to the accumulated effect of the sweep.
 */
ZTEST(dual_board_loopback_target, test_s0_reserved_addr_sweep)
{
	size_t attempts = 0;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = s0_round_trip(0x33U);
	zassert_ok(rc, "baseline round trip failed (%d)", rc);

	for (size_t i = 0; i < ARRAY_SIZE(s0_reserved_addrs); i++) {
		s0_inject(s0_reserved_addrs[i], false);

		rc = s0_regain_service((uint8_t)(0x40U + i), &attempts);
		zassert_ok(rc, "target did not return to service within %u transfers after "
			       "0x%02x/W (%zu attempted, last %d)",
			   S0_RESYNC_XFERS, s0_reserved_addrs[i], attempts, rc);
	}
}

/*
 * Per MIPI I3C Basic v1.0 Table 52 note 1: "In the ENTDAA mode, '7'hE / R'
 * shall be excluded from the S0 error definition."
 *
 * 7'h7E/R outside ENTDAA is an S0 trigger, not a private read.
 *
 * The exclusion is what makes this an error here: no dynamic address
 * assignment is in progress.  7'h7E/W is deliberately not exercised -- it is
 * the legal broadcast address and would start a CCC.
 */
ZTEST(dual_board_loopback_target, test_s0_broadcast_read)
{
	size_t attempts = 0;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = s0_round_trip(0x55U);
	zassert_ok(rc, "baseline round trip failed (%d)", rc);

	s0_inject(S0_BCAST_ADDR, true);

	rc = s0_regain_service(0x66U, &attempts);
	zassert_ok(rc, "target did not return to service within %u transfers after 7'h7E/R "
		       "(%zu attempted, last %d)",
		   S0_RESYNC_XFERS, attempts, rc);
}

/*
 * Per MIPI I3C Basic v1.0 Table 52, S0 recovery method b: "(Optional) If both
 * SCL and SDA stay at High level for a period greater than 60 us, then enable
 * STOP or START detector to exit from the S0/S1 error situation."  Exiting
 * the error situation means exiting it completely, not partially.
 *
 * Recovery is complete, not merely immediate.
 *
 * One round trip after an injection can pass while the target is still
 * degraded; ten consecutive verified transfers is what catches a target that
 * survives the error but drops or reorders traffic afterwards.
 */
ZTEST(dual_board_loopback_target, test_s0_recovery_is_complete)
{
	static const size_t iterations = 10U;
	size_t attempts = 0;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	s0_inject(0x7FU, false);

	rc = s0_regain_service(0x70U, &attempts);
	zassert_ok(rc, "target did not return to service within %u transfers after 7'h7F/W "
		       "(%zu attempted, last %d)",
		   S0_RESYNC_XFERS, attempts, rc);

	for (size_t i = 0; i < iterations; i++) {
		rc = s0_round_trip((uint8_t)(0x80U + i));
		zassert_ok(rc, "round trip %zu of %zu failed after the S0 injection (%d)", i + 1U,
			   iterations, rc);
	}
}
