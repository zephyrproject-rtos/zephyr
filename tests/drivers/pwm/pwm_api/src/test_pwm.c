/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Verify PWM can work well when configure through nsec,
 * or cycle.
 *
 * @details
 * - Test Steps
 *   -# Bind PWM_0 port 0.
 *   -# Set PWM period and pulse using pwm_set_cycles() or pwm_set().
 *   -# Use multimeter or other instruments to measure the output
 *	from PWM_OUT_0.
 * - Expected Results
 *   -# The output of PWM_OUT_0 will differ according to the value
 *	of period and pulse.
 *	Always on  ->  Period : Pulse (1 : 1)  ->  3.3V
 *	Half on  ->  Period : Pulse (2 : 1)  ->  1.65V
 *	Always off  ->  Period : Pulse (1 : 0)  ->  0V
 *
 * Note: It is possible to enable timing checks through the symbol
 * CONFIG_ENABLE_TIMING_CHECK. This setting will use GPIO channel
 * pin interrupts and the systimer - assuming it is validated - to
 * do automated measurements.
 */

#include <zephyr/device.h>
#include <inttypes.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#if CONFIG_ENABLE_TIMING_CHECK
#include <zephyr/drivers/gpio.h>
#include <stdlib.h>
#include <math.h>
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(pwm_0))
#define PWM_DEV_NODE DT_ALIAS(pwm_0)
#elif DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(pwm_1))
#define PWM_DEV_NODE DT_ALIAS(pwm_1)
#elif DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(pwm_2))
#define PWM_DEV_NODE DT_ALIAS(pwm_2)
#elif DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(pwm_3))
#define PWM_DEV_NODE DT_ALIAS(pwm_3)

#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pwm)
#define PWM_DEV_NODE DT_INST(0, nordic_nrf_pwm)

#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_pwm)
#define PWM_DEV_NODE DT_INST(0, st_stm32_pwm)

#elif DT_HAS_COMPAT_STATUS_OKAY(xlnx_xps_timer_1_00_a_pwm)
#define PWM_DEV_NODE DT_INST(0, xlnx_xps_timer_1_00_a_pwm)

#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_kinetis_ftm_pwm)
#define PWM_DEV_NODE DT_INST(0, nxp_kinetis_ftm_pwm)

#elif DT_HAS_COMPAT_STATUS_OKAY(intel_blinky_pwm)
#define PWM_DEV_NODE DT_INST(0, intel_blinky_pwm)

#elif DT_HAS_COMPAT_STATUS_OKAY(renesas_ra8_pwm)
#define PWM_DEV_NODE DT_INST(0, renesas_ra8_pwm)

#else
#error "Define a PWM device"
#endif

#if CONFIG_ENABLE_TIMING_CHECK

#define DT_RESOURCES DT_INST(0, test_pwm_api)

#if DT_NODE_HAS_STATUS_OKAY(DT_RESOURCES)
#define GPIO_HDL DT_GPIO_CTLR(DT_RESOURCES, in_gpios)
#define GPIO_DEV DEVICE_DT_GET(GPIO_HDL)
#define GPIO_PIN DT_GPIO_PIN(DT_RESOURCES, in_gpios)
#define GPIO_FLG DT_GPIO_FLAGS(DT_RESOURCES, in_gpios)
#else
#error Invalid device tree config for GPIO input pin
#endif

/* Variables for timing measurement */
static struct test_context {
	uint32_t last_edge_time;
	uint32_t high_time;
	uint32_t low_time;
	bool gpio_cfg_done;
	bool sampling_done;
	uint8_t skip_cnt;
} ctx;

/* Skipping a couple of edges greatly improves measurement precision
 * due to interrupt latency present on the first edge (ref ZEP-868)
 */
#define SKIP_EDGE_NUM 2

#endif /* CONFIG_ENABLE_TIMING_CHECK */

#if defined(CONFIG_BOARD_COLIBRI_IMX7D_MCIMX7D_M4) || defined(CONFIG_SOC_MK64F12) ||               \
	defined(CONFIG_SOC_MKW41Z4)
#define DEFAULT_PERIOD_CYCLE 1024
#define DEFAULT_PULSE_CYCLE  512
#define DEFAULT_PERIOD_NSEC  2000000
#define DEFAULT_PULSE_NSEC   500000
#elif defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3) ||                  \
	defined(CONFIG_SOC_SERIES_ESP32C3)
