/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Events the target starts: target-initiated read (IBI/TIR) and hot join.
 *
 * Controller-role handoff is not here: both apps in this suite are role-only
 * builds, and controllership transfer needs CONFIG_I3C_DUAL_ROLE with
 * secondary-controller plumbing on both sides.
 */

#include "test_common.h"

#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/ztest.h>

#define IBI_WAIT_MS 1000U

/*
 * A TIR from a target that advertises an IBI payload carries a mandatory
 * MDB plus up to four data bytes (MIPI I3C Basic v1.0 5.1.6.2.2), so the
 * legal request lengths are 1..5 and zero is not a boundary to test.
 */

/* CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE is 0 when IBI is off, which would leave the
 * buffer above zero-length; the cases skip at runtime but still compile.
 */
#define TIR_PAYLOAD_MAX 5U
#define TIR_MAX MIN(TIR_PAYLOAD_MAX, (unsigned int)CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE)

/* I3C_CCC_EVT_* are BIT() macros; STRINGIFY() would yield "(1UL << 0)". */
static int events_set(bool enable, uint8_t mask, int *rc_out)
{
	char cmd[24];

	snprintk(cmd, sizeof(cmd), "%s %u", enable ? "ENEC" : "DISEC", mask);
	return ctrl_ccc(cmd, rc_out, NULL);
}

#ifdef CONFIG_I3C_USE_IBI
static int raise_tir(const uint8_t *payload, uint8_t len)
{
	struct i3c_ibi req = {
		.ibi_type = I3C_IBI_TARGET_INTR,
		.payload = (uint8_t *)payload,
		.payload_len = len,
	};

	return i3c_ibi_raise(i3c_dev, &req);
}
#else
/* i3c_ibi_raise() is compiled out with CONFIG_I3C_USE_IBI=n, but the bodies
 * still have to build so the cases report as skipped instead of vanishing.
 */
static int raise_tir(const uint8_t *payload, uint8_t len)
{
	ARG_UNUSED(payload);
	ARG_UNUSED(len);

	return -ENOSYS;
}
#endif

/* A must be listening before we raise, or the IBI is lost, not delayed. */
static void ibi_arm(void)
{
	int a_rc = -1;
	int rc = ctrl_set_ibi(true, &a_rc);

	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "Board A could not enable IBI (%d)", a_rc);
}

static void ibi_disarm(void)
{
	int a_rc = -1;

	(void)ctrl_set_ibi(false, &a_rc);
}

/*
 * A TIR reaches the controller with the payload we supplied.
 *
 * Source: MIPI I3C Basic v1.0 5.1.6.
 */
ZTEST(dual_board_loopback_target, test_tir_delivers_payload)
{
	static const uint8_t payload[4] = {0x5a, 0xa5, 0x01, 0xfe};
	uint8_t got[TARGET_BUF_MAX];
	size_t got_len = 0;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_IBI();
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	ibi_arm();

	rc = raise_tir(payload, sizeof(payload));
	zassert_ok(rc, "i3c_ibi_raise() failed (%d)", rc);

	rc = ctrl_expect_ibi(IBI_WAIT_MS, got, sizeof(got), &got_len);
	ibi_disarm();

	zassert_ok(rc, "Board A saw no IBI (%d)", rc);
	zassert_equal(got_len, sizeof(payload), "controller got %zu payload bytes, want %zu",
		      got_len, sizeof(payload));
	zassert_mem_equal(got, payload, sizeof(payload), "IBI payload differs");
}

/*
 * Payload length is carried exactly at the edges.
 *
 * Source: MIPI I3C Basic v1.0 5.1.6.2.2.  Distinct patterns per length so
 * a driver that truncated, padded or clamped would fail the comparison
 * rather than accidentally match.
 *
 * The 5-byte case is the one that matters: the IP reports a payload in
 * 4-byte queue entries, so anything shorter cannot catch a driver that
 * mistakes one entry for a whole IBI.
 */
ZTEST(dual_board_loopback_target, test_tir_payload_boundary)
{
	static const uint8_t lengths[] = {1U, 2U, TIR_MAX};
	uint8_t got[TARGET_BUF_MAX];

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_IBI();
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	ibi_arm();

	for (size_t i = 0; i < ARRAY_SIZE(lengths); i++) {
		uint8_t payload[TIR_PAYLOAD_MAX];
		const uint8_t len = lengths[i];
		size_t got_len = 0;
		int rc;

		fill_pattern(payload, len, (uint8_t)(0xd0 + i));

		rc = raise_tir(payload, len);
		if (rc != 0) {
			ibi_disarm();
			zassert_ok(rc, "raise of %u bytes failed (%d)", len, rc);
		}

		rc = ctrl_expect_ibi(IBI_WAIT_MS, got, sizeof(got), &got_len);
		if (rc != 0) {
			ibi_disarm();
			zassert_ok(rc, "no IBI for the %u-byte payload (%d)", len, rc);
		}

		zassert_equal(got_len, len, "controller got %zu bytes, want %u", got_len, len);
		zassert_mem_equal(got, payload, len, "%u-byte payload differs", len);
	}

	ibi_disarm();
}

/*
 * DISEC(INTR) stops our TIRs reaching the controller, ENEC restores them.
 *
 * Source: MIPI I3C Basic v1.0 5.1.9.3.1.  The ENEC half is the negative
 * control: without it this would pass on a bus that had simply stopped
 * carrying IBIs for an unrelated reason.
 *
 * The raise itself may either be refused locally or accepted and then
 * not delivered; the spec constrains only what the controller sees, so
 * the assertion is on delivery.
 */
