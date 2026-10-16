/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Illegally formatted CCCs, asserted from the target.
 *
 * Per MIPI I3C Basic v1.0 5.1.10.1.6: "If the Slave detects an illegally
 * formatted CCC, then the Slave shall generate NACK (after the Dynamic
 * Address) and then wait for STOP or Repeated START.  An example of an
 * illegally formatted CCC would be if the Slave receives the Dynamic
 * Address/Write during the GETBCR CCC."
 *
 * That example is the whole basis of this group: the illegality is the
 * *direction*, not the command code, so a Get CCC addressed for Write is a
 * malformed frame the spec itself names.  The NACK is mandatory, which gives
 * these cases a requirement-grade oracle: the CCC must not succeed.
 *
 * Two things are deliberately not asserted here.
 *
 * 1. Partial application.  5.1.9.2.1 says that when the controller "invalidly
 *    terminates the data associated with a CCC prematurely", the target
 *    "shall use best efforts to handle the termination [...] That is, the
 *    Slave may choose to disregard the event with no effect, or the Slave may
 *    process a partial impact from the incomplete data."  Both outcomes are
 *    conformant, so asserting that a truncated SETMWL leaves MWL untouched
 *    would pin an implementation choice.  What is required is the "best
 *    efforts" part: the target copes and stays usable.
 *
 * 2. The GETSTATUS protocol-error bit.  It is the natural target-side
 *    observable, but Table 41 says of it: "The Slave might or might not be
 *    able to check for such errors", so a target that never sets it is still
 *    conformant.  It was measured once and this target leaves it clear, so
 *    it is not asserted.
 */

#include "test_common.h"

#include <zephyr/drivers/i3c/ccc.h>

#define S5_PROBE_LEN 4U

/* SETMWL carries a two-byte length; one byte is a truncated frame. */
#define S5_SETMWL_SHORT_LEN 1U

/* Arbitrary, distinct from the boot value so a no-op is visible. */
#define S5_MWL_BASELINE     32U
#define S5_MWL_BASELINE_ARG "SETMWL 32"

/**
 * @brief Fail unless Board A actually put the malformed CCC on the bus.
 *
 * dw_i3c_do_ccc() rejects a few invalid args locally -- a non-zero length with no
 * buffer, more targets than the hardware can tag, a direct CCC with no payload array
 * -- and none of those reach the wire.  A case that tripped one of them would be
 * asserting about an idle bus, exactly the trap test_invalid_addr.c documents, so
 * the harness rc is checked before anything else.
 */
static void assert_ccc_reached_bus(uint8_t id, int harness_rc, int a_rc)
{
	zassert_ok(harness_rc, "harness: no reply from Board A for CCC 0x%02x (%d)", id,
		   harness_rc);
	zassert_not_equal(a_rc, -ENOTSUP,
			  "CCC 0x%02x: Board A's driver refused the shape locally (-ENOTSUP); "
			  "nothing reached the bus and this case proves nothing",
			  id);
}

/**
 * @brief Send one malformed CCC and assert the mandatory NACK.
 *
 * @param id Raw CCC ID.
 * @param read Direction for the direct form.
 * @param len Payload length in bytes.
 */
static void s5_inject(uint8_t id, bool read, size_t len, const char *tag)
{
	int a_rc = -1;
	int rc;

	rc = ctrl_raw_ccc(id, read, len, &a_rc, NULL);
	assert_ccc_reached_bus(id, rc, a_rc);

	/* The clause requires a NACK, not a particular Zephyr errno, since the
	 * various I3C drivers disagree on which error code to return here
	 */
	zassert_not_equal(a_rc, 0,
			  "CCC 0x%02x (%s): reached the bus and succeeded; 5.1.10.1.6 requires "
			  "the target to \"generate NACK (after the Dynamic Address)\" for an "
			  "illegally formatted CCC",
			  id, tag);
}

/*
 * Per MIPI I3C Basic v1.0 5.1.10.1.6: "An example of an illegally formatted
 * CCC would be if the Slave receives the Dynamic Address/Write during the
 * GETBCR CCC."
 *
 * The spec's own example: a Get CCC addressed for Write.
 *
 * GETBCR is a read command, so addressing the target with RnW=0 makes the
 * frame illegal by direction alone, with a legal command code and a legal
 * address.  That isolates the one thing under test.
 */