#define DEFAULT_PERIOD_CYCLE 16200
#define DEFAULT_PULSE_CYCLE  8100
#define DEFAULT_PERIOD_NSEC  160000
#define DEFAULT_PULSE_NSEC   40000
#elif DT_HAS_COMPAT_STATUS_OKAY(intel_blinky_pwm)
#define DEFAULT_PERIOD_CYCLE 32768
#define DEFAULT_PULSE_CYCLE  16384
#define DEFAULT_PERIOD_NSEC  2000000
#define DEFAULT_PULSE_NSEC   500000
#else
#define DEFAULT_PERIOD_CYCLE 64000
#define DEFAULT_PULSE_CYCLE  32000
#define DEFAULT_PERIOD_NSEC  2000000
#define DEFAULT_PULSE_NSEC   1000000
#endif

#if defined CONFIG_BOARD_SAM_E70_XPLAINED
#define DEFAULT_PWM_PORT 2 /* PWM on EXT2 connector, pin 8 */
#elif defined CONFIG_PWM_NRFX
#define DEFAULT_PWM_PORT 0
#define INVALID_PWM_PORT 9
#elif defined CONFIG_BOARD_ADAFRUIT_ITSYBITSY_M4_EXPRESS
#define DEFAULT_PWM_PORT 2 /* TCC1/WO[2] on PA18 (D7) */
#elif defined CONFIG_BOARD_MIMXRT685_EVK
#define DEFAULT_PWM_PORT 7 /* D3 on Arduino connector J27 */
#elif defined(CONFIG_BOARD_LPCXPRESSO55S69_LPC55S69_CPU0_NS) ||                                    \
	defined(CONFIG_BOARD_LPCXPRESSO55S69_LPC55S69_CPU0)
#define DEFAULT_PWM_PORT 2 /* D2 on Arduino connector P18 */
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_pwm)
/* Default port should be adapted per board to fit the channel
 * associated to the PWM pin. For intsance, for following device,
 *      pwm1: pwm {
 *              status = "okay";
 *              pinctrl-0 = <&tim1_ch3_pe13>;
 *      };
 * the following should be used:
 * #define DEFAULT_PWM_PORT 3
 */
#define DEFAULT_PWM_PORT 1
#else
#define DEFAULT_PWM_PORT 0
#endif

#define UNIT_CYCLES 0
#define UNIT_NSECS  1

const struct device *get_pwm_device(void)
{
	return DEVICE_DT_GET(PWM_DEV_NODE);
}

#if CONFIG_ENABLE_TIMING_CHECK

/* Interrupt handler for edge detection */
void edge_detect_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	uint32_t current_time = k_cycle_get_32();

	if (ctx.sampling_done || ++ctx.skip_cnt < SKIP_EDGE_NUM) {
		return;
	}

	if (!ctx.last_edge_time) {
		/* init last_edge_time for first delta*/
		ctx.last_edge_time = current_time;
		return;
	}

	uint32_t elapsed_time = current_time - ctx.last_edge_time;

	int pin_state = gpio_pin_get_raw(GPIO_DEV, GPIO_PIN);

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

void setup_edge_detection(const struct device *gpio_dev, uint32_t pin)
{
	static struct gpio_callback gpio_cb;

	/* Configure GPIO pin for edge detection */
	if (!ctx.gpio_cfg_done) {
		gpio_pin_configure(gpio_dev, pin, (GPIO_INPUT | GPIO_FLG));
		gpio_init_callback(&gpio_cb, edge_detect_handler, BIT(pin));
		gpio_add_callback(gpio_dev, &gpio_cb);
		gpio_pin_interrupt_configure(gpio_dev, pin, GPIO_INT_EDGE_BOTH);

		ctx.gpio_cfg_done = true;
	}

	ctx.last_edge_time = 0;
	ctx.high_time = 0;
	ctx.low_time = 0;
	ctx.sampling_done = false;
	ctx.skip_cnt = 0;
}

bool check_range(float refval, float measval)
{
	float delta = fabsf(refval - measval);
	float allowed_deviation = (refval * (float)CONFIG_ALLOWED_DEVIATION) / 100;

	return delta <= allowed_deviation;
}

