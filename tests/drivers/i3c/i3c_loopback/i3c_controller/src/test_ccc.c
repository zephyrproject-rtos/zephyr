/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 2: CCC sweep — exercises i3c_ccc_do_* helpers against Board B.
 */

#include "test_common.h"
#include "test_identity.h"

#include <zephyr/drivers/i3c/ccc.h>

ZTEST(i3c_loopback, test_ccc_getbcr)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	struct i3c_ccc_getbcr bcr = {0};
	int rc = i3c_ccc_do_getbcr(&target_b_desc, &bcr);

	zassert_true(rc >= 0 || rc == -ENODEV || rc == -ENXIO, "GETBCR returned unexpected %d", rc);
}

ZTEST(i3c_loopback, test_ccc_getdcr)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	struct i3c_ccc_getdcr dcr = {0};
	int rc = i3c_ccc_do_getdcr(&target_b_desc, &dcr);

	zassert_true(rc >= 0 || rc == -ENODEV || rc == -ENXIO, "GETDCR returned unexpected %d", rc);
}

ZTEST(i3c_loopback, test_ccc_getpid)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	struct i3c_ccc_getpid pid = {0};
	int rc = i3c_ccc_do_getpid(&target_b_desc, &pid);

	zassert_true(rc >= 0 || rc == -ENODEV || rc == -ENXIO, "GETPID returned unexpected %d", rc);
}

ZTEST(i3c_loopback, test_ccc_getmrl)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	struct i3c_ccc_mrl mrl = {0};
	int rc = i3c_ccc_do_getmrl(&target_b_desc, &mrl);

	zassert_true(rc >= 0 || rc == -ENODEV || rc == -ENOSYS || rc == -ENXIO,
		     "GETMRL returned unexpected %d", rc);
}

ZTEST(i3c_loopback, test_ccc_getmwl)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	struct i3c_ccc_mwl mwl = {0};
	int rc = i3c_ccc_do_getmwl(&target_b_desc, &mwl);

	zassert_true(rc >= 0 || rc == -ENODEV || rc == -ENOSYS || rc == -ENXIO,
		     "GETMWL returned unexpected %d", rc);
}

ZTEST(i3c_loopback, test_ccc_setmrl_setmwl_roundtrip)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	struct i3c_ccc_mrl mrl_set = {.len = 64, .ibi_len = 4};
	struct i3c_ccc_mwl mwl_set = {.len = 64};
	struct i3c_ccc_mrl mrl_get = {0};
	struct i3c_ccc_mwl mwl_get = {0};
	int rc;

	rc = i3c_ccc_do_setmrl(&target_b_desc, &mrl_set);
	zassert_true(rc == 0 || rc == -ENXIO, "SETMRL failed (%d)", rc);
	if (rc != 0) {
		return;
	}
	rc = i3c_ccc_do_setmwl(&target_b_desc, &mwl_set);
	zassert_true(rc == 0 || rc == -ENXIO, "SETMWL failed (%d)", rc);
	if (rc != 0) {
		return;
	}

	rc = i3c_ccc_do_getmrl(&target_b_desc, &mrl_get);
	zassert_ok(rc, "GETMRL after SET failed (%d)", rc);
	zassert_equal(mrl_get.len, 64, "MRL mismatch: got %u", mrl_get.len);

	rc = i3c_ccc_do_getmwl(&target_b_desc, &mwl_get);
	zassert_ok(rc, "GETMWL after SET failed (%d)", rc);
	zassert_equal(mwl_get.len, 64, "MWL mismatch: got %u", mwl_get.len);

	/* Restore MWL/MRL to 256 so subsequent xfer tests can send full-FIFO
	 * payloads without violating the target's reported limits.
	 */
	struct i3c_ccc_mwl mwl_restore = {.len = 256};
	struct i3c_ccc_mrl mrl_restore = {.len = 256, .ibi_len = 4};

	(void)i3c_ccc_do_setmwl(&target_b_desc, &mwl_restore);
	(void)i3c_ccc_do_setmrl(&target_b_desc, &mrl_restore);
}

ZTEST(i3c_loopback, test_ccc_enec_disec)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	struct i3c_ccc_events ev = {.events = I3C_CCC_EVT_INTR};
	int rc;

	rc = i3c_ccc_do_events_set(&target_b_desc, true, &ev);
	zassert_true(rc == 0 || rc == -ENODEV || rc == -ENXIO, "ENEC %d", rc);
	rc = i3c_ccc_do_events_set(&target_b_desc, false, &ev);
	zassert_true(rc == 0 || rc == -ENODEV || rc == -ENXIO, "DISEC %d", rc);

	rc = i3c_ccc_do_events_all_set(i3c_dev, true, &ev);
	zassert_true(rc == 0 || rc == -ENODEV, "ENEC all %d", rc);
	rc = i3c_ccc_do_events_all_set(i3c_dev, false, &ev);
	zassert_true(rc == 0 || rc == -ENODEV, "DISEC all %d", rc);
}

ZTEST(i3c_loopback, test_ccc_getstatus)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	union i3c_ccc_getstatus st = {0};
	int rc = i3c_ccc_do_getstatus_fmt1(&target_b_desc, &st);

	zassert_true(rc >= 0 || rc == -ENODEV || rc == -ENXIO, "GETSTATUS returned unexpected %d",
		     rc);
}

