/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hal/ana_cmpr_periph.h>
#include <hal/gpio_ll.h>

#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#define ZERO_DET_PARENT DT_NODELABEL(ana_cmpr0)
#define ZERO_DET_USER   DT_PATH(zephyr_user)

#define ZERO_DET_CHAN_GET(node) DEVICE_DT_GET(node),

static const struct device *const chans[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(ZERO_DET_PARENT, ZERO_DET_CHAN_GET)
};

static const struct gpio_dt_spec pins[] = {
	DT_FOREACH_PROP_ELEM_SEP(ZERO_DET_USER, test_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))
};

#if DT_NODE_HAS_PROP(ZERO_DET_USER, ref_gpios)
#define ZERO_DET_CHAN_PAD(node) DT_PROP(node, source_pad),
static const struct gpio_dt_spec ref_pin = GPIO_DT_SPEC_GET(ZERO_DET_USER, ref_gpios);
static const uint8_t chan_pads[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(ZERO_DET_PARENT, ZERO_DET_CHAN_PAD)
};
#define ZERO_DET_HAS_EXT_REF 1
#else
#define ZERO_DET_HAS_EXT_REF 0
#endif

static atomic_t callback_hits;
static const struct device *callback_dev;

BUILD_ASSERT(ARRAY_SIZE(chans) == ARRAY_SIZE(pins), "one GPIO per channel");
BUILD_ASSERT(ARRAY_SIZE(chans) >= 3, "enable three channels");

static void zero_det_callback(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	if (dev == callback_dev) {
		atomic_inc(&callback_hits);
	}
}

static void set_pin(size_t idx, int value)
{
	zassert_ok(gpio_pin_set_dt(&pins[idx], value));
}

static void set_all(int value)
{
	for (size_t i = 0; i < ARRAY_SIZE(pins); i++) {
		set_pin(i, value);
	}
}

static void *zero_det_setup(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(chans); i++) {
		zassert_true(device_is_ready(chans[i]));
		zassert_true(gpio_is_ready_dt(&pins[i]));
		zassert_ok(gpio_pin_configure_dt(&pins[i], GPIO_OUTPUT_INACTIVE));
	}

#if ZERO_DET_HAS_EXT_REF
	zassert_true(gpio_is_ready_dt(&ref_pin));
	zassert_ok(gpio_pin_configure_dt(&ref_pin, GPIO_OUTPUT_INACTIVE));
#endif

	return NULL;
}

static void zero_det_before(void *fixture)
{
	ARG_UNUSED(fixture);

	atomic_set(&callback_hits, 0);
	callback_dev = NULL;
	set_all(0);
#if ZERO_DET_HAS_EXT_REF
	zassert_ok(gpio_pin_configure_dt(&ref_pin, GPIO_OUTPUT_INACTIVE));
#endif
	k_msleep(1);

	for (size_t i = 0; i < ARRAY_SIZE(chans); i++) {
		zassert_ok(comparator_set_trigger(chans[i], COMPARATOR_TRIGGER_NONE));
		zassert_ok(comparator_set_trigger_callback(chans[i], NULL, NULL));
	}
}

ZTEST(ana_cmpr_zd, test_output_follows_pin)
{
#if ZERO_DET_HAS_EXT_REF
	const int *pad_gpios = ana_cmpr_periph[0].pad_gpios;

	for (size_t i = 0; i < ARRAY_SIZE(chan_pads); i++) {
		uint32_t gpio = (uint32_t)pad_gpios[chan_pads[i]];

		gpio_ll_output_disable(&GPIO, gpio);
		gpio_ll_pullup_en(&GPIO, gpio);
		gpio_ll_pulldown_en(&GPIO, gpio);
	}

	zassert_ok(gpio_pin_set_dt(&ref_pin, 1));
	k_msleep(1);
	for (size_t i = 0; i < ARRAY_SIZE(chans); i++) {
		zassert_equal(comparator_get_output(chans[i]), 0);
	}

	zassert_ok(gpio_pin_set_dt(&ref_pin, 0));
	k_msleep(1);
	for (size_t i = 0; i < ARRAY_SIZE(chans); i++) {
		zassert_equal(comparator_get_output(chans[i]), 1);
	}

	for (size_t i = 0; i < ARRAY_SIZE(pins); i++) {
		zassert_ok(gpio_pin_configure_dt(&pins[i], GPIO_OUTPUT_INACTIVE));
	}
#endif

	for (size_t i = 0; i < ARRAY_SIZE(chans); i++) {
		set_all(0);
		set_pin(i, 1);
		k_msleep(1);

		zassert_equal(comparator_get_output(chans[i]), 1);

		if (ZERO_DET_HAS_EXT_REF == 0) {
			for (size_t j = 0; j < ARRAY_SIZE(chans); j++) {
				if (j == i) {
					continue;
				}

				zassert_equal(comparator_get_output(chans[j]), 0);
			}
		}
	}
}