static int timing_check(const struct device *pwm_dev, uint32_t port, uint32_t period,
			uint32_t pulse, uint8_t unit)
{
	uint64_t cycles_s_sys, cycles_s_pwm;
	uint32_t expected_period_ns;
	uint32_t expected_pulse_ns;
	uint32_t expected_duty_cycle;
	int pin_state;

	/* wait for stable signal */
	k_sleep(K_MSEC(100));

	/* set up GPIO input pin for edge detection */
	setup_edge_detection(GPIO_DEV, GPIO_PIN);

	/* wait for sampling */
	k_sleep(K_MSEC(100));

	/* store pin state for duty == 100% or 0% checks */
	pin_state = gpio_pin_get_raw(GPIO_DEV, GPIO_PIN);

	cycles_s_sys = (uint64_t)sys_clock_hw_cycles_per_sec();
	pwm_get_cycles_per_sec(pwm_dev, port, &cycles_s_pwm);

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
		float measured_duty_cycle = (ctx.high_time * 100.0f) / measured_period;
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

#endif

static int test_task(uint32_t port, uint32_t period, uint32_t pulse, uint8_t unit)
{
	TC_PRINT("[PWM]: %" PRIu8 ", [period]: %" PRIu32 ", [pulse]: %" PRIu32 "\n", port, period,
		 pulse);

	const struct device *pwm_dev = get_pwm_device();

	if (!device_is_ready(pwm_dev)) {
		TC_PRINT("PWM device is not ready\n");
		return TC_FAIL;
	}

	if (unit == UNIT_CYCLES) {
		/* Verify pwm_set_cycles() */
		if (pwm_set_cycles(pwm_dev, port, period, pulse, 0)) {
			TC_PRINT("Fail to set the period and pulse width\n");
			return TC_FAIL;
		}
	} else { /* unit == UNIT_NSECS */
		/* Verify pwm_set() */
		if (pwm_set(pwm_dev, port, period, pulse, 0)) {
			TC_PRINT("Fail to set the period and pulse width\n");
			return TC_FAIL;
		}
	}

#if CONFIG_ENABLE_TIMING_CHECK
	return timing_check(pwm_dev, port, period, pulse, unit);
#else
	return TC_PASS;
#endif
}

ZTEST_USER(pwm_basic, test_pwm_nsec)
{
	/* Period : Pulse (2000000 : 1000000), unit (nsec). Voltage : 1.65V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_NSEC,
				DEFAULT_PULSE_NSEC, UNIT_NSECS) == TC_PASS, NULL);
	k_sleep(K_MSEC(CONFIG_TEST_DELAY));

	/* Period : Pulse (2000000 : 2000000), unit (nsec). Voltage : 3.3V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_NSEC,
				DEFAULT_PERIOD_NSEC, UNIT_NSECS) == TC_PASS, NULL);
	k_sleep(K_MSEC(CONFIG_TEST_DELAY));

	/* Period : Pulse (2000000 : 0), unit (nsec). Voltage : 0V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_NSEC,
				0, UNIT_NSECS) == TC_PASS, NULL);
	k_sleep(K_MSEC(CONFIG_TEST_DELAY));
}

ZTEST_USER(pwm_basic, test_pwm_cycle)
{
	/* Period : Pulse (64000 : 32000), unit (cycle). Voltage : 1.65V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_CYCLE,
				DEFAULT_PULSE_CYCLE, UNIT_CYCLES) == TC_PASS, NULL);
	k_sleep(K_MSEC(CONFIG_TEST_DELAY));

	/* Period : Pulse (64000 : 64000), unit (cycle). Voltage : 3.3V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_CYCLE,
				DEFAULT_PERIOD_CYCLE, UNIT_CYCLES) == TC_PASS, NULL);
	k_sleep(K_MSEC(CONFIG_TEST_DELAY));

	/* Period : Pulse (64000 : 0), unit (cycle). Voltage : 0V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_CYCLE,
				0, UNIT_CYCLES) == TC_PASS, NULL);
	k_sleep(K_MSEC(CONFIG_TEST_DELAY));
}

#if defined INVALID_PWM_PORT
ZTEST_USER(pwm_basic, test_pwm_invalid_port)
{
	const struct device *pwm_dev = get_pwm_device();

	TC_PRINT("[PWM]: %" PRIu8 ", [period]: %" PRIu32 ", [pulse]: %" PRIu32 "\n",
		 INVALID_PWM_PORT, DEFAULT_PERIOD_CYCLE, DEFAULT_PULSE_CYCLE);

	zassert_true(device_is_ready(pwm_dev), "PWM device is not ready");

	zassert_equal(pwm_set_cycles(pwm_dev, INVALID_PWM_PORT, DEFAULT_PERIOD_CYCLE,
				     DEFAULT_PULSE_CYCLE, 0),
		      -EINVAL, "Invalid PWM port\n");
}
#endif