ZTEST(i3c_loopback, test_ccc_entas_levels)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	int rc;

	rc = i3c_ccc_do_entas0(&target_b_desc);
	zassert_true(rc == 0 || rc == -ENODEV || rc == -ENOSYS || rc == -ENXIO, "ENTAS0 %d", rc);
	rc = i3c_ccc_do_entas1(&target_b_desc);
	zassert_true(rc == 0 || rc == -ENODEV || rc == -ENOSYS || rc == -ENXIO, "ENTAS1 %d", rc);
	rc = i3c_ccc_do_entas2(&target_b_desc);
	zassert_true(rc == 0 || rc == -ENODEV || rc == -ENOSYS || rc == -ENXIO, "ENTAS2 %d", rc);
	rc = i3c_ccc_do_entas3(&target_b_desc);
	zassert_true(rc == 0 || rc == -ENODEV || rc == -ENOSYS || rc == -ENXIO, "ENTAS3 %d", rc);

	rc = i3c_ccc_do_entas0_all(i3c_dev);
	zassert_true(rc == 0 || rc == -ENODEV || rc == -ENOSYS, "ENTAS0 all %d", rc);
}

ZTEST(i3c_loopback, test_ccc_getmxds)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	/* Skip if target doesn't advertise BCR[0] (Max Data Speed Limit);
	 * the target HW correctly NACKs the directed CCC in that case.
	 */
	if ((target_b_desc.bcr & I3C_BCR_MAX_DATA_SPEED_LIMIT) == 0U) {
		ztest_test_skip();
		return;
	}

	union i3c_ccc_getmxds mxds = {0};
	int rc = i3c_ccc_do_getmxds_fmt1(&target_b_desc, &mxds);

	zassert_true(rc >= 0 || rc == -ENODEV || rc == -ENOSYS || rc == -ENXIO,
		     "GETMXDS returned unexpected %d", rc);
}

ZTEST(i3c_loopback, test_ccc_setnewda_round_trip)
{
	uint8_t saved = target_b_desc.dynamic_addr;
	struct i3c_ccc_address new_da = {.addr = 0x42U << 1};
	int rc;

	if (saved == 0) {
		ztest_test_skip();
		return;
	}

	rc = i3c_ccc_do_setnewda(&target_b_desc, new_da);
	zassert_true(rc == 0 || rc == -ENODEV || rc == -ENXIO, "SETNEWDA %d", rc);

	if (rc != 0) {
		/* SETNEWDA not supported or device absent — nothing to restore. */
		return;
	}

	/* Sync software descriptor AND hardware DAT to the new address
	 * before the restore CCC; otherwise the restore packet is sent
	 * to the old address and dynamic_addr stays stuck at 0x42.
	 */
	target_b_desc.dynamic_addr = 0x42U;
	(void)i3c_reattach_i3c_device(&target_b_desc, saved);

	/* Restore the original DA. */
	struct i3c_ccc_address restore = {.addr = (uint8_t)(saved << 1)};

	rc = i3c_ccc_do_setnewda(&target_b_desc, restore);
	if (rc == 0) {
		target_b_desc.dynamic_addr = saved;
		(void)i3c_reattach_i3c_device(&target_b_desc, 0x42U);
	} else {
		/*
		 * Restore failed — hard-reset the bus so subsequent tests see
		 * a live target at a valid dynamic address.
		 */
		(void)i3c_ccc_do_rstdaa_all(i3c_dev);
		target_b_desc.dynamic_addr = 0;
		struct i3c_ccc_address da = {.addr = TARGET_STATIC_ADDR};

		if (i3c_ccc_do_setdasa(&target_b_desc, da) == 0) {
			target_b_desc.dynamic_addr = TARGET_STATIC_ADDR;
		}
	}
}

ZTEST(i3c_loopback, test_ccc_setaasa_all)
{
	int rc;

	/*
	 * SETAASA assigns SA as DA only for targets with no DA, so RSTDAA
	 * first to clear all DAs.  Sending SETAASA to a target that
	 * already has a valid DA causes the DW target IP to glitch
	 * DYNAMIC_ADDR_VALID mid-rewrite — a directed CCC during that
	 * window NACKs.
	 */
	rc = i3c_ccc_do_rstdaa_all(i3c_dev);
	zassert_true(rc == 0 || rc == -ENODEV || rc == -ENOSYS, "pre-SETAASA RSTDAA returned %d",
		     rc);

	rc = i3c_ccc_do_setaasa_all(i3c_dev);
	zassert_true(rc == 0 || rc == -ENODEV || rc == -ENOSYS, "SETAASA returned unexpected %d",
		     rc);

	/* SETAASA assigned SA (TARGET_STATIC_ADDR) as the new DA; the
	 * controller-side DAT/addr_slots still hold the same value (broadcasts
	 * don't touch them), so no reattach is needed.
	 */
	if (rc == 0) {
		target_b_desc.dynamic_addr = TARGET_STATIC_ADDR;
	} else if (target_b_desc.dynamic_addr == 0) {
		/* SETAASA not supported — restore via SETDASA */
		struct i3c_ccc_address da = {.addr = TARGET_STATIC_ADDR};

		if (i3c_ccc_do_setdasa(&target_b_desc, da) == 0) {
			target_b_desc.dynamic_addr = TARGET_STATIC_ADDR;
		}
	}
}

ZTEST(i3c_loopback, test_ccc_rstdaa_all)
{
	int rc = i3c_ccc_do_rstdaa_all(i3c_dev);

	zassert_true(rc == 0 || rc == -ENODEV || rc == -ENOSYS, "RSTDAA all returned %d", rc);

	/* Re-enumerate target_b via SETDASA (same path as suite_setup)
	 * so subsequent tests have a valid DA.
	 */
	target_b_desc.dynamic_addr = 0;

	struct i3c_ccc_address da = {.addr = TARGET_STATIC_ADDR};

	rc = i3c_ccc_do_setdasa(&target_b_desc, da);
	if (rc == 0) {
		target_b_desc.dynamic_addr = TARGET_STATIC_ADDR;
	}
}
