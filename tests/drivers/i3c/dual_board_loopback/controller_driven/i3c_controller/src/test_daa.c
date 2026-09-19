/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 1: Bus init / DAA — exercises i3c_bus_init, i3c_do_daa,
 * dw_i3c_init, dw_i3c_init_scl_timing.
 */

#include "test_common.h"

#include <zephyr/drivers/i3c/ccc.h>

#include "test_identity.h"

ZTEST(dual_board_loopback, test_bus_init_assigns_dynamic_address)
{
	zassert_true(device_is_ready(i3c_dev), "I3C controller %s not ready", i3c_dev->name);

	if (target_b_desc.dynamic_addr == 0) {
		(void)i3c_do_daa(i3c_dev);
	}

	zassert_true(target_b_desc.dynamic_addr != 0,
		     "DAA failed: target still has dynamic_addr=0");
	zassert_true(target_b_desc.dynamic_addr <= 0x7F, "dynamic addr 0x%02x out of valid range",
		     target_b_desc.dynamic_addr);
}

/*
 * ENTDAA with no preceding RSTDAA must leave an already-enumerated target on
 * the DA it is holding rather than allocating it a new one.
 */
ZTEST(dual_board_loopback, test_entdaa_only_no_rstdaa)
{
	uint32_t tgt_before = 0;
	uint32_t tgt_after = 0;
	uint8_t da_before;
	int before_rc;
	int after_rc;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	da_before = target_b_desc.dynamic_addr;
	before_rc = target_device_addr_read(&tgt_before);

	rc = i3c_do_daa(i3c_dev);
	k_msleep(TGT_CCC_SETTLE_MS);
	after_rc = target_device_addr_read(&tgt_after);

	zassert_ok(before_rc, "DUMP before ENTDAA failed (%d)", before_rc);
	zassert_ok(after_rc, "DUMP after ENTDAA failed (%d)", after_rc);
	zassert_not_equal(tgt_after & TGT_DEVICE_ADDR_DA_VALID, 0U,
			  "ENTDAA left the target with no valid DA (DEVICE_ADDR=0x%08x)",
			  tgt_after);
	zassert_equal(FIELD_GET(TGT_DEVICE_ADDR_DA_MASK, tgt_after), da_before,
		      "ENTDAA moved the target from DA 0x%02x to 0x%02x (DEVICE_ADDR "
		      "0x%08x -> 0x%08x)",
		      da_before, (uint8_t)FIELD_GET(TGT_DEVICE_ADDR_DA_MASK, tgt_after),
		      tgt_before, tgt_after);
	zassert_equal(target_b_desc.dynamic_addr, da_before,
		      "ENTDAA moved the descriptor from DA 0x%02x to 0x%02x", da_before,
		      target_b_desc.dynamic_addr);
	zassert_ok(rc, "ENTDAA-only returned %d", rc);
}

/*
 * Stress reproducer for the post-ENTDAA xfer path.  After RSTDAA +
 * ENTDAA the freshly re-enumerated target may NACK its DA on the
 * first private xfer while it re-arms (target IP quirk); the inner
 * I3C_NACK_RETRIES loop is the correct application-level handling.
 */
ZTEST(dual_board_loopback, test_rstdaa_entdaa_xfer_stress)
{
	int rc;
	uint8_t probe = 0;
	struct i3c_msg msg = {
		.buf = &probe,
		.len = 1,
		.flags = I3C_MSG_READ | I3C_MSG_STOP,
	};

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	for (int iter = 0; iter < 200; iter++) {
		rc = i3c_ccc_do_rstdaa_all(i3c_dev);
		zassert_true(rc == 0 || rc == -ENODEV || rc == -ENOSYS, "iter %d RSTDAA: %d", iter,
			     rc);

		/* Allow target time to re-arm before ENTDAA (target IP quirk). */
		k_msleep(10);

		rc = i3c_do_daa(i3c_dev);
		zassert_true(rc == 0 || rc == -ENODEV, "iter %d ENTDAA: %d", iter, rc);

		probe = 0;
		(void)dual_board_loopback_stage_read("BB", 1);
		rc = -1;
		for (int a = 0; a < I3C_NACK_RETRIES; a++) {
			rc = i3c_transfer(&target_b_desc, &msg, 1);
			if (rc != -ENXIO) {
				break;
			}
			k_busy_wait(I3C_NACK_BACKOFF_US);
		}
		zassert_ok(rc, "iter %d post-ENTDAA xfer failed (%d)", iter, rc);
		zassert_equal(probe, 0x42, "iter %d probe got 0x%02x, want 0x42", iter, probe);
	}
}

