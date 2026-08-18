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

/* 16-pin known bit pattern, to pin the emitted byte order. */
static const struct device *const board_id_pattern = DEVICE_DT_GET(DT_NODELABEL(board_id_pattern));

static const struct device *const gpio_id = DEVICE_DT_GET(DT_NODELABEL(gpio_id));

ZTEST(board_id_gpio, test_ready)
{
	zassert_true(device_is_ready(board_id0), "board_id0 not ready");
	zassert_true(device_is_ready(board_id_pullup), "board_id_pullup not ready");
	zassert_true(device_is_ready(board_id_min), "board_id_min not ready");
	zassert_true(device_is_ready(board_id_max), "board_id_max not ready");
	zassert_true(device_is_ready(board_id_pattern), "board_id_pattern not ready");
}

ZTEST(board_id_gpio, test_happy_path_no_pull)
{
	uint8_t buf[1] = {0xFFU};

	zassert_equal(board_id_read(board_id0, buf, sizeof(buf)), 1);
	zassert_equal(buf[0], 0x00U, "expected all straps low, got 0x%02x", buf[0]);
}

ZTEST(board_id_gpio, test_happy_path_pull_up)
{
	uint8_t buf[1] = {0};

	zassert_equal(board_id_read(board_id_pullup, buf, sizeof(buf)), 1);
	zassert_equal(buf[0], 0x07U, "expected all 3 straps high, got 0x%02x", buf[0]);
}

ZTEST(board_id_gpio, test_pin_count_min)
{
	uint8_t buf[1] = {0xFFU};

	zassert_equal(board_id_read(board_id_min, buf, sizeof(buf)), 1);
	zassert_equal(buf[0], 0x00U, "expected single strap low, got 0x%02x", buf[0]);
}

ZTEST(board_id_gpio, test_pin_count_max)
{
	uint8_t buf[4] = {0};

	zassert_equal(board_id_read(board_id_max, buf, sizeof(buf)), 4);
	zassert_equal(buf[0], 0xFFU, "byte 0 mismatch: 0x%02x", buf[0]);
	zassert_equal(buf[1], 0xFFU, "byte 1 mismatch: 0x%02x", buf[1]);
	zassert_equal(buf[2], 0xFFU, "byte 2 mismatch: 0x%02x", buf[2]);
	zassert_equal(buf[3], 0xFFU, "byte 3 mismatch: 0x%02x", buf[3]);
}

/* Packed value 0x8001: pin that the driver emits it big-endian, MSB first. */
ZTEST(board_id_gpio, test_byte_order)
{
	uint8_t buf[2] = {0};

	zassert_equal(board_id_read(board_id_pattern, buf, sizeof(buf)), 2);
	zassert_equal(buf[0], 0x80U, "MSB mismatch: 0x%02x", buf[0]);
	zassert_equal(buf[1], 0x01U, "LSB mismatch: 0x%02x", buf[1]);
}

ZTEST(board_id_gpio, test_short_buffer_returns_leading_bytes)
{
	uint8_t buf[1] = {0};

	/* Buffer shorter than the 2-byte identity: get the leading (MSB) byte only. */
	zassert_equal(board_id_read(board_id_pattern, buf, sizeof(buf)), 1);
	zassert_equal(buf[0], 0x80U, "expected leading byte only, got 0x%02x", buf[0]);
}

ZTEST(board_id_gpio, test_value_is_cached_at_init)
{
	uint8_t buf[1] = {0xFFU};

	/* Driver must return the value cached at init, not a live read. */
	zassert_ok(gpio_emul_input_set(gpio_id, 0, 1));

	zassert_equal(board_id_read(board_id0, buf, sizeof(buf)), 1);
	zassert_equal(buf[0], 0x00U, "cached value changed after a live GPIO edit: 0x%02x", buf[0]);

	/* Restore, so this test does not leak state into another test case. */
	zassert_ok(gpio_emul_input_set(gpio_id, 0, 0));
}

ZTEST(board_id_gpio, test_einval_on_null_buffer)
{
	zassert_equal(board_id_read(board_id0, NULL, 1), -EINVAL);
}

ZTEST(board_id_gpio, test_einval_on_zero_length)
{
	uint8_t buf[1] = {0};

	zassert_equal(board_id_read(board_id0, buf, 0), -EINVAL);
}

/* Backend-independent: a failing read must return -EIO. */
static ssize_t fake_board_id_read_eio(const struct device *dev, uint8_t *buffer, size_t length)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(buffer);
	ARG_UNUSED(length);

	return -EIO;
}

static DEVICE_API(board_id, fake_board_id_api) = {
	.read = fake_board_id_read_eio,
};

static const struct device fake_board_id_dev = {
	.name = "fake_board_id",
	.api = &fake_board_id_api,
};

ZTEST(board_id_gpio, test_eio_on_transport_failure)
{
	uint8_t buf[1] = {0};

	zassert_equal(board_id_read(&fake_board_id_dev, buf, sizeof(buf)), -EIO);
}

ZTEST_SUITE(board_id_gpio, NULL, NULL, NULL, NULL, NULL);