ZTEST(ana_cmpr_zd, test_trigger_stays_on_its_channel)
{
#if ZERO_DET_HAS_EXT_REF
	uint32_t ref_gpio = (uint32_t)ana_cmpr_periph[0].pad_gpios[
		DT_PROP(ZERO_DET_PARENT, ext_ref_pad)];

	gpio_ll_output_disable(&GPIO, ref_gpio);
	gpio_ll_pullup_en(&GPIO, ref_gpio);
	gpio_ll_pulldown_en(&GPIO, ref_gpio);
	k_msleep(1);
#endif

	for (size_t i = 0; i < ARRAY_SIZE(chans); i++) {
		int level = comparator_get_output(chans[i]);

		zassert_equal(level, 0);
	}

	zassert_ok(comparator_set_trigger(chans[0], COMPARATOR_TRIGGER_RISING_EDGE));
	callback_dev = chans[0];
	zassert_ok(comparator_set_trigger_callback(chans[0], zero_det_callback, NULL));

	set_pin(1, 1);
	k_msleep(2);
	zassert_equal(atomic_get(&callback_hits), 0);

	set_pin(0, 1);
	k_msleep(2);
	zassert_equal(atomic_get(&callback_hits), 1);
	zassert_equal(comparator_get_output(chans[0]), 1);

	if (ZERO_DET_HAS_EXT_REF == 0) {
		zassert_equal(comparator_get_output(chans[1]), 1);
		zassert_equal(comparator_get_output(chans[2]), 0);
	}
}

ZTEST(ana_cmpr_zd, test_disarm_one_of_two)
{
	if (ZERO_DET_HAS_EXT_REF != 0) {
		ztest_test_skip();
	}

	zassert_ok(comparator_set_trigger(chans[0], COMPARATOR_TRIGGER_RISING_EDGE));
	zassert_ok(comparator_set_trigger(chans[1], COMPARATOR_TRIGGER_FALLING_EDGE));
	zassert_ok(comparator_set_trigger(chans[0], COMPARATOR_TRIGGER_NONE));

	set_pin(0, 1);
	set_pin(1, 1);
	k_msleep(2);
	set_pin(0, 0);
	set_pin(1, 0);
	k_msleep(2);

	zassert_equal(comparator_trigger_is_pending(chans[0]), 0);
	zassert_equal(comparator_trigger_is_pending(chans[1]), 1);
}

ZTEST(ana_cmpr_zd, test_arm_over_stale_result)
{
	if (ZERO_DET_HAS_EXT_REF != 0) {
		ztest_test_skip();
	}

	zassert_equal(comparator_get_output(chans[0]), 0);

	set_pin(0, 1);
	k_msleep(1);

	zassert_ok(comparator_set_trigger(chans[0], COMPARATOR_TRIGGER_RISING_EDGE));
	k_msleep(2);
	zassert_equal(comparator_trigger_is_pending(chans[0]), 0);
	zassert_equal(comparator_get_output(chans[0]), 1);

	set_pin(0, 0);
	k_msleep(1);
	set_pin(0, 1);
	k_msleep(2);
	zassert_equal(comparator_trigger_is_pending(chans[0]), 1);
}

ZTEST_SUITE(ana_cmpr_zd, NULL, zero_det_setup, zero_det_before, NULL, NULL);
