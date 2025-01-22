/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generate PWM signals in different configurations and use a GPIO
 * input pin to check the programmed timing. This test uses the systimer as
 * benchmark, so it assumes the system tick is verified and precise.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/gpio.h>
#include <stdlib.h>
#include <math.h>

static struct gpio_callback gpio_cb;

#define TEST_PWM_COUNT                      DT_PROP_LEN(DT_PATH(zephyr_user), pwms)
#define TEST_PWM_CONFIG_ENTRY(idx, node_id) PWM_DT_SPEC_GET_BY_IDX(node_id, idx)
#define TEST_PWM_CONFIG_ARRAY(node_id)                                                             \
	{                                                                                          \
		LISTIFY(TEST_PWM_COUNT, TEST_PWM_CONFIG_ENTRY, (,), node_id)                       \
	}

#define TEST_GPIO_COUNT                      DT_PROP_LEN(DT_PATH(zephyr_user), gpios)
#define TEST_GPIO_CONFIG_ENTRY(idx, node_id) GPIO_DT_SPEC_GET_BY_IDX(node_id, gpios, idx)
#define TEST_GPIO_CONFIG_ARRAY(node_id)                                                            \
	{                                                                                          \
		LISTIFY(TEST_GPIO_COUNT, TEST_GPIO_CONFIG_ENTRY, (,), node_id)                     \
	}

static const struct pwm_dt_spec pwms_dt[] = TEST_PWM_CONFIG_ARRAY(DT_PATH(zephyr_user));
static const struct gpio_dt_spec gpios_dt[] = TEST_GPIO_CONFIG_ARRAY(DT_PATH(zephyr_user));

static struct test_context {
	uint32_t last_edge_time;
	uint32_t high_time;
	uint32_t low_time;
	bool sampling_done;
	uint8_t skip_cnt;
} ctx;

static void gpio_edge_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	int pin_state;
	uint32_t current_time = k_cycle_get_32();

	if (ctx.sampling_done || ++ctx.skip_cnt < CONFIG_SKIP_EDGE_NUM) {
		return;
	}

	if (!ctx.last_edge_time) {
		/* init last_edge_time for first delta */
		ctx.last_edge_time = current_time;
		return;
	}

	uint32_t elapsed_time = current_time - ctx.last_edge_time;

	int pin = __builtin_ffs(pins) - 1;

	if (pin >= 0) {
		pin_state = gpio_pin_get(dev, pin);
	} else {
		return;
	}

	if (pin_state) {
		ctx.low_time = elapsed_time;
	} else {
		ctx.high_time = elapsed_time;
	}

	/* sampling is done when both high and low times were stored */
	if (ctx.high_time && ctx.low_time) {
		ctx.sampling_done = true;
	}

	ctx.last_edge_time = current_time;
}

static void setup_edge_detect(void)
{
	ctx.last_edge_time = 0;
	ctx.high_time = 0;
	ctx.low_time = 0;
	ctx.sampling_done = false;
	ctx.skip_cnt = 0;
}

static void config_gpio(const struct gpio_dt_spec *gpio_dt)
{
	/* Configure GPIO pin for edge detection */
	gpio_pin_configure_dt(gpio_dt, GPIO_INPUT);

	gpio_cb.pin_mask = BIT(gpio_dt->pin);

	gpio_init_callback(&gpio_cb, gpio_edge_isr, gpio_cb.pin_mask);
	gpio_add_callback(gpio_dt->port, &gpio_cb);
	gpio_pin_interrupt_configure(gpio_dt->port, gpio_dt->pin, GPIO_INT_EDGE_BOTH);
}

static void unconfig_gpio(const struct gpio_dt_spec *gpio_dt)
{
	/* Disable interrupt for already tested channel */
	gpio_pin_interrupt_configure(gpio_dt->port, gpio_dt->pin, GPIO_INT_DISABLE);

	gpio_cb.pin_mask &= ~BIT(gpio_dt->pin);
}

static bool check_range(float refval, float measval)
{
	float delta = fabsf(refval - measval);
	float allowed_deviation = (refval * (float)CONFIG_ALLOWED_DEVIATION) / 100;

	return delta <= allowed_deviation;
}

