/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/debugpoint_internal.h>
#include <zephyr/debug/watchpoint.h>
#include <zephyr/devicetree.h>
#include <zephyr/ztest.h>

#include <errno.h>

BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP) || CONFIG_MP_MAX_NUM_CPUS == 2);

#if defined(CONFIG_SMP) && DT_PROP_OR(DT_PATH(cpus, cpu_1), zephyr_deferred_start, 0)
#define EXPECTED_ADD_RESULT (-ENOTSUP)
#else
#define EXPECTED_ADD_RESULT 0
#endif

static uint8_t watched;

static void watchpoint_callback(const struct k_watchpoint *wp,
				const struct k_watchpoint_event *event, void *arg)
{
	ARG_UNUSED(wp);
	ARG_UNUSED(event);
	ARG_UNUSED(arg);
}

static void debugpoint_callback(const struct z_debugpoint_config *config,
				const struct z_debugpoint_event *event, void *arg)
{
	ARG_UNUSED(config);
	ARG_UNUSED(event);
	ARG_UNUSED(arg);
}

ZTEST(watchpoint_cpu_startup, test_public_add)
{
	struct k_watchpoint wp = K_WATCHPOINT_INITIALIZER(
		&watched, sizeof(watched), K_WATCHPOINT_WRITE, watchpoint_callback, NULL);

	for (int attempt = 0; attempt < 2; attempt++) {
		zassert_equal(k_watchpoint_add(&wp), EXPECTED_ADD_RESULT);
		if (EXPECTED_ADD_RESULT != 0) {
			zassert_false(k_watchpoint_is_active(&wp));
		}
		zassert_ok(k_watchpoint_remove(&wp));
		zassert_false(k_watchpoint_is_active(&wp));
	}
}

ZTEST(watchpoint_cpu_startup, test_core_add)
{
	const struct z_debugpoint_config config = {
		.type = Z_DEBUGPOINT_WATCH_WRITE,
		.addr = &watched,
		.size = sizeof(watched),
		.callback = debugpoint_callback,
	};

	for (int attempt = 0; attempt < 2; attempt++) {
		z_debugpoint_handle_t handle = Z_DEBUGPOINT_HANDLE_INVALID;

		zassert_equal(z_debugpoint_add(&config, &handle), EXPECTED_ADD_RESULT);
		if (EXPECTED_ADD_RESULT != 0) {
			zassert_equal(handle, Z_DEBUGPOINT_HANDLE_INVALID);
		} else {
			zassert_not_equal(handle, Z_DEBUGPOINT_HANDLE_INVALID);
		}
		zassert_ok(z_debugpoint_remove(handle));
	}
}

ZTEST_SUITE(watchpoint_cpu_startup, NULL, NULL, NULL, NULL, NULL);