/*
 * A device declared in devicetree but not populated on the board is a
 * supported configuration: its SETDASA is NACKed, and it must be left with
 * dynamic_addr == 0 rather than failing bus initialisation. Drivers such as
 * i3c_dw and i3c_npcx return i3c_bus_init() straight out of their init
 * function, so failing here would take the whole controller down with it.
 */
ZTEST(dual_board_loopback, test_bus_init_tolerates_absent_device)
{
	/* Free address that no device on this bench responds to. */
	struct i3c_device_desc absent = {
		.bus = i3c_dev,
		.dev = &i3c_peer_dev,
		.static_addr = 0x4AU,
	};
	struct i3c_dev_list absent_list = {
		.i3c = &absent,
		.num_i3c = 1,
	};
	struct i3c_dev_list real_list = {
		.i3c = &target_b_desc,
		.num_i3c = 1,
	};
	int rc;
	int restore_rc;

	rc = i3c_bus_init(i3c_dev, &absent_list);

	/* Re-enumerate before asserting: the call above issues RSTDAA, so
	 * leaving early would strand every later test without a target.
	 */
	restore_rc = i3c_bus_init(i3c_dev, &real_list);

	zassert_ok(rc, "i3c_bus_init failed (%d) because a declared device at 0x%02x is absent",
		   rc, absent.static_addr);
	zassert_equal(absent.dynamic_addr, 0U,
		      "absent device was given dynamic address 0x%02x", absent.dynamic_addr);

	zassert_ok(restore_rc, "failed to restore bus after absent-device test (%d)", restore_rc);
	zassert_not_equal(target_b_desc.dynamic_addr, 0U,
			  "target_b left without a dynamic address after restore");
}

ZTEST(dual_board_loopback, test_bcr_dcr_pid_match_overlay)
{
	struct i3c_ccc_getbcr bcr = {0};
	struct i3c_ccc_getdcr dcr = {0};
	struct i3c_ccc_getpid pid = {0};
	int rc;

	/* Direct CCCs require a valid dynamic address. */
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	rc = i3c_ccc_do_getbcr(&target_b_desc, &bcr);
	zassert_ok(rc, "GETBCR failed (%d)", rc);
	zassert_equal(bcr.bcr, TARGET_BCR, "BCR mismatch: got 0x%02x, expected 0x%02x", bcr.bcr,
		      TARGET_BCR);

	rc = i3c_ccc_do_getdcr(&target_b_desc, &dcr);
	zassert_ok(rc, "GETDCR failed (%d)", rc);
	zassert_equal(dcr.dcr, TARGET_DCR, "DCR mismatch: got 0x%02x, expected 0x%02x", dcr.dcr,
		      TARGET_DCR);

	rc = i3c_ccc_do_getpid(&target_b_desc, &pid);
	zassert_ok(rc, "GETPID failed (%d)", rc);
}

/*
 * ENTDAA-only stress.  Tight loop of RSTDAA + ENTDAA with no
 * private transfers in between, isolating the DA-assignment handshake
 * failure rate from any cascade caused by post-DAA private xfers.
 * Reports counts via printk; does not assert per iteration so a small
 * early failure does not hide a low base rate.
 */
ZTEST(dual_board_loopback, test_entdaa_only_stress)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	const int iters = 100;
	int rstdaa_fails = 0;
	int entdaa_fails = 0;
	int da_zero_after = 0;
	int probes = 0;

	for (int i = 0; i < iters; i++) {
		int rc = i3c_ccc_do_rstdaa_all(i3c_dev);

		if (rc != 0 && rc != -ENODEV && rc != -ENOSYS) {
			rstdaa_fails++;
		}
		k_msleep(10);

		rc = i3c_do_daa(i3c_dev);
		if (rc != 0 && rc != -ENODEV) {
			entdaa_fails++;
		}
		if (target_b_desc.dynamic_addr == 0U) {
			da_zero_after++;

			/* The controller believes the DA is gone; ask the target
			 * what DEVICE_ADDR it still holds.  Capped at 3 probes -
			 * extra sync-UART traffic on the controller's critical
			 * path is known to shift the failure rate.
			 */
			if (probes < 3) {
				char line[192];

				probes++;
				sync_drain(sync_uart);
				sync_send(sync_uart, "DUMP");
				if (sync_expect_line(sync_uart, "DUMP", line, sizeof(line),
						     K_MSEC(500)) == 0) {
					printk("[daa-stress] iter=%d ctrl_da=0 tgt %s\n", i, line);
				} else {
					printk("[daa-stress] iter=%d ctrl_da=0 tgt DUMP timeout\n",
					       i);
				}
			}

			/* Recover so the next iteration can run. */
			(void)i3c_recover_bus(i3c_dev);
		}
	}
	printk("[daa-stress] iters=%d rstdaa_fail=%d entdaa_fail=%d da_zero=%d\n", iters,
	       rstdaa_fails, entdaa_fails, da_zero_after);
}
