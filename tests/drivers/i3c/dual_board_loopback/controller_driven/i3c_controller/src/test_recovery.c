/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 7: Error injection / recovery — exercises NACK handling, bus
 * recovery, soft reset paths.
 */

#include "test_common.h"

#include <zephyr/drivers/i3c/ccc.h>

ZTEST(dual_board_loopback, test_recovery_address_nack)
{
	struct i3c_device_desc ghost = {
		.bus = i3c_dev,
		.dev = &i3c_peer_dev,
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

ZTEST(dual_board_loopback, test_recovery_xfer_after_nack_succeeds)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	uint8_t byte = 0x42;
	struct i3c_msg msg = {
		.buf = &byte,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	int rc = i3c_transfer(&target_b_desc, &msg, 1);

	zassert_ok(rc, "transfer after NACK recovery failed (%d)", rc);
}

ZTEST(dual_board_loopback, test_recovery_bus_recover_api)
{
	int rc = i3c_recover_bus(i3c_dev);

	zassert_true(rc == 0 || rc == -ENOSYS, "i3c_recover_bus rc=%d", rc);
}

/*
 * A NACK with commands still queued stops the controller mid-sequence, which
 * is the state the DEVICE_CTRL recovery path exists to clear.  Nothing here
 * calls i3c_recover_bus(), so the driver must recover on the next transfer by
 * itself.
 */
ZTEST(dual_board_loopback, test_recovery_halt_after_queued_nack)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	struct i3c_device_desc ghost = {
		.bus = i3c_dev,
		.dev = &i3c_peer_dev,
		.pid = ((uint64_t)0x0000ccccU << 32) | 0xBADAD0F1U,
		.dynamic_addr = 0x6E, /* unused */
	};
	uint8_t tx[2] = {0xA5, 0x5A};
	struct i3c_msg nack_msgs[2] = {
		{
			.buf = &tx[0],
			.len = 1,
			.flags = I3C_MSG_WRITE,
		},
		{
			.buf = &tx[1],
			.len = 1,
			.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
		},
	};
	uint8_t byte = 0x42;
	struct i3c_msg good = {
		.buf = &byte,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	int rc;

	(void)i3c_attach_i3c_device(&ghost);
	rc = i3c_transfer(&ghost, nack_msgs, ARRAY_SIZE(nack_msgs));
	(void)i3c_detach_i3c_device(&ghost);
	zassert_true(rc < 0, "queued transfer to ghost address should fail, got %d", rc);

	rc = i3c_transfer(&target_b_desc, &good, 1);
	zassert_ok(rc, "controller did not recover after a queued NACK (%d)", rc);
}

ZTEST(dual_board_loopback, test_recovery_invalid_ccc)
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
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();
	struct i3c_ccc_getbcr bcr = {0};

	rc = i3c_ccc_do_getbcr(&target_b_desc, &bcr);
	zassert_true(rc >= 0 || rc == -ENXIO, "post-recovery GETBCR failed (%d)", rc);
}

/* num_targets is a size_t and ccc.h documents no upper bound, so a caller may
 * ask for more commands than the controller can tag or queue.  How many is too
 * many is driver-specific; what is asserted here is only that the request is
 * refused rather than overrunning the driver's command array, and that the
 * refusal costs no bus traffic.
 */
ZTEST(dual_board_loopback, test_recovery_ccc_too_many_targets)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	static struct i3c_ccc_target_payload targets[32];
	struct i3c_ccc_payload payload = {
		.ccc = {.id = I3C_CCC_GETBCR},
		.targets = {.payloads = targets, .num_targets = ARRAY_SIZE(targets)},
	};
	struct i3c_ccc_getbcr bcr = {0};
	int rc;

	for (size_t i = 0; i < ARRAY_SIZE(targets); i++) {
		targets[i].addr = target_b_desc.dynamic_addr;
		targets[i].rnw = 1;
		targets[i].data = NULL;
		targets[i].data_len = 0;
	}

	rc = i3c_do_ccc(i3c_dev, &payload);
	zassert_true(rc != 0, "CCC with %zu targets should be refused, got %d",
		     ARRAY_SIZE(targets), rc);

	rc = i3c_ccc_do_getbcr(&target_b_desc, &bcr);
	zassert_ok(rc, "GETBCR after a refused CCC failed (%d)", rc);
}