static int check_timing(const struct pwm_dt_spec *pwm_dt, const struct gpio_dt_spec *gpio_dt,
			uint8_t duty)
{
	uint64_t cycles_s_sys, cycles_s_pwm;
	int pin_state;
	bool inverted = (pwm_dt->flags & PWM_POLARITY_INVERTED) ? true : false;

	/* reset parameters for edge detection */
	setup_edge_detect();

	/* wait for sampling */
	k_sleep(K_MSEC(CONFIG_SAMPLING_TIME));

	/* store pin state for duty == 100% or 0% checks */
	pin_state = gpio_pin_get_dt(gpio_dt);

	if (inverted) {
		pin_state = !pin_state;
	}

	cycles_s_sys = (uint64_t)sys_clock_hw_cycles_per_sec();
	pwm_get_cycles_per_sec(pwm_dt->dev, pwm_dt->channel, &cycles_s_pwm);

	/* sampling_done should be false for 0 and 100% duty (no switching) */
	TC_PRINT("Sampling done: %s\n", ctx.sampling_done ? "true" : "false");

	if (duty == 100) {
		if ((pin_state == 1) && !ctx.sampling_done) {
			return TC_PASS;
		} else {
			return TC_FAIL;
		}
	} else if (duty == 0) {
		if ((pin_state == 0) && !ctx.sampling_done) {
			return TC_PASS;
		} else {
			return TC_FAIL;
		}
	} else {
		uint32_t measured_period = ctx.high_time + ctx.low_time;
		uint32_t measured_period_ns = (measured_period * 1e9) / cycles_s_sys;
		uint32_t pulse_time = inverted ? ctx.low_time : ctx.high_time;
		float measured_duty = (pulse_time * 100.0f) / measured_period;
		uint32_t measured_duty_2p = (uint32_t)(measured_duty * 100);
		uint32_t period_deviation_2p =
			(uint64_t)10000 * abs(measured_period_ns - pwm_dt->period) / pwm_dt->period;
		uint32_t duty_deviation_2p =
			(uint32_t)10000 * fabs(measured_duty - (float)duty) / duty;

		TC_PRINT("Measured period: %u cycles, high: %u, low: %u [unit: systimer ticks]\n",
			 measured_period, ctx.high_time, ctx.low_time);
		TC_PRINT("Measured period: %u ns, deviation: %d.%d%%\n", measured_period_ns,
			 period_deviation_2p / 100, period_deviation_2p % 100);
		TC_PRINT("Measured duty: %d.%d%%, deviation: %d.%d%%\n", measured_duty_2p / 100,
			 measured_duty_2p % 100, duty_deviation_2p / 100, duty_deviation_2p % 100);

		/* Compare measured values with expected ones */
		if (check_range(measured_period_ns, pwm_dt->period) &&
		    check_range(measured_duty, duty)) {
			TC_PRINT("PWM output matches the programmed values\n");
			return TC_PASS;
		}

		TC_PRINT("PWM output does NOT match the programmed values\n");
		return TC_FAIL;
	}
}

static void test_run(const struct pwm_dt_spec *pwm_dt, const struct gpio_dt_spec *gpio_dt,
		     uint8_t duty, bool set_channel)
{
	int result;
	uint32_t pulse = (uint32_t)((pwm_dt->period * duty) / 100);
	bool inverted = (pwm_dt->flags & PWM_POLARITY_INVERTED) ? true : false;

	TC_PRINT("Test case: [Channel: %" PRIu32 "] [Period: %" PRIu32 "] [Pulse: %" PRIu32
		 "] [Inverted: %s]\n",
		 pwm_dt->channel, pwm_dt->period, pulse, inverted ? "Yes" : "No");

	if (set_channel) {
		result = pwm_set_dt(pwm_dt, pwm_dt->period, pulse);
		zassert_false(result, "Failed on pwm_set() call");
	}

	config_gpio(gpio_dt);

	result = check_timing(pwm_dt, gpio_dt, duty);

	unconfig_gpio(gpio_dt);

	zassert_equal(result, TC_PASS, "Test case failed");
}

ZTEST(pwm_gpio_loopback, test_pwm)
{
	for (int i = 0; i < TEST_PWM_COUNT; i++) {
		zassert_true(device_is_ready(pwms_dt[i].dev), "PWM device is not ready");
		zassert_true(device_is_ready(gpios_dt[i].port), "GPIO device is not ready");

		/* Test case: [Duty: 25%] */
		test_run(&pwms_dt[i], &gpios_dt[i], 25, true);

		/* Test case: [Duty: 100%] */
		test_run(&pwms_dt[i], &gpios_dt[i], 100, true);

		/* Test case: [Duty: 0%] */
		test_run(&pwms_dt[i], &gpios_dt[i], 0, true);

		/* Test case: [Duty: 80%] */
		test_run(&pwms_dt[i], &gpios_dt[i], 80, true);
	}
}

ZTEST(pwm_gpio_loopback, test_pwm_cross)
{
	for (int i = 0; i < TEST_PWM_COUNT; i++) {
		/* Test case: [Duty: 40%] */
		test_run(&pwms_dt[i], &gpios_dt[i], 40, true);
	}

	/* Set all channels and check if they retain the original
	 * configuration without calling pwm_set again
	 */
	for (int i = 0; i < TEST_PWM_COUNT; i++) {
		test_run(&pwms_dt[i], &gpios_dt[i], 40, false);
	}
}

ZTEST_SUITE(pwm_gpio_loopback, NULL, NULL, NULL, NULL, NULL);
