/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/comparator/nrf_comp.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

static const struct device *test_dev = DEVICE_DT_GET(DT_ALIAS(test_comp));
static const struct gpio_dt_spec test_pin_1 = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), first_gpios);
static const struct gpio_dt_spec test_pin_2 = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), second_gpios);


struct comp_nrf_comp_se_config comp_se_config = {
#if defined(CONFIG_BOARD_NRF54LM20APDK)
	.psel = COMP_NRF_COMP_PSEL_AIN1,
	.extrefsel = COMP_NRF_COMP_EXTREFSEL_AIN5,
#else
	.psel = COMP_NRF_COMP_PSEL_AIN5,
	.extrefsel = COMP_NRF_COMP_EXTREFSEL_AIN1,
#endif
	.sp_mode = COMP_NRF_COMP_SP_MODE_HIGH,
	.isource = COMP_NRF_COMP_ISOURCE_DISABLED,
	.refsel = COMP_NRF_COMP_REFSEL_AREF,
	.th_up = 32,
	.th_down = 32,
};

struct comp_nrf_comp_diff_config comp_diff_config = {
#if defined(CONFIG_BOARD_NRF54LM20APDK)
	.psel = COMP_NRF_COMP_PSEL_AIN3,
	.extrefsel = COMP_NRF_COMP_EXTREFSEL_AIN1,
#else
	.psel = COMP_NRF_COMP_PSEL_AIN4,
	.extrefsel = COMP_NRF_COMP_EXTREFSEL_AIN5,
#endif
	.sp_mode = COMP_NRF_COMP_SP_MODE_LOW,
	.isource = COMP_NRF_COMP_ISOURCE_DISABLED,
	.enable_hyst = true,
};

atomic_t counter;

static void test_callback(const struct device *dev, void *user_data)
{
	counter++;
}

/**
 * @brief Configure comparator in single-ended mode with
 * external voltage reference.
 * Check if events were detected.
 */

ZTEST(comparator_runtime_configure, test_comp_config_se_aref)
{
	int rc;

	rc = comp_nrf_comp_configure_se(test_dev, &comp_se_config);
	zassert_equal(rc, 0, "Cannot configure comparator.");

	rc = comparator_set_trigger_callback(test_dev, test_callback, NULL);
	zassert_equal(rc, 0, "Cannot set callback for comparator.");

	rc = comparator_set_trigger(test_dev, COMPARATOR_TRIGGER_BOTH_EDGES);
	zassert_equal(rc, 0, "Cannot set trigger for comparator.");

	k_msleep(10);

	atomic_clear(&counter);
	gpio_pin_set_dt(&test_pin_2, 1);
	k_msleep(10);
	zassert_equal(atomic_get(&counter), 1, "COMP was not triggered for first threshold cross");
	gpio_pin_set_dt(&test_pin_2, 0);
	k_msleep(10);
	zassert_equal(atomic_get(&counter), 2, "COMP was not triggered for second threshold cross");

}

/**
 * @brief Configure comparator in single-ended mode with
 * internal voltage reference.
 * Check if events were detected.
 */


ZTEST(comparator_runtime_configure, test_comp_config_se_vdd)
{
	int rc;

	struct comp_nrf_comp_se_config conf = comp_se_config;

#ifdef COMP_REFSEL_REFSEL_AVDDAO1V8
	conf.refsel = COMP_NRF_COMP_REFSEL_AVDDAO1V8;
#else
	conf.refsel = COMP_NRF_COMP_REFSEL_VDD;
#endif
	rc = comp_nrf_comp_configure_se(test_dev, &conf);
	zassert_equal(rc, 0, "Cannot configure comparator.");

	rc = comparator_set_trigger_callback(test_dev, test_callback, NULL);
	zassert_equal(rc, 0, "Cannot set callback for comparator.");

	rc = comparator_set_trigger(test_dev, COMPARATOR_TRIGGER_BOTH_EDGES);
	zassert_equal(rc, 0, "Cannot set trigger for comparator.");

	k_msleep(10);

	atomic_clear(&counter);
	gpio_pin_set_dt(&test_pin_2, 1);
	k_msleep(10);
	zassert_equal(atomic_get(&counter), 1, "COMP was not triggered for first threshold cross");
	gpio_pin_set_dt(&test_pin_2, 0);
	k_msleep(10);
	zassert_equal(atomic_get(&counter), 2, "COMP was not triggered for second threshold cross");

}

/**
 * @brief Configure comparator in differential mode
 * Check if events were detected.
 */

ZTEST(comparator_runtime_configure, test_comp_config_diff_both)
{
	int rc;

	gpio_pin_set_dt(&test_pin_1, 1);
	gpio_pin_set_dt(&test_pin_2, 0);
	comparator_trigger_is_pending(test_dev);

	atomic_clear(&counter);
	struct comp_nrf_comp_diff_config config = comp_diff_config;

	config.isource = COMP_NRF_COMP_ISOURCE_2UA5;
	rc = comp_nrf_comp_configure_diff(test_dev, &config);
	zassert_equal(rc, 0, "Cannot configure comparator.");

	rc = comparator_set_trigger_callback(test_dev, test_callback, NULL);
	zassert_equal(rc, 0, "Cannot set callback for comparator.");

	rc = comparator_set_trigger(test_dev, COMPARATOR_TRIGGER_BOTH_EDGES);
	zassert_equal(rc, 0, "Cannot set trigger for comparator.");
	k_msleep(10);

	atomic_clear(&counter);
	gpio_pin_set_dt(&test_pin_1, 0);
	gpio_pin_set_dt(&test_pin_2, 1);
	k_msleep(10);
	zassert_equal(atomic_get(&counter), 1, "COMP was not triggered for first threshold cross");

	gpio_pin_set_dt(&test_pin_2, 0);
	gpio_pin_set_dt(&test_pin_1, 1);
	k_msleep(10);

	zassert_equal(atomic_get(&counter), 2, "COMP was not triggered for second threshold cross");
}

