/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc. (AMD)
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/ztest.h>

#if !DT_HAS_COMPAT_STATUS_OKAY(xlnx_clkx5_wiz_1_0)
#error "No enabled xlnx,clkx5-wiz-1.0 node found in devicetree"
#endif

#define CLK_WIZ_NODE   DT_COMPAT_GET_ANY_STATUS_OKAY(xlnx_clkx5_wiz_1_0)
#define CLK_WIZ_DEV    DEVICE_DT_GET(CLK_WIZ_NODE)

#define NUM_OUT_CLKS   DT_PROP(CLK_WIZ_NODE, xlnx_num_out_clks)

#define CLK_ID_FIRST   0U
#define CLK_ID_LAST    (NUM_OUT_CLKS - 1U)
#define CLK_ID_INVALID NUM_OUT_CLKS

#define RATE_100MHZ    100000000U
#define RATE_200MHZ    200000000U
#define RATE_300MHZ    300000000U

#define RATE_TOL_PCT   5U

#define CLK_SYS(id)  ((clock_control_subsys_t)(uintptr_t)(id))

ZTEST(xlnx_clk_wiz_device, test_device_is_ready)
{
	const struct device *dev = CLK_WIZ_DEV;

	zassert_not_null(dev, "clock_control device pointer is NULL");
	zassert_true(device_is_ready(dev),
		     "clock_control device is not ready");
}

ZTEST_SUITE(xlnx_clk_wiz_device, NULL, NULL, NULL, NULL, NULL);

ZTEST(xlnx_clk_wiz_on_off, test_on_clk0)
{
	const struct device *dev = CLK_WIZ_DEV;
	int ret;

	ret = clock_control_on(dev, CLK_SYS(CLK_ID_FIRST));
	zassert_equal(ret, 0,
		      "clock_control_on(clk0) failed: %d", ret);
}

ZTEST(xlnx_clk_wiz_on_off, test_on_clk_last)
{
	const struct device *dev = CLK_WIZ_DEV;
	int ret;

	ret = clock_control_on(dev, CLK_SYS(CLK_ID_LAST));
	zassert_equal(ret, 0,
		      "clock_control_on(clk_last=%u) failed: %d",
		      CLK_ID_LAST, ret);
}

ZTEST(xlnx_clk_wiz_on_off, test_on_invalid_clk_id)
{
	const struct device *dev = CLK_WIZ_DEV;
	int ret;

	ret = clock_control_on(dev, CLK_SYS(CLK_ID_INVALID));
	zassert_equal(ret, -EINVAL,
		      "clock_control_on(invalid clk_id=%u) should return "
		      "-EINVAL, got %d", CLK_ID_INVALID, ret);
}

ZTEST(xlnx_clk_wiz_on_off, test_off_clk0)
{
	const struct device *dev = CLK_WIZ_DEV;
	int ret;

	ret = clock_control_off(dev, CLK_SYS(CLK_ID_FIRST));
	zassert_equal(ret, 0,
		      "clock_control_off(clk0) failed: %d", ret);
}

ZTEST(xlnx_clk_wiz_on_off, test_off_clk_last)
{
	const struct device *dev = CLK_WIZ_DEV;
	int ret;

	ret = clock_control_off(dev, CLK_SYS(CLK_ID_LAST));
	zassert_equal(ret, 0,
		      "clock_control_off(clk_last=%u) failed: %d",
		      CLK_ID_LAST, ret);
}

ZTEST(xlnx_clk_wiz_on_off, test_off_invalid_clk_id)
{
	const struct device *dev = CLK_WIZ_DEV;
	int ret;

	ret = clock_control_off(dev, CLK_SYS(CLK_ID_INVALID));
	zassert_equal(ret, -EINVAL,
		      "clock_control_off(invalid clk_id=%u) should return "
		      "-EINVAL, got %d", CLK_ID_INVALID, ret);
}

ZTEST(xlnx_clk_wiz_on_off, test_on_off_toggle_clk0)
{
	const struct device *dev = CLK_WIZ_DEV;
	int ret;

	ret = clock_control_on(dev, CLK_SYS(CLK_ID_FIRST));
	zassert_equal(ret, 0, "on step 1 failed: %d", ret);

	ret = clock_control_off(dev, CLK_SYS(CLK_ID_FIRST));
	zassert_equal(ret, 0, "off step failed: %d", ret);

	ret = clock_control_on(dev, CLK_SYS(CLK_ID_FIRST));
	zassert_equal(ret, 0, "on step 2 failed: %d", ret);
}

