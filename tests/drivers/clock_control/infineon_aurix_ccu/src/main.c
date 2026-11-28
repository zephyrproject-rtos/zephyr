/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/infineon-aurix-ccu.h>
#include <zephyr/sys/clock.h>
#include <zephyr/ztest.h>

#define CCU_NODE  DT_COMPAT_GET_ANY_STATUS_OKAY(infineon_aurix_ccu)
#define USER_NODE DT_PATH(zephyr_user)

static const struct device *const ccu = DEVICE_DT_GET(CCU_NODE);

static clock_control_subsys_t subsys(uint32_t id)
{
	return (clock_control_subsys_t)(uintptr_t)id;
}

static uint32_t rate_of(uint32_t id)
{
	uint32_t rate = 0;

	zassert_ok(clock_control_get_rate(ccu, subsys(id), &rate), "get_rate(%u)", id);

	return rate;
}

ZTEST(aurix_ccu, test_rates)
{
	zassert_equal(rate_of(AURIX_CCU_SRI), DT_PROP(USER_NODE, sri_hz));
	zassert_equal(rate_of(AURIX_CCU_SPB), DT_PROP(USER_NODE, spb_hz));
	zassert_equal(rate_of(AURIX_CCU_FSI), DT_PROP(USER_NODE, fsi_hz));
	zassert_equal(rate_of(AURIX_CCU_STM), DT_PROP(USER_NODE, stm_hz));
	zassert_equal(rate_of(AURIX_CCU_ASCLINF), DT_PROP(USER_NODE, asclinf_hz));
}

ZTEST(aurix_ccu, test_stm_is_kernel_clock)
{
	zassert_equal(rate_of(AURIX_CCU_STM), sys_clock_hw_cycles_per_sec());
}

ZTEST(aurix_ccu, test_on_status_off)
{
	for (uint32_t id = AURIX_CCU_SRI; id <= AURIX_CCU_ASCLINF; id++) {
		zassert_ok(clock_control_on(ccu, subsys(id)));
		zassert_equal(clock_control_get_status(ccu, subsys(id)), CLOCK_CONTROL_STATUS_ON);
		zassert_equal(clock_control_off(ccu, subsys(id)), -ENOTSUP);
	}
}

ZTEST(aurix_ccu, test_invalid_id)
{
	uint32_t rate;

	zassert_equal(clock_control_get_rate(ccu, subsys(100), &rate), -EINVAL);
	zassert_equal(clock_control_on(ccu, subsys(100)), -EINVAL);
	zassert_equal(clock_control_get_status(ccu, subsys(100)), CLOCK_CONTROL_STATUS_UNKNOWN);
}

static void *setup(void)
{
	zassert_true(device_is_ready(ccu));

	return NULL;
}

ZTEST_SUITE(aurix_ccu, NULL, setup, NULL, NULL, NULL);
