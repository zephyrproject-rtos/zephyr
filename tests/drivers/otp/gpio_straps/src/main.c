/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/nvmem.h>
#include <zephyr/ztest.h>

static const struct device *const straps_low = DEVICE_DT_GET(DT_NODELABEL(straps_low));
static const struct device *const straps_high = DEVICE_DT_GET(DT_NODELABEL(straps_high));
static const struct device *const straps_active_low =
	DEVICE_DT_GET(DT_NODELABEL(straps_active_low));
static const struct device *const straps_pattern = DEVICE_DT_GET(DT_NODELABEL(straps_pattern));
static const struct device *const wide_gpio = DEVICE_DT_GET(DT_NODELABEL(wide_gpio));

static const struct nvmem_cell revision =
	NVMEM_CELL_GET_BY_NAME(DT_NODELABEL(nvmem_consumer), revision);

ZTEST(otp_gpio_straps, test_ready)
{
	zassert_true(device_is_ready(straps_low));
	zassert_true(device_is_ready(straps_high));
	zassert_true(device_is_ready(straps_active_low));
	zassert_true(device_is_ready(straps_pattern));
}

ZTEST(otp_gpio_straps, test_unbiased_straps_read_zero)
{
	uint8_t buf = 0xFFU;

	zassert_ok(otp_read(straps_low, 0, &buf, sizeof(buf)));
	zassert_equal(buf, 0x00U, "expected 0x00, got 0x%02x", buf);
}

ZTEST(otp_gpio_straps, test_pulled_up_straps_read_one)
{
	uint8_t buf = 0x00U;

	zassert_ok(otp_read(straps_high, 0, &buf, sizeof(buf)));
	zassert_equal(buf, 0x07U, "expected 0x07, got 0x%02x", buf);
}

/* Strap 0 is pulled up and strap 1 is not, so active low inverts both. */
ZTEST(otp_gpio_straps, test_active_low_inverts_the_pin)
{
	uint8_t buf = 0xFFU;

	zassert_ok(otp_read(straps_active_low, 0, &buf, sizeof(buf)));
	zassert_equal(buf, 0x02U, "expected 0x02, got 0x%02x", buf);
}

/* Bit N comes from strap N, packed least significant bit first, byte 0 first. */
ZTEST(otp_gpio_straps, test_bit_and_byte_packing)
{
	uint8_t buf[2] = {0};

	zassert_ok(otp_read(straps_pattern, 0, buf, sizeof(buf)));
	zassert_equal(buf[0], 0x01U, "byte 0: expected 0x01, got 0x%02x", buf[0]);
	zassert_equal(buf[1], 0x80U, "byte 1: expected 0x80, got 0x%02x", buf[1]);
}

ZTEST(otp_gpio_straps, test_read_at_offset)
{
	uint8_t buf = 0x00U;

	zassert_ok(otp_read(straps_pattern, 1, &buf, sizeof(buf)));
	zassert_equal(buf, 0x80U, "expected 0x80, got 0x%02x", buf);
}

ZTEST(otp_gpio_straps, test_read_of_zero_length_is_a_no_op)
{
	uint8_t buf = 0xA5U;

	zassert_ok(otp_read(straps_pattern, 2, &buf, 0));
	zassert_equal(buf, 0xA5U, "the buffer was touched: 0x%02x", buf);
}

ZTEST(otp_gpio_straps, test_read_past_the_end_is_rejected)
{
	uint8_t buf[2] = {0};

	/* Two bytes wide: one byte read from offset 1 is the last legal read. */
	zassert_ok(otp_read(straps_pattern, 1, buf, 1));
	zassert_equal(otp_read(straps_pattern, 1, buf, 2), -EINVAL);
	zassert_equal(otp_read(straps_pattern, 2, buf, 1), -EINVAL);
	zassert_equal(otp_read(straps_pattern, 0, buf, 3), -EINVAL);
}

ZTEST(otp_gpio_straps, test_negative_offset_is_rejected)
{
	uint8_t buf = 0;

	zassert_equal(otp_read(straps_pattern, -1, &buf, sizeof(buf)), -EINVAL);
}

ZTEST(otp_gpio_straps, test_straps_are_released_after_sampling)
{
	gpio_flags_t flags = GPIO_INPUT;
	uint8_t buf[2] = {0};

	/* One sample is taken, so no strap is left biased against its resistor. */
	zassert_ok(gpio_emul_flags_get(wide_gpio, 0, &flags));
	zassert_equal(flags, GPIO_DISCONNECTED, "strap left configured: 0x%x", flags);

	/* The bit captured before release is still returned by later reads. */
	zassert_ok(otp_read(straps_pattern, 0, buf, sizeof(buf)));
	zassert_equal(buf[0], 0x01U, "byte 0: expected 0x01, got 0x%02x", buf[0]);
	zassert_equal(buf[1], 0x80U, "byte 1: expected 0x80, got 0x%02x", buf[1]);
}

ZTEST(otp_gpio_straps, test_straps_cannot_be_programmed)
{
	const uint8_t buf = 0x01U;

	zassert_equal(otp_program(straps_high, 0, &buf, sizeof(buf)), -ENOSYS);
}

/* The driver names nothing: a board names its fields with nvmem cells. */
ZTEST(otp_gpio_straps, test_nvmem_cell_over_the_straps)
{
	uint8_t buf = 0x00U;

	zassert_ok(nvmem_cell_read(&revision, &buf, 0, sizeof(buf)));
	zassert_equal(buf, 0x07U, "expected 0x07, got 0x%02x", buf);
}

ZTEST_SUITE(otp_gpio_straps, NULL, NULL, NULL, NULL, NULL);