ZTEST(dual_board_loopback_target, test_s5_get_ccc_wrong_direction)
{
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = ctrl_round_trip(0x11U, S5_PROBE_LEN);
	zassert_ok(rc, "baseline round trip failed (%d)", rc);

	s5_inject(I3C_CCC_GETBCR, false, 1U, "GETBCR/W");

	rc = ctrl_round_trip(0x22U, S5_PROBE_LEN);
	zassert_ok(rc, "target did not return to service after GETBCR/W (%d)", rc);
}

/*
 * Per MIPI I3C Basic v1.0 5.1.10.1.6, the illegality in the spec's example is
 * the direction: "the Slave receives the Dynamic Address/Write during the
 * GETBCR CCC".  GETMWL is likewise a Get command -- Table 26 frames the
 * direct MWL commands as "Sr Slave Addr / RnW / ACK", the Get form being the
 * read -- so addressing it for Write is the same class of malformed frame.
 *
 * The direction error is a class, not one command.
 *
 * test_s5_get_ccc_wrong_direction could pass because a target special-cases
 * GETBCR.  A second Get CCC, one that returns a settable value rather than a
 * fixed capability register, is what shows the check is on the direction.
 */
ZTEST(dual_board_loopback_target, test_s5_second_get_ccc_wrong_direction)
{
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = ctrl_round_trip(0x33U, S5_PROBE_LEN);
	zassert_ok(rc, "baseline round trip failed (%d)", rc);

	s5_inject(I3C_CCC_GETMWL, false, 1U, "GETMWL/W");

	rc = ctrl_round_trip(0x44U, S5_PROBE_LEN);
	zassert_ok(rc, "target did not return to service after GETMWL/W (%d)", rc);
}

/*
 * Per MIPI I3C Basic v1.0 5.1.9.2.1: "If the Master invalidly terminates the
 * data associated with a CCC prematurely, then the Slave shall use best
 * efforts to handle the termination and ascertain the proper course of
 * action.  That is, the Slave may choose to disregard the event with no
 * effect, or the Slave may process a partial impact from the incomplete
 * data."
 *
 * A truncated defined-length CCC is survivable.
 *
 * Both outcomes the clause names are conformant, so the resulting MWL is not
 * asserted.  "Best efforts" is what is required, and what is asserted: the
 * data path still works and the controller can still set MWL correctly
 * afterwards.  A target that wedged, stopped answering, or could no longer be
 * configured would fail.
 */
ZTEST(dual_board_loopback_target, test_s5_truncated_ccc_is_survivable)
{
	unsigned int mwl = 0;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = ctrl_ccc(S5_MWL_BASELINE_ARG, &a_rc, NULL);
	zassert_ok(rc, "harness: SETMWL baseline had no reply (%d)", rc);
	zassert_ok(a_rc, "baseline SETMWL was rejected (%d)", a_rc);

	rc = ctrl_raw_ccc(I3C_CCC_SETMWL(false), false, S5_SETMWL_SHORT_LEN, &a_rc, NULL);
	assert_ccc_reached_bus(I3C_CCC_SETMWL(false), rc, a_rc);

	rc = ctrl_round_trip(0x55U, S5_PROBE_LEN);
	zassert_ok(rc, "target stopped serving traffic after a truncated SETMWL (%d)", rc);

	/* The recoverability half: whatever the truncated frame did, a
	 * well-formed one must still land.
	 */
	rc = ctrl_ccc(S5_MWL_BASELINE_ARG, &a_rc, NULL);
	zassert_ok(rc, "harness: recovery SETMWL had no reply (%d)", rc);
	zassert_ok(a_rc, "SETMWL was rejected after a truncated one (%d)", a_rc);

	rc = ctrl_ccc("GETMWL", &a_rc, &mwl);
	zassert_ok(rc, "harness: GETMWL had no reply (%d)", rc);
	zassert_ok(a_rc, "GETMWL was rejected (%d)", a_rc);
	zassert_equal(mwl, S5_MWL_BASELINE,
		      "MWL is %u, want %u: the target could not be reconfigured after a "
		      "truncated CCC",
		      mwl, S5_MWL_BASELINE);
}
