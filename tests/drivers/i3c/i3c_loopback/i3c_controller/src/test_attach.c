/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 4: Attach / detach / reattach.  Exercises
 *   dw_i3c_attach_device, dw_i3c_detach_device, dw_i3c_reattach_device,
 *   i3c_addr_slots tracking.
 */

#include "test_common.h"
#include <string.h>

ZTEST(i3c_loopback, test_attach_unknown_device)
{
	struct i3c_device_desc dyn = {
		.bus = i3c_dev,
		.pid = ((uint64_t)0x0000aaaaU << 32) | 0x12345678U,
	};
	int rc;

	rc = i3c_attach_i3c_device(&dyn);
	zassert_ok(rc, "attach unknown device returned %d", rc);

	(void)i3c_detach_i3c_device(&dyn);
}

ZTEST(i3c_loopback, test_detach_returns_enodev_on_xfer)
{
	uint8_t buf = 0x5A;
	struct i3c_msg msg = {
		.buf = &buf,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	int rc;

	rc = i3c_detach_i3c_device(&target_b_desc);
	zassert_ok(rc, "detach returned %d", rc);

	rc = i3c_transfer(&target_b_desc, &msg, 1);
	zassert_true(rc < 0, "expected error after detach, got %d", rc);

	rc = i3c_attach_i3c_device(&target_b_desc);
	zassert_ok(rc, "re-attach returned %d", rc);
}

ZTEST(i3c_loopback, test_reattach_with_new_da)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)i3c_dev->data;
	uint8_t old_da = target_b_desc.dynamic_addr;
	uint8_t new_da;
	int rc;

	if (old_da == 0U) {
		ztest_test_skip();
		return;
	}

	/* Pick a free DA distinct from the current one to avoid
	 * -EADDRNOTAVAIL.
	 */
	new_da = i3c_addr_slots_next_free_find(&data->attached_dev.addr_slots,
					       (uint8_t)(old_da + 1U));
	zassert_not_equal(new_da, 0U, "no free DA available for reattach");

	target_b_desc.dynamic_addr = new_da;
	rc = i3c_reattach_i3c_device(&target_b_desc, old_da);
	zassert_true(rc == 0 || rc == -ENOSYS,
		     "reattach old=0x%02x new=0x%02x returned unexpected %d", old_da, new_da, rc);

	/* Restore the original DA in controller descriptor + addr_slots
	 * so later tests still address target_b correctly.
	 */
	target_b_desc.dynamic_addr = old_da;
	(void)i3c_reattach_i3c_device(&target_b_desc, new_da);
}

ZTEST(i3c_loopback, test_attach_multiple_descriptors)
{
	struct i3c_device_desc descs[4] = {0};
	int attached = 0;
	int rc;

	for (int i = 0; i < (int)ARRAY_SIZE(descs); i++) {
		descs[i].bus = i3c_dev;
		descs[i].pid = ((uint64_t)0x0000bbbbU << 32) | (uint32_t)(0x100U + i);

		rc = i3c_attach_i3c_device(&descs[i]);
		if (rc != 0) {
			break;
		}
		attached++;
	}

	zassert_true(attached >= 1, "expected to attach at least 1, got %d", attached);

	for (int i = 0; i < attached; i++) {
		(void)i3c_detach_i3c_device(&descs[i]);
	}
}