/**
 * @brief Configure comparator in differential mode
 * trigger both edges, event should be detected for falling one
 */

ZTEST(comparator_runtime_configure, test_comp_config_diff_falling)
{
	int rc;

	gpio_pin_set_dt(&test_pin_1, 0);
	gpio_pin_set_dt(&test_pin_2, 1);
	comparator_trigger_is_pending(test_dev);

	atomic_clear(&counter);
	struct comp_nrf_comp_diff_config config = comp_diff_config;

	config.isource = COMP_NRF_COMP_ISOURCE_5UA;
	rc = comp_nrf_comp_configure_diff(test_dev, &config);

	zassert_equal(rc,  0, "Cannot configure comparator.");

	rc = comparator_set_trigger_callback(test_dev, test_callback, NULL);
	zassert_equal(rc, 0, "Cannot set callback for comparator.");

	rc = comparator_set_trigger(test_dev, COMPARATOR_TRIGGER_FALLING_EDGE);
	zassert_equal(rc, 0, "Cannot set trigger for comparator.");

	atomic_clear(&counter);
	gpio_pin_set_dt(&test_pin_1, 1);
	gpio_pin_set_dt(&test_pin_2, 0);
	k_msleep(10);

	zassert_equal(counter, 0, "COMP was triggered for rising threshold cross");

	gpio_pin_set_dt(&test_pin_1, 0);
	gpio_pin_set_dt(&test_pin_2, 1);
	k_msleep(10);

	zassert_equal(atomic_get(&counter), 1, "COMP wasn't triggered for falling threshold cross");
}

/**
 * @brief Configure comparator in differential mode
 * trigger both edges, event should be detected for rising one
 */

ZTEST(comparator_runtime_configure, test_comp_config_diff_rising)
{
	int rc;

	gpio_pin_set_dt(&test_pin_1, 1);
	gpio_pin_set_dt(&test_pin_2, 0);
	comparator_trigger_is_pending(test_dev);

	atomic_clear(&counter);
	struct comp_nrf_comp_diff_config config = comp_diff_config;

	config.isource = COMP_NRF_COMP_ISOURCE_10UA;
	rc = comp_nrf_comp_configure_diff(test_dev, &config);
	zassert_equal(rc, 0, "Cannot configure comparator.");

	rc = comparator_set_trigger_callback(test_dev, test_callback, NULL);
	zassert_equal(rc, 0, "Cannot set callback for comparator.");

	rc = comparator_set_trigger(test_dev, COMPARATOR_TRIGGER_RISING_EDGE);
	zassert_equal(rc, 0, "Cannot set trigger for comparator.");

	atomic_clear(&counter);
	gpio_pin_set_dt(&test_pin_1, 0);
	gpio_pin_set_dt(&test_pin_2, 1);
	k_msleep(10);

	zassert_equal(atomic_get(&counter), 0, "COMP was triggered for falling threshold cross");

	gpio_pin_set_dt(&test_pin_1, 1);
	gpio_pin_set_dt(&test_pin_2, 0);
	k_msleep(10);

	zassert_equal(atomic_get(&counter), 1, "COMP was not triggered for rising threshold cross");
}

/**
 * @brief Configure comparator in differential mode
 * trigger both edges, event should not be detected.
 */

ZTEST(comparator_runtime_configure, test_comp_config_diff_none)
{
	int rc;

	gpio_pin_set_dt(&test_pin_1, 1);
	gpio_pin_set_dt(&test_pin_2, 0);
	comparator_trigger_is_pending(test_dev);

	atomic_clear(&counter);
	struct comp_nrf_comp_diff_config config = comp_diff_config;

	config.isource = COMP_NRF_COMP_ISOURCE_10UA;
	rc = comp_nrf_comp_configure_diff(test_dev, &config);
	zassert_equal(rc, 0, "Cannot configure comparator.");

	rc = comparator_set_trigger_callback(test_dev, test_callback, NULL);
	zassert_equal(rc, 0, "Cannot set callback for comparator.");

	rc = comparator_set_trigger(test_dev, COMPARATOR_TRIGGER_NONE);
	zassert_equal(rc, 0, "Cannot set trigger for comparator.");

	atomic_clear(&counter);
	gpio_pin_set_dt(&test_pin_1, 0);
	gpio_pin_set_dt(&test_pin_2, 1);
	k_msleep(10);

	zassert_equal(atomic_get(&counter), 0, "COMP was triggered for falling threshold cross");

	gpio_pin_set_dt(&test_pin_1, 1);
	gpio_pin_set_dt(&test_pin_2, 0);
	k_msleep(10);

	zassert_equal(atomic_get(&counter), 0, "COMP was not triggered for rising threshold cross");
}

static void *suite_setup(void)
{
	TC_PRINT("Test executed on %s\n", CONFIG_BOARD_TARGET);
	TC_PRINT("===================================================================\n");

	return NULL;
}

static void test_before(void *f)
{
	ARG_UNUSED(f);
	gpio_pin_configure_dt(&test_pin_1, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&test_pin_2, GPIO_OUTPUT_INACTIVE);

}

ZTEST_SUITE(comparator_runtime_configure, NULL, suite_setup, test_before, NULL, NULL);
