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
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/gpio.h>
#include <stdlib.h>
#include <math.h>

static const struct device *pwm_dev = DEVICE_DT_GET(DT_ALIAS(pwm));
static const struct gpio_dt_spec gpio_dt = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), in_gpios);
static struct gpio_callback gpio_cb;

static struct test_context {
	uint32_t last_edge_time;
	uint32_t high_time;
	uint32_t low_time;
	bool sampling_done;
	uint8_t skip_cnt;
} ctx;

#define DEFAULT_PWM_PORT 0

#define UNIT_CYCLES 0
#define UNIT_NSECS  1

static void gpio_edge_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
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

	int pin_state = gpio_pin_get_dt(&gpio_dt);

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

static bool check_range(float refval, float measval)
{
	float delta = fabsf(refval - measval);
	float allowed_deviation = (refval * (float)CONFIG_ALLOWED_DEVIATION) / 100;

	return delta <= allowed_deviation;
}

static int check_timing(uint32_t period, uint32_t pulse, uint8_t unit, bool inverted)
{
	uint64_t cycles_s_sys, cycles_s_pwm;
	uint32_t expected_period_ns;
	uint32_t expected_pulse_ns;
	uint32_t expected_duty_cycle;
	int pin_state;

	/* reset parameters for edge detection */
	setup_edge_detect();

	/* wait for sampling */
	k_sleep(K_MSEC(CONFIG_SAMPLING_TIME));

	/* store pin state for duty == 100% or 0% checks */
	pin_state = gpio_pin_get_dt(&gpio_dt);

	if (inverted) {
		pin_state = !pin_state;
	}

	cycles_s_sys = (uint64_t)sys_clock_hw_cycles_per_sec();
	pwm_get_cycles_per_sec(pwm_dev, DEFAULT_PWM_PORT, &cycles_s_pwm);

	/* limit pulse to period length */
	if (pulse > period) {
		pulse = period;
	}

	if (unit == UNIT_CYCLES) {
		/* convert cycles to ns using PWM clock period */
		expected_period_ns = (period * 1e9) / cycles_s_pwm;
		expected_pulse_ns = (pulse * 1e9) / cycles_s_pwm;
	} else if (unit == UNIT_NSECS) {
		/* already in nanoseconds */
		expected_period_ns = period;
		expected_pulse_ns = pulse;
	} else {
		TC_PRINT("Unexpected unit");
		return TC_FAIL;
	}

	/* sampling_done should be false for 0 and 100% duty (no switching) */
	TC_PRINT("Sampling done: %s\n", ctx.sampling_done ? "true" : "false");

	expected_duty_cycle = (expected_pulse_ns * 100) / expected_period_ns;

	if (expected_duty_cycle == 100) {
		if ((pin_state == 1) && !ctx.sampling_done) {
			return TC_PASS;
		} else {
			return TC_FAIL;
		}
	} else if (expected_duty_cycle == 0) {
		if ((pin_state == 0) && !ctx.sampling_done) {
			return TC_PASS;
		} else {
			return TC_FAIL;
		}
	} else {
		uint32_t measured_period = ctx.high_time + ctx.low_time;
		uint32_t measured_period_ns = (measured_period * 1e9) / cycles_s_sys;
		uint32_t pulse_time = inverted ? ctx.low_time : ctx.high_time;
		float measured_duty_cycle = (pulse_time * 100.0f) / measured_period;
		uint32_t measured_duty_cycle_2p = (uint32_t)(measured_duty_cycle * 100);
		uint32_t period_deviation_2p = (uint64_t)10000 *
					       abs(measured_period_ns - expected_period_ns) /
					       expected_period_ns;
		uint32_t duty_deviation_2p =
			(uint32_t)10000 * fabs(measured_duty_cycle - (float)expected_duty_cycle) /
			expected_duty_cycle;

		TC_PRINT("Expected period: %u ns, pulse: %u ns duty cycle: %u%%\n",
			 expected_period_ns, expected_pulse_ns, expected_duty_cycle);
		TC_PRINT("Measured period: %u cycles, high: %u, low: %u [unit: systimer ticks]\n",
			 measured_period, ctx.high_time, ctx.low_time);
		TC_PRINT("Measured period: %u ns, deviation: %d.%d%%\n", measured_period_ns,
			 period_deviation_2p / 100, period_deviation_2p % 100);
		TC_PRINT("Measured duty: %d.%d%%, deviation: %d.%d%%\n",
			 measured_duty_cycle_2p / 100, measured_duty_cycle_2p % 100,
			 duty_deviation_2p / 100, duty_deviation_2p % 100);

		/* Compare measured values with expected ones */
		if (check_range(measured_period_ns, expected_period_ns) &&
		    check_range(measured_duty_cycle, expected_duty_cycle)) {
			TC_PRINT("PWM output matches the programmed values\n");
			return TC_PASS;
		}

		TC_PRINT("PWM output does NOT match the programmed values\n");
		return TC_FAIL;
	}
}