ZTEST(xlnx_clk_wiz_on_off, test_on_idempotent)
{
	const struct device *dev = CLK_WIZ_DEV;
	int ret;

	ret = clock_control_on(dev, CLK_SYS(CLK_ID_FIRST));
	zassert_equal(ret, 0, "first on failed: %d", ret);

	ret = clock_control_on(dev, CLK_SYS(CLK_ID_FIRST));
	zassert_equal(ret, 0, "second on (idempotent) failed: %d", ret);
}

ZTEST_SUITE(xlnx_clk_wiz_on_off, NULL, NULL, NULL, NULL, NULL);

ZTEST(xlnx_clk_wiz_get_rate, test_get_rate_clk0_nonzero)
{
	const struct device *dev = CLK_WIZ_DEV;
	uint32_t rate = 0U;
	int ret;

	ret = clock_control_get_rate(dev, CLK_SYS(CLK_ID_FIRST), &rate);
	zassert_equal(ret, 0,
		      "clock_control_get_rate(clk0) failed: %d", ret);
	zassert_not_equal(rate, 0U,
			  "clock_control_get_rate(clk0) returned 0 Hz");
}

ZTEST(xlnx_clk_wiz_get_rate, test_get_rate_all_valid_clks)
{
	const struct device *dev = CLK_WIZ_DEV;
	uint32_t rate;
	int ret;

	for (uint32_t id = 0U; id < NUM_OUT_CLKS; id++) {
		rate = 0U;
		ret = clock_control_get_rate(dev, CLK_SYS(id), &rate);
		zassert_equal(ret, 0,
			      "clock_control_get_rate(clk%u) failed: %d",
			      id, ret);
		zassert_not_equal(rate, 0U,
				  "clock_control_get_rate(clk%u) returned 0 Hz",
				  id);
	}
}

ZTEST(xlnx_clk_wiz_get_rate, test_get_rate_invalid_clk_id)
{
	const struct device *dev = CLK_WIZ_DEV;
	uint32_t rate = 0U;
	int ret;

	ret = clock_control_get_rate(dev, CLK_SYS(CLK_ID_INVALID), &rate);
	zassert_equal(ret, -EINVAL,
		      "clock_control_get_rate(invalid id=%u) should return "
		      "-EINVAL, got %d", CLK_ID_INVALID, ret);
}

ZTEST_SUITE(xlnx_clk_wiz_get_rate, NULL, NULL, NULL, NULL, NULL);

ZTEST(xlnx_clk_wiz_set_rate, test_set_rate_100mhz_clk0)
{
	const struct device *dev = CLK_WIZ_DEV;
	uint32_t target = RATE_100MHZ;
	int ret;

	ret = clock_control_set_rate(dev, CLK_SYS(CLK_ID_FIRST),
				     (clock_control_subsys_rate_t)&target);
	zassert_equal(ret, 0,
		      "clock_control_set_rate(clk0, 100 MHz) failed: %d", ret);
}

ZTEST(xlnx_clk_wiz_set_rate, test_set_rate_200mhz_clk0)
{
	const struct device *dev = CLK_WIZ_DEV;
	uint32_t target = RATE_200MHZ;
	int ret;

	ret = clock_control_set_rate(dev, CLK_SYS(CLK_ID_FIRST),
				     (clock_control_subsys_rate_t)&target);
	zassert_equal(ret, 0,
		      "clock_control_set_rate(clk0, 200 MHz) failed: %d", ret);
}

ZTEST(xlnx_clk_wiz_set_rate, test_set_rate_300mhz_clk0)
{
	const struct device *dev = CLK_WIZ_DEV;
	uint32_t target = RATE_300MHZ;
	int ret;

	ret = clock_control_set_rate(dev, CLK_SYS(CLK_ID_FIRST),
				     (clock_control_subsys_rate_t)&target);
	zassert_equal(ret, 0,
		      "clock_control_set_rate(clk0, 300 MHz) failed: %d", ret);
}

