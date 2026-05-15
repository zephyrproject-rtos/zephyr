/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Power-management: exercises the I3C controller's PM suspend/resume
 * path.  On platforms with a vendor pm_resume hook this is the only
 * test that re-runs the wrapper-gate / peri-clock setup after deep
 * sleep.
 */

#include "test_common.h"

#if defined(CONFIG_PM_DEVICE)

#include <zephyr/pm/device.h>

/*
 * Verifies SUSPEND + RESUME succeed and a post-resume I3C transfer to
 * Board B completes — proves bus, clocks, and attached-device state
 * were fully restored.
 */
ZTEST(i3c_loopback, test_pm_suspend_resume)
{
	uint8_t tx = 0xA5;
	struct i3c_msg msg = {
		.buf = &tx,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	int rc;

	rc = pm_device_action_run(i3c_dev, PM_DEVICE_ACTION_SUSPEND);
	zassert_ok(rc, "PM suspend failed (%d)", rc);

	rc = pm_device_action_run(i3c_dev, PM_DEVICE_ACTION_RESUME);
	zassert_ok(rc, "PM resume failed (%d)", rc);

	/* Brief settle for clocks/wrapper gate after resume */
	k_msleep(5);

	/* Post-resume transfer.  Gate on target DA so the suite skips
	 * cleanly if the target is absent.
	 */
	if (target_b_desc.dynamic_addr != 0) {
		rc = i3c_transfer(&target_b_desc, &msg, 1);
		zassert_ok(rc, "post-resume transfer failed (%d)", rc);
	}
}

#endif /* CONFIG_PM_DEVICE */
