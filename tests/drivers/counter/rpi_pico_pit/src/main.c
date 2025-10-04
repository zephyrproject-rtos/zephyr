/*
 * Copyright (c) 2024 Navimatix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/device.h"
#include "zephyr/kernel.h"
#include "zephyr/devicetree.h"
#include "zephyr/ztest_assert.h"
#include "zephyr/ztest_test.h"
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/counter.h>
#include <hardware/pwm.h>

struct counter_pico_pit_fixture {
	const struct device *pit;
	const struct device *pit_channel_1;
	const struct device *pit_channel_2;
};

const struct device *timer = DEVICE_DT_GET_ONE(raspberrypi_pico_timer);

static void counter_callback_1(const struct device *dev, void *userdata)
{
	int *value = userdata;

	*value = 1;
}

static void counter_callback_2(const struct device *dev, void *userdata)
{
	int *value = userdata;

	*value += 1;
}

static void *counter_pico_pit_setup(void)
{
	static struct counter_pico_pit_fixture fixture = {
		.pit = DEVICE_DT_GET(DT_NODELABEL(rpi_pico_pit_controller)),
		.pit_channel_1 = DEVICE_DT_GET(DT_NODELABEL(rpi_pico_pit_channel_1)),
		.pit_channel_2 = DEVICE_DT_GET(DT_NODELABEL(rpi_pico_pit_channel_2)),
	};

	zassert_not_null(fixture.pit);
	zassert_not_null(fixture.pit_channel_1);
	zassert_not_null(fixture.pit_channel_2);
	return &fixture;
}

static void counter_pico_pit_before(void *f)
{
	struct counter_pico_pit_fixture *fixture = f;
	struct counter_top_cfg top_cfg = {.callback = NULL, .user_data = NULL, .flags = 0};

	top_cfg.ticks = UINT16_MAX;

	zassert_ok(counter_stop(fixture->pit_channel_1));
	zassert_ok(counter_stop(fixture->pit_channel_2));
	zassert_ok(counter_set_top_value(fixture->pit_channel_1, &top_cfg));
	zassert_ok(counter_set_top_value(fixture->pit_channel_2, &top_cfg));
}

ZTEST_F(counter_pico_pit, test_no_value_change_after_stop)
{
	uint32_t value_1 = 0;
	uint32_t value_2 = 0;

	zassert_ok(counter_start(fixture->pit_channel_1));
	k_msleep(100);
	zassert_ok(counter_stop(fixture->pit_channel_1));
	zassert_ok(counter_get_value(fixture->pit_channel_1, &value_1));
	k_msleep(200);
	zassert_ok(counter_get_value(fixture->pit_channel_1, &value_2));
	zassert_equal(value_2, value_1,
		      "Counter value should not have changed (should be %u but is %u)", value_1,
		      value_2);
}

ZTEST_F(counter_pico_pit, test_value_increase_over_time)
{
	uint32_t value_1 = 0;
	uint32_t value_2 = 0;

	zassert_ok(counter_start(fixture->pit_channel_1));
	zassert_ok(counter_get_value(fixture->pit_channel_1, &value_1));
	k_msleep(50);
	zassert_ok(counter_get_value(fixture->pit_channel_1, &value_2));
	zassert_true(value_1 < value_2,
		     "Counter value has increased (%u should be greater than %u)", value_2,
		     value_1);
}

ZTEST_F(counter_pico_pit, test_set_top_value)
{
	uint32_t top_value = 20000;
	uint32_t init_top_value = counter_get_top_value(fixture->pit_channel_1);

	zassert_equal(init_top_value, UINT16_MAX, "Initial top value should be %u but is %u",
		      UINT16_MAX, init_top_value);
	struct counter_top_cfg top_cfg = {
		.callback = NULL, .user_data = NULL, .flags = 0, .ticks = top_value};

	zassert_ok(counter_set_top_value(fixture->pit_channel_1, &top_cfg));

	uint32_t current_top_value = counter_get_top_value(fixture->pit_channel_1);

	zassert_equal(top_value, current_top_value, "Top value should be %u but is %u", top_value,
		      current_top_value);
}

ZTEST_F(counter_pico_pit, test_counter_wraps)
{
	uint32_t value_1 = 0;
	uint32_t value_2 = 0;
	uint32_t freq = counter_get_frequency(fixture->pit_channel_1);

	struct counter_top_cfg top_cfg = {.callback = NULL, .user_data = NULL, .flags = 0};

	top_cfg.ticks = freq / 10U;
	zassert_ok(counter_set_top_value(fixture->pit_channel_1, &top_cfg));
	k_msleep(70);
	zassert_ok(counter_get_value(fixture->pit_channel_1, &value_1));
	k_msleep(40);
	zassert_ok(counter_get_value(fixture->pit_channel_1, &value_2));
	zassert_true(value_1 > value_2, "Counter did not wrap (%u should be smaller than %u)",
		     value_2, value_1);
}

ZTEST_F(counter_pico_pit, test_top_value_zero_ticks)
{
	uint32_t top_value = 0;
	struct counter_top_cfg top_cfg = {
		.callback = NULL, .user_data = NULL, .flags = 0, .ticks = top_value};
	int ret = counter_set_top_value(fixture->pit_channel_1, &top_cfg);

	zassert_not_equal(ret, 0, "Counter wrongly accepted top value of 0 ticks");
}

ZTEST_F(counter_pico_pit, test_top_value_interrupt_set)
{
	int32_t freq = counter_get_frequency(fixture->pit_channel_1);
	int data = 0;

	struct counter_top_cfg top_cfg = {
		.callback = counter_callback_1, .user_data = &data, .flags = 0};

	top_cfg.ticks = freq / 10U;
	zassert_ok(counter_set_top_value(fixture->pit_channel_1, &top_cfg));
	k_msleep(200);
	zassert_equal(data, 1, "Counter top callback did not fire");
}

ZTEST_F(counter_pico_pit, test_top_value_interrupt_unset)
{
	int32_t freq = counter_get_frequency(fixture->pit_channel_1);
	int data = 0;

	struct counter_top_cfg top_cfg = {
		.callback = counter_callback_2, .user_data = &data, .flags = 0};

	top_cfg.ticks = freq / 10U;
	zassert_ok(counter_set_top_value(fixture->pit_channel_1, &top_cfg));
	k_msleep(120);
	top_cfg.callback = NULL;

	int ret = counter_set_top_value(fixture->pit_channel_1, &top_cfg);

	zassert_equal(ret, 0, "Error on top callback unset");
	k_msleep(120);
	zassert_equal(data, 1, "Counter top callback was not unset");
}

ZTEST_F(counter_pico_pit, test_top_value_no_counter_value_reset)
{
	zassert_ok(counter_start(fixture->pit_channel_1));
	struct counter_top_cfg top_cfg = {
		.callback = NULL, .user_data = NULL, .flags = COUNTER_TOP_CFG_DONT_RESET};

	top_cfg.ticks = UINT16_MAX;
	k_msleep(50);
	zassert_ok(counter_set_top_value(fixture->pit_channel_1, &top_cfg));

	uint32_t value = 0;

	zassert_ok(counter_get_value(fixture->pit_channel_1, &value));
	zassert_true(value > 10000, "Counter value should not have reset but it did, %u", value);
}

ZTEST_F(counter_pico_pit, test_top_value_no_value_reset_new_top_value_smaller_than_counter_value)
{
	zassert_ok(counter_start(fixture->pit_channel_1));

	k_msleep(50);
	struct counter_top_cfg top_cfg = {
		.callback = NULL, .user_data = NULL, .flags = COUNTER_TOP_CFG_DONT_RESET};

	top_cfg.ticks = 10000;
	int ret = counter_set_top_value(fixture->pit_channel_1, &top_cfg);

	zassert_equal(ret, -ETIME, "set_top_value should have returned -ETIME but did return %d",
		      ret);
}

ZTEST_F(counter_pico_pit, test_two_pit_channels_top_interrupts)
{
	int32_t freq = counter_get_frequency(fixture->pit_channel_1);
	int data_1 = 0;
	int data_2 = 0;
	struct counter_top_cfg top_cfg_1 = {
		.callback = counter_callback_2, .user_data = &data_1, .flags = 0};

	top_cfg_1.ticks = freq / 10U;

	struct counter_top_cfg top_cfg_2 = {
		.callback = counter_callback_2, .user_data = &data_2, .flags = 0};

	top_cfg_2.ticks = freq / 20U;
	data_1 = 0;
	zassert_ok(counter_set_top_value(fixture->pit_channel_1, &top_cfg_1));
	zassert_ok(counter_set_top_value(fixture->pit_channel_2, &top_cfg_2));

	k_msleep(210);
	zassert_ok(counter_stop(fixture->pit_channel_1));
	zassert_ok(counter_stop(fixture->pit_channel_2));

	zassert_equal(data_1, 2,
		      "Top interrupt for pit channel 1 did not fire the correct number of times "
		      "(should be %d but is %d)",
		      2, data_1);
	zassert_equal(data_2, 4,
		      "Top interrupt for pit channel 2 did not fire the correct number of times "
		      "(should be %d but is %d)",
		      4, data_2);
}

ZTEST_SUITE(counter_pico_pit, NULL, counter_pico_pit_setup, counter_pico_pit_before, NULL, NULL);
