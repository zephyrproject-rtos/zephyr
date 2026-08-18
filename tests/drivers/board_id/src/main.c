/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 The Zephyr Project Contributors
 * Copyright (c) 2026 Dev It Wise
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/board_id.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/ztest.h>

/* idle-config = "none": no pull, straps read 0 at init. */
static const struct device *const board_id0 = DEVICE_DT_GET(DT_NODELABEL(board_id0));

/* idle-config = "pull-up": all straps read 1 at init. */
static const struct device *const board_id_pullup = DEVICE_DT_GET(DT_NODELABEL(board_id_pullup));

/* Single-pin edge case, no pull. */
static const struct device *const board_id_min = DEVICE_DT_GET(DT_NODELABEL(board_id_min));

/* 32-pin edge case, pulled up. */
static const struct device *const board_id_max = DEVICE_DT_GET(DT_NODELABEL(board_id_max));

static const struct device *const gpio_id = DEVICE_DT_GET(DT_NODELABEL(gpio_id));

ZTEST(board_id_gpio, test_ready)
{
	zassert_true(device_is_ready(board_id0), "board_id0 not ready");
	zassert_true(device_is_ready(board_id_pullup), "board_id_pullup not ready");
	zassert_true(device_is_ready(board_id_min), "board_id_min not ready");
	zassert_true(device_is_ready(board_id_max), "board_id_max not ready");
}

ZTEST(board_id_gpio, test_happy_path_no_pull)
{
	uint32_t id = 0xFFFFFFFFU;

	zassert_ok(board_id_read(board_id0, &id));
	zassert_equal(id, 0U, "expected all straps low, got 0x%08x", id);
}

ZTEST(board_id_gpio, test_happy_path_pull_up)
{
	uint32_t id = 0;

	zassert_ok(board_id_read(board_id_pullup, &id));
	zassert_equal(id, 0x7U, "expected all 3 straps high, got 0x%08x", id);
}

ZTEST(board_id_gpio, test_pin_count_min)
{
	uint32_t id = 0xFFFFFFFFU;

	zassert_ok(board_id_read(board_id_min, &id));
	zassert_equal(id, 0U, "expected single strap low, got 0x%08x", id);
}

ZTEST(board_id_gpio, test_pin_count_max)
{
	uint32_t id = 0;

	zassert_ok(board_id_read(board_id_max, &id));
	zassert_equal(id, 0xFFFFFFFFU, "expected all 32 straps high, got 0x%08x", id);
}

ZTEST(board_id_gpio, test_value_is_cached_at_init)
{
	uint32_t id = 0xFFFFFFFFU;

	/* Flip the underlying strap after init; the driver must keep returning
	 * the value it cached at init, not a live read.
	 */
	zassert_ok(gpio_emul_input_set(gpio_id, 0, 1));

	zassert_ok(board_id_read(board_id0, &id));
	zassert_equal(id, 0U, "cached value changed after a live GPIO edit: 0x%08x", id);

	/* Restore, so this test does not leak state into another test case. */
	zassert_ok(gpio_emul_input_set(gpio_id, 0, 0));
}

ZTEST(board_id_gpio, test_einval_on_null_out_param)
{
	zassert_equal(board_id_read(board_id0, NULL), -EINVAL);
}

/* Contract test independent of any backend: a driver that fails the read must
 * return -EIO and must not touch the caller's out-parameter.
 */
static int fake_board_id_read_eio(const struct device *dev, uint32_t *id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	return -EIO;
}

static DEVICE_API(board_id, fake_board_id_api) = {
	.read = fake_board_id_read_eio,
};

static const struct device fake_board_id_dev = {
	.name = "fake_board_id",
	.api = &fake_board_id_api,
};

ZTEST(board_id_gpio, test_eio_does_not_write_out_param)
{
	uint32_t id = 0xDEADBEEFU;

	zassert_equal(board_id_read(&fake_board_id_dev, &id), -EIO);
	zassert_equal(id, 0xDEADBEEFU, "out-parameter was written on an error path");
}

ZTEST_SUITE(board_id_gpio, NULL, NULL, NULL, NULL, NULL);