ZTEST(dual_board_loopback_target, test_tir_refused_when_disabled)
{
	static const uint8_t payload[2] = {0x11, 0x22};
	uint8_t got[TARGET_BUF_MAX];
	size_t got_len = 0;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_IBI();
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	ibi_arm();

	rc = events_set(false, I3C_CCC_EVT_INTR, &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "DISEC failed (%d)", a_rc);

	(void)raise_tir(payload, sizeof(payload));
	rc = ctrl_expect_ibi(IBI_WAIT_MS, got, sizeof(got), &got_len);
	zassert_equal(rc, -EAGAIN, "an IBI was delivered after DISEC(INTR) (rc=%d)", rc);

	rc = events_set(true, I3C_CCC_EVT_INTR, &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "ENEC failed (%d)", a_rc);

	rc = raise_tir(payload, sizeof(payload));
	if (rc == 0) {
		rc = ctrl_expect_ibi(IBI_WAIT_MS, got, sizeof(got), &got_len);
	}
	ibi_disarm();

	zassert_ok(rc, "control: no IBI after ENEC(INTR) restored them (%d)", rc);
	zassert_equal(got_len, sizeof(payload), "control: got %zu bytes, want %zu", got_len,
		      sizeof(payload));
}

/*
 * Clear our dynamic address so the IP will consider a hot join at all, then
 * let A accept one. Leaves the bus with B unaddressed on purpose.
 */
static int hj_arm(void)
{
	int a_rc = -1;
	int rc;

	rc = ctrl_set_hj(true, &a_rc);
	if (rc != 0) {
		return rc;
	}
	if (a_rc != 0) {
		return a_rc;
	}

	/* Directed CCC, so it has to go out while we still have an address. */
	rc = events_set(true, I3C_CCC_EVT_HJ, &a_rc);
	if (rc != 0) {
		return rc;
	}
	if (a_rc != 0) {
		return a_rc;
	}

	rc = ctrl_ccc("RSTDAA", &a_rc, NULL);
	if (rc != 0) {
		return rc;
	}
	return a_rc;
}

static int raise_hj(void)
{
#ifdef CONFIG_I3C_USE_IBI
	struct i3c_ibi req = {.ibi_type = I3C_IBI_HOTJOIN};

	return i3c_ibi_raise(i3c_dev, &req);
#else
	return -ENOSYS;
#endif
}

/*
 * A target with no dynamic address can request one by hot join.
 *
 * Source: MIPI I3C Basic v1.0 5.1.9.3.1 (ENHJ/DISHJ).
 */
ZTEST(dual_board_loopback_target, test_hj_when_permitted)
{
	unsigned int da = 0;
	int restore_rc = -1;
	int raise_rc;
	int hj_rc;
	int rc;

	ztest_test_skip(); /* See README.rst */

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = hj_arm();
	zassert_ok(rc, "harness: could not arm hot join (%d)", rc);

	raise_rc = raise_hj();
	hj_rc = (raise_rc == 0) ? ctrl_expect_hj(2000U, &da) : -ECANCELED;

	/* Restore before asserting: without a DA every later case fails. */
	(void)ctrl_re_enum(&restore_rc);
	/* Re-enumeration re-issues ENEC(HJ); leave it off or a later RSTDAA lets
	 * the IP hot-join itself straight back onto the bus.
	 */
	(void)events_set(false, I3C_CCC_EVT_HJ, &restore_rc);
	(void)ctrl_set_hj(false, &restore_rc);

	zassert_ok(raise_rc, "i3c_ibi_raise(HOTJOIN) failed (%d)", raise_rc);
	zassert_ok(hj_rc, "Board A never serviced the hot join (%d)", hj_rc);
	zassert_not_equal(da, 0U, "hot join completed without assigning a dynamic address");
}

/*
 * The address handed out by hot join actually carries traffic.
 *
 * Source: MIPI I3C Basic v1.0 5.1.9.3.3 and 5.1.9.3.1. test_hj_when_permitted
 * proves a hot join is serviced; this proves the resulting address works, which
 * is what a driver that merely updated its own descriptor would fail.
 */
ZTEST(dual_board_loopback_target, test_hj_after_rstdaa)
{
	uint8_t pattern[8];
	unsigned int da = 0;
	size_t bytes = 0;
	int restore_rc = -1;
	int write_rc = -1;
	int raise_rc;
	int hj_rc;
	int rc;

	ztest_test_skip(); /* See README.rst */

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = hj_arm();
	zassert_ok(rc, "harness: could not arm hot join (%d)", rc);

	raise_rc = raise_hj();
	hj_rc = (raise_rc == 0) ? ctrl_expect_hj(2000U, &da) : -ECANCELED;

	if (hj_rc == 0) {
		event_reset();
		fill_pattern(pattern, sizeof(pattern), 0xe7);
		if (ctrl_write(pattern, sizeof(pattern), &write_rc) == 0) {
			(void)event_wait_count(EVENT_WRITE_BYTE, sizeof(pattern), CB_SETTLE);
			bytes = event_count(EVENT_WRITE_BYTE);
		}
	}

	(void)ctrl_re_enum(&restore_rc);
	(void)events_set(false, I3C_CCC_EVT_HJ, &restore_rc);
	(void)ctrl_set_hj(false, &restore_rc);

	zassert_ok(raise_rc, "i3c_ibi_raise(HOTJOIN) failed (%d)", raise_rc);
	zassert_ok(hj_rc, "Board A never serviced the hot join (%d)", hj_rc);
	zassert_ok(write_rc, "write to the hot-join address 0x%02x failed (%d)", da, write_rc);
	zassert_equal(bytes, sizeof(pattern), "only %zu of %zu bytes arrived at DA 0x%02x", bytes,
		      sizeof(pattern), da);
}
