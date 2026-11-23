/*
 * Copyright (c) 2025 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

ZTEST_SUITE(reset_mmio_tests, NULL, NULL, NULL, NULL, NULL);

#define RESET_MAX_NUM 16

void check_status(const struct device *dev, uint32_t base, uint32_t id, bool expected_state,
		  bool active_low)
{
	uint8_t actual_state;

	zassert_ok(reset_status(dev, id, &actual_state), "Failed getting reset state");
	zassert_equal(actual_state, expected_state,
		      "reset state %u doesn't match expected state %u", actual_state,
		      expected_state);
	zassert_equal(FIELD_GET(BIT(id), sys_read32(base)), active_low ^ expected_state);
}

/* Tests that the reset driver assert functionality is correct for active
 * low devices.
 */
ZTEST(reset_mmio_tests, test_reset_mmio_assert_active_low)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(reset1));
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(reset1));
	bool active_low = true;
	uint8_t i;

	for (i = 0; i < RESET_MAX_NUM; i++) {
		reset_line_deassert(dev, i);
		check_status(dev, base, i, false, active_low);
		/* Check idempotency */
		reset_line_deassert(dev, i);
		check_status(dev, base, i, false, active_low);

		/* Check ressserting resets */
		reset_line_assert(dev, i);
		check_status(dev, base, i, true, active_low);
		/* Check idempotency */
		reset_line_assert(dev, i);
		check_status(dev, base, i, true, active_low);
	}
}

/* Tests that the reset driver assert functionality is correct for active
 * high devices.
 */
ZTEST(reset_mmio_tests, test_reset_mmio_assert_active_high)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(reset0));
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(reset0));
	bool active_low = false;
	uint8_t i;

	for (i = 0; i < RESET_MAX_NUM; i++) {
		reset_line_deassert(dev, i);
		check_status(dev, base, i, false, active_low);
		/* Check idempotency */
		reset_line_deassert(dev, i);
		check_status(dev, base, i, false, active_low);

		/* Check ressserting resets */
		reset_line_assert(dev, i);
		check_status(dev, base, i, true, active_low);
		/* Check idempotency */
		reset_line_assert(dev, i);
		check_status(dev, base, i, true, active_low);
	}
}

/* Tests that the reset driver toggle functionality is correct for active
 * low devices
 */
ZTEST(reset_mmio_tests, test_reset_mmio_toggle_active_low)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(reset1));
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(reset1));
	bool active_low = true;
	uint8_t i;

	for (i = 0; i < RESET_MAX_NUM; i++) {
		/* Begin by making sure the reset is asserted */
		reset_line_assert(dev, i);
		check_status(dev, base, i, true, active_low);
		reset_line_toggle(dev, i);
		check_status(dev, base, i, false, active_low);
		reset_line_toggle(dev, i);
		check_status(dev, base, i, true, active_low);
	}
}

/* Tests that the reset driver toggle functionality is correct for active
 * high devices
 */
ZTEST(reset_mmio_tests, test_reset_mmio_toggle_active_high)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(reset0));
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(reset0));
	bool active_low = false;
	uint8_t i;

	for (i = 0; i < RESET_MAX_NUM; i++) {
		/* Begin by making sure the reset is asserted */
		reset_line_assert(dev, i);
		check_status(dev, base, i, true, active_low);
		reset_line_toggle(dev, i);
		check_status(dev, base, i, false, active_low);
		reset_line_toggle(dev, i);
		check_status(dev, base, i, true, active_low);
	}
}

/* Tests that the reset driver rejects out of bounds bits */
ZTEST(reset_mmio_tests, test_reset_mmio_oob)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(reset0));
	uint8_t i, status;

	for (i = RESET_MAX_NUM; i < 32; i++) {
		zassert_not_ok(reset_line_assert(dev, i));
		zassert_not_ok(reset_line_deassert(dev, i));
		zassert_not_ok(reset_status(dev, i, &status));
		zassert_not_ok(reset_line_toggle(dev, i));
	}
}