static void test_run(uint32_t period, uint32_t pulse, uint8_t unit, bool inverted)
{
	int result;
	pwm_flags_t flags = inverted ? PWM_POLARITY_INVERTED : 0;

	TC_PRINT("Test case: [Period: %" PRIu32 "] [Pulse: %" PRIu32
		 "] [Unit: %s] [Inverted: %s]\n",
		 period, pulse, (unit == UNIT_CYCLES) ? "cycles" : "nsec", inverted ? "Yes" : "No");

	if (unit == UNIT_CYCLES) {
		result = pwm_set_cycles(pwm_dev, DEFAULT_PWM_PORT, period, pulse, flags);
		zassert_false(result, "Failed on pwm_set_cycles() call");
	} else {
		result = pwm_set(pwm_dev, DEFAULT_PWM_PORT, period, pulse, flags);
		zassert_false(result, "Failed on pwm_set() call");
	}

	result = check_timing(period, pulse, unit, inverted);

	zassert_equal(result, TC_PASS, "Test case failed");
}

ZTEST(pwm_gpio_loopback, test_pwm_nsec)
{
	/* Test case: [Duty: 25%] [Unit: nsec] [Inverted: No] */
	test_run(CONFIG_PERIOD_NSEC, CONFIG_PULSE_NSEC, UNIT_NSECS, false);

	/* Test case: [Duty: 100%] [Unit: nsec] [Inverted: No] */
	test_run(CONFIG_PERIOD_NSEC, CONFIG_PERIOD_NSEC, UNIT_NSECS, false);

	/* Test case: [Duty: 0%] [Unit: nsec] [Inverted: No] */
	test_run(CONFIG_PERIOD_NSEC, 0, UNIT_NSECS, false);

	/* Test case: [Duty: 25%] [Unit: nsec] [Inverted: No] */
	test_run(CONFIG_PERIOD_NSEC, CONFIG_PULSE_NSEC, UNIT_NSECS, false);
}

ZTEST(pwm_gpio_loopback, test_pwm_nsec_inverted)
{
	/* Test case: [Duty: 25%] [Unit: nsec] [Inverted: Yes] */
	test_run(CONFIG_PERIOD_NSEC, CONFIG_PULSE_NSEC, UNIT_NSECS, true);

	/* Test case: [Duty: 100%] [Unit: nsec] [Inverted: Yes] */
	test_run(CONFIG_PERIOD_NSEC, CONFIG_PERIOD_NSEC, UNIT_NSECS, true);

	/* Test case: [Duty: 0%] [Unit: nsec] [Inverted: Yes] */
	test_run(CONFIG_PERIOD_NSEC, 0, UNIT_NSECS, true);

	/* Test case: [Duty: 25%] [Unit: nsec] [Inverted: Yes] */
	test_run(CONFIG_PERIOD_NSEC, CONFIG_PULSE_NSEC, UNIT_NSECS, true);
}

ZTEST(pwm_gpio_loopback, test_pwm_cycle)
{
	/* Test case: [Duty: 50%] [Unit: cycle] [Inverted: No] */
	test_run(CONFIG_PERIOD_CYCLES, CONFIG_PULSE_CYCLES, UNIT_CYCLES, false);

	/* Test case: [Duty: 100%] [Unit: cycle] [Inverted: No] */
	test_run(CONFIG_PERIOD_CYCLES, CONFIG_PERIOD_CYCLES, UNIT_CYCLES, false);

	/* Test case: [Duty: 0%] [Unit: cycle] [Inverted: No] */
	test_run(CONFIG_PERIOD_CYCLES, 0, UNIT_CYCLES, false);

	/* Test case: [Duty: 50%] [Unit: cycle] [Inverted: No] */
	test_run(CONFIG_PERIOD_CYCLES, CONFIG_PULSE_CYCLES, UNIT_CYCLES, false);
}

static void *pwm_gpio_loopback_setup(void)
{
	zassert_true(device_is_ready(pwm_dev), "PWM device is not ready");

	/* Configure GPIO pin for edge detection */
	gpio_pin_configure_dt(&gpio_dt, GPIO_INPUT);
	gpio_init_callback(&gpio_cb, gpio_edge_isr, BIT(gpio_dt.pin));
	gpio_add_callback(gpio_dt.port, &gpio_cb);
	gpio_pin_interrupt_configure(gpio_dt.port, gpio_dt.pin, GPIO_INT_EDGE_BOTH);

	return NULL;
}

ZTEST_SUITE(pwm_gpio_loopback, NULL, pwm_gpio_loopback_setup, NULL, NULL, NULL);
