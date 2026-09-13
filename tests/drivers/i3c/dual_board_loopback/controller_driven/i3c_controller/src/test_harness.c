/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Meta tests: these exercise the loopback fixture itself (main.c) rather
 * than the I3C driver. They guard the recovery paths the rest of the suite
 * depends on, so a single bus fault cannot cascade into every later test.
 *
 * These issues presented as instability - e.g. 1 in every 25 runs would
 * spontaneously fail horribly.  These tests safeguard the suite itself -
 * if the harness is updated and these fail, the instability may have been
 * reintroduced.
 */

#include "test_common.h"

#include <zephyr/drivers/i3c/ccc.h>

#include "test_identity.h"

/*
 * A NACKed SETDASA makes i3c_bus_setdasa() detach the descriptor, which
 * clears the controller's DAT entry. Re-assigning dynamic_addr alone does
 * not undo that, so every later transfer returns -EINVAL and one NACK
 * cascades into the whole suite failing.
 */
ZTEST(dual_board_loopback, meta_heal_recovers_detached_descriptor)
{
	uint8_t probe = 0x00U;
	struct i3c_msg msg = {
		.buf = &probe,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	/* Reproduce exactly what a failed SETDASA leaves behind. */
	rc = i3c_detach_i3c_device(&target_b_desc);
	zassert_ok(rc, "failed to detach target_b for the test (%d)", rc);

	/* Faking the address back is not enough: the DAT entry is gone. */
	target_b_desc.dynamic_addr = TARGET_STATIC_ADDR;
	zassert_not_equal(i3c_transfer(&target_b_desc, &msg, 1), 0,
			  "transfer unexpectedly succeeded on a detached descriptor");

	rc = dual_board_loopback_heal_target();
	zassert_ok(rc, "heal failed to recover a detached descriptor (%d)", rc);
	zassert_not_equal(target_b_desc.dynamic_addr, 0U, "heal left target_b without a DA");

	rc = i3c_transfer_retry(&target_b_desc, &msg, 1);
	zassert_ok(rc, "target unusable after heal (%d)", rc);
}

/*
 * i3c_ccc_do_setdasa() rejects a descriptor that still holds a dynamic
 * address, so a heal that skips clearing it silently returns -EINVAL and
 * recovers nothing. Guard that ordering.
 */
ZTEST(dual_board_loopback, meta_heal_recovers_stale_dynamic_address)
{
	uint8_t probe = 0x00U;
	struct i3c_msg msg = {
		.buf = &probe,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	/* Attached, but pointing at an address the target no longer answers. */
	target_b_desc.dynamic_addr = TARGET_STATIC_ADDR + 1U;

	rc = dual_board_loopback_heal_target();
	zassert_ok(rc, "heal failed against a stale dynamic address (%d)", rc);
	zassert_equal(target_b_desc.dynamic_addr, TARGET_STATIC_ADDR,
		      "heal left target_b at 0x%02x, expected 0x%02x", target_b_desc.dynamic_addr,
		      TARGET_STATIC_ADDR);

	rc = i3c_transfer_retry(&target_b_desc, &msg, 1);
	zassert_ok(rc, "target unusable after heal (%d)", rc);
}
