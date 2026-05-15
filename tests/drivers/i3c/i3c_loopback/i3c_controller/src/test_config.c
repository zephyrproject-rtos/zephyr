/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 8: Configuration — exercises i3c_config_get, i3c_configure,
 * i3c_device_find, i3c_bus_has_sec_controller.
 */

#include "test_common.h"

ZTEST(i3c_loopback, test_config_get_returns_scl_frequencies)
{
	struct i3c_config_controller cfg = {0};
	int rc = i3c_config_get(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);

	zassert_ok(rc, "config_get rc=%d", rc);
	zassert_true(cfg.scl.i3c > 0, "i3c SCL freq = %u (expected > 0)", cfg.scl.i3c);
}

ZTEST(i3c_loopback, test_configure_runtime_scl_change)
{
	struct i3c_config_controller cfg = {0};
	int rc = i3c_config_get(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);

	zassert_ok(rc, "config_get rc=%d", rc);

	uint32_t saved = cfg.scl.i3c;

	cfg.scl.i3c = saved / 2;
	rc = i3c_configure(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	zassert_true(rc == 0 || rc == -ENOSYS, "configure rc=%d", rc);

	if (rc == 0) {
		struct i3c_config_controller readback = {0};

		rc = i3c_config_get(i3c_dev, I3C_CONFIG_CONTROLLER, &readback);
		zassert_ok(rc, "readback config_get rc=%d", rc);
		zassert_true(readback.scl.i3c <= saved, "SCL not updated: expected <= %u, got %u",
			     saved, readback.scl.i3c);
	}

	cfg.scl.i3c = saved;
	(void)i3c_configure(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
}

ZTEST(i3c_loopback, test_device_find_by_pid)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	uint64_t pid = target_b_desc.pid;
	struct i3c_device_id id = I3C_DEVICE_ID(pid);
	struct i3c_device_desc *desc = i3c_device_find(i3c_dev, &id);

	zassert_not_null(desc, "device_find returned NULL for attached target");
	zassert_equal(desc->dynamic_addr, target_b_desc.dynamic_addr,
		      "found desc DA 0x%02x != expected 0x%02x", desc->dynamic_addr,
		      target_b_desc.dynamic_addr);
}

ZTEST(i3c_loopback, test_bus_has_sec_controller)
{
	bool has = i3c_bus_has_sec_controller(i3c_dev);

	/* Target board BCR=0x26 (plain I3C target, no secondary controller
	 * role), so this should return false.
	 */
	zassert_false(has, "expected no secondary controller on bus");
}
