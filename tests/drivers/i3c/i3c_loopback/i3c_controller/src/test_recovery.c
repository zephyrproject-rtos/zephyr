/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 7: Error injection / recovery — exercises NACK handling, bus
 * recovery, soft reset paths.
 */

#include "test_common.h"

#include <zephyr/drivers/i3c/ccc.h>

ZTEST(i3c_loopback, test_recovery_address_nack)
{
	struct i3c_device_desc ghost = {
		.bus = i3c_dev,
		.pid = ((uint64_t)0x0000ccccU << 32) | 0xBADAD0F0U,
		.dynamic_addr = 0x6E, /* unused */
	};
	uint8_t byte = 0;
	struct i3c_msg msg = {
		.buf = &byte,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};

	(void)i3c_attach_i3c_device(&ghost);
	int rc = i3c_transfer(&ghost, &msg, 1);

	zassert_true(rc < 0, "transfer to ghost address should NACK, got %d", rc);
	(void)i3c_detach_i3c_device(&ghost);
}

ZTEST(i3c_loopback, test_recovery_xfer_after_nack_succeeds)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	uint8_t byte = 0x42;
	struct i3c_msg msg = {
		.buf = &byte,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	int rc = i3c_transfer(&target_b_desc, &msg, 1);

	zassert_ok(rc, "transfer after NACK recovery failed (%d)", rc);
}

ZTEST(i3c_loopback, test_recovery_bus_recover_api)
{
	int rc = i3c_recover_bus(i3c_dev);

	zassert_true(rc == 0 || rc == -ENOSYS, "i3c_recover_bus rc=%d", rc);
}

ZTEST(i3c_loopback, test_recovery_invalid_ccc)
{
	/* Direct CCC with NULL targets.payloads must be rejected with
	 * -EINVAL before reaching hardware.
	 */
	struct i3c_ccc_payload bad = {
		.ccc = {.id = I3C_CCC_GETBCR},
		.targets = {.payloads = NULL, .num_targets = 1},
	};
	int rc = i3c_do_ccc(i3c_dev, &bad);

	zassert_true(rc < 0, "expected error on direct CCC w/ NULL targets, got %d", rc);

	/* Sanity: bus still usable. */
	I3C_LOOPBACK_REQUIRE_TARGET_DA();
	struct i3c_ccc_getbcr bcr = {0};

	rc = i3c_ccc_do_getbcr(&target_b_desc, &bcr);
	zassert_true(rc >= 0 || rc == -ENXIO, "post-recovery GETBCR failed (%d)", rc);
}
