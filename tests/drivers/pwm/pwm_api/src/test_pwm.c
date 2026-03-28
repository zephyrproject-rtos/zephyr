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
 */

#include <zephyr/device.h>
#include <inttypes.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(pwm_test))
#define PWM_DEV_NODE DT_ALIAS(pwm_test)
#else
#error "Test requires a pwm-test alias in DTS"
#endif

#define DEFAULT_PERIOD_CYCLE CONFIG_DEFAULT_PERIOD_CYCLE
#define DEFAULT_PULSE_CYCLE  CONFIG_DEFAULT_PULSE_CYCLE
#define DEFAULT_PERIOD_NSEC  CONFIG_DEFAULT_PERIOD_NSEC
#define DEFAULT_PULSE_NSEC   CONFIG_DEFAULT_PULSE_NSEC

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_fake_pwm)
#include <zephyr/fff.h>
DEFINE_FFF_GLOBALS;
#endif

#define DEFAULT_PWM_PORT CONFIG_DEFAULT_PWM_PORT
#define INVALID_PWM_PORT CONFIG_INVALID_PWM_PORT

#define UNIT_CYCLES	0
#define UNIT_NSECS	1

const struct device *get_pwm_device(void)
{
	return DEVICE_DT_GET(PWM_DEV_NODE);
}

static int test_task(uint32_t port, uint32_t period, uint32_t pulse, uint8_t unit)
{
	TC_PRINT("[PWM]: %" PRIu8 ", [period]: %" PRIu32 ", [pulse]: %" PRIu32 "\n",
		port, period, pulse);

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

	return TC_PASS;
}

ZTEST_USER(pwm_basic, test_pwm_nsec)
{
	/* Period : Pulse (2000000 : 1000000), unit (nsec). Voltage : 1.65V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_NSEC,
				DEFAULT_PULSE_NSEC, UNIT_NSECS) == TC_PASS);
	k_sleep(K_MSEC(1000));

	/* Period : Pulse (2000000 : 2000000), unit (nsec). Voltage : 3.3V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_NSEC,
				DEFAULT_PERIOD_NSEC, UNIT_NSECS) == TC_PASS);
	k_sleep(K_MSEC(1000));

	/* Period : Pulse (2000000 : 0), unit (nsec). Voltage : 0V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_NSEC,
				0, UNIT_NSECS) == TC_PASS);
	k_sleep(K_MSEC(1000));
}

ZTEST_USER(pwm_basic, test_pwm_cycle)
{
	/* Period : Pulse (64000 : 32000), unit (cycle). Voltage : 1.65V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_CYCLE,
				DEFAULT_PULSE_CYCLE, UNIT_CYCLES) == TC_PASS);
	k_sleep(K_MSEC(1000));

	/* Period : Pulse (64000 : 64000), unit (cycle). Voltage : 3.3V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_CYCLE,
				DEFAULT_PERIOD_CYCLE, UNIT_CYCLES) == TC_PASS);
	k_sleep(K_MSEC(1000));

	/* Period : Pulse (64000 : 0), unit (cycle). Voltage : 0V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_CYCLE,
				0, UNIT_CYCLES) == TC_PASS);
	k_sleep(K_MSEC(1000));
}

#if (CONFIG_INVALID_PWM_PORT >= 0)
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