ZTEST(xlnx_clk_wiz_set_rate, test_set_rate_zero)
{
	const struct device *dev = CLK_WIZ_DEV;
	uint32_t target = 0U;
	int ret;

	ret = clock_control_set_rate(dev, CLK_SYS(CLK_ID_FIRST),
				     (clock_control_subsys_rate_t)&target);
	zassert_equal(ret, -EINVAL,
		      "clock_control_set_rate(0 Hz) should return -EINVAL, "
		      "got %d", ret);
}

ZTEST(xlnx_clk_wiz_set_rate, test_set_rate_null_ptr)
{
	const struct device *dev = CLK_WIZ_DEV;
	int ret;

	ret = clock_control_set_rate(dev, CLK_SYS(CLK_ID_FIRST), NULL);
	zassert_equal(ret, -EINVAL,
		      "clock_control_set_rate(NULL) should return -EINVAL, "
		      "got %d", ret);
}

ZTEST(xlnx_clk_wiz_set_rate, test_set_rate_invalid_clk_id)
{
	const struct device *dev = CLK_WIZ_DEV;
	uint32_t target = RATE_100MHZ;
	int ret;

	ret = clock_control_set_rate(dev, CLK_SYS(CLK_ID_INVALID),
				     (clock_control_subsys_rate_t)&target);
	zassert_equal(ret, -EINVAL,
		      "clock_control_set_rate(invalid id=%u) should return "
		      "-EINVAL, got %d", CLK_ID_INVALID, ret);
}

ZTEST_SUITE(xlnx_clk_wiz_set_rate, NULL, NULL, NULL, NULL, NULL);

static void roundtrip_check(uint32_t clk_id, uint32_t target_hz)
{
	const struct device *dev = CLK_WIZ_DEV;
	uint32_t actual = 0U;
	uint32_t tol;
	int ret;

	ret = clock_control_set_rate(dev, CLK_SYS(clk_id),
				     (clock_control_subsys_rate_t)&target_hz);
	zassert_equal(ret, 0,
		      "set_rate(clk%u, %u Hz) failed: %d",
		      clk_id, target_hz, ret);

	ret = clock_control_get_rate(dev, CLK_SYS(clk_id), &actual);
	zassert_equal(ret, 0,
		      "get_rate(clk%u) after set_rate failed: %d",
		      clk_id, ret);

	tol = (uint32_t)((uint64_t)target_hz * RATE_TOL_PCT / 100U);
	zassert_within(actual, target_hz, tol,
		       "clk%u: get_rate=%u Hz is more than %u%% away from "
		       "target=%u Hz (tol=%u Hz)",
		       clk_id, actual, RATE_TOL_PCT, target_hz, tol);
}

ZTEST(xlnx_clk_wiz_roundtrip, test_roundtrip_100mhz_clk0)
{
	roundtrip_check(CLK_ID_FIRST, RATE_100MHZ);
}

ZTEST(xlnx_clk_wiz_roundtrip, test_roundtrip_200mhz_clk0)
{
	roundtrip_check(CLK_ID_FIRST, RATE_200MHZ);
}

ZTEST(xlnx_clk_wiz_roundtrip, test_roundtrip_300mhz_clk0)
{
	roundtrip_check(CLK_ID_FIRST, RATE_300MHZ);
}

ZTEST(xlnx_clk_wiz_roundtrip, test_get_rate_nonzero_after_set)
{
	const struct device *dev = CLK_WIZ_DEV;
	uint32_t target = RATE_200MHZ;
	uint32_t actual = 0U;
	int ret;

	ret = clock_control_set_rate(dev, CLK_SYS(CLK_ID_FIRST),
				     (clock_control_subsys_rate_t)&target);
	zassert_equal(ret, 0, "set_rate failed: %d", ret);

	ret = clock_control_get_rate(dev, CLK_SYS(CLK_ID_FIRST), &actual);
	zassert_equal(ret, 0, "get_rate failed: %d", ret);
	zassert_not_equal(actual, 0U, "get_rate returned 0 Hz after set_rate");
}

ZTEST(xlnx_clk_wiz_roundtrip, test_roundtrip_last_clk)
{
	roundtrip_check(CLK_ID_LAST, RATE_100MHZ);
}

ZTEST_SUITE(xlnx_clk_wiz_roundtrip, NULL, NULL, NULL, NULL, NULL);
