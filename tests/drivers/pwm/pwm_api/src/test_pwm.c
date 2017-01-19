/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup test_pwm_basic_operations
 * @{
 * @defgroup t_pwm_basic_operations test_pwm_sample
 * @brief TestPurpose: verify PWM can work well when configure
 *			through usec or cycle.
 * @details
 * - Test Steps
 *   -# Bind PWM port 0.
 *   -# Set PWM period and pulse using pwm_pin_set_cycles() or
 *	pwm_pin_set_usec().
 *   -# Use multimeter or other instruments to measure the output
 *	from PWM_OUT_0.
 * - Expected Results
 *   -# The output of PWM_OUT_0 will differ according to the value
 *	of period and pulse.
 *	Always on  ->  Period : Pulse (1 : 1)  ->  3.3V
 *	Half on  ->  Period : Pulse (2 : 1)  ->  1.65V
 *	Always off  ->  Period : Pulse (1 : 0)  ->  0V
 * @}
 */

#include <device.h>
#include <inttypes.h>
#include <pwm.h>
#include <zephyr.h>
#include <ztest.h>

#define PWM_DEV_NAME CONFIG_PWM_QMSI_DEV_NAME

#define DEFAULT_PWM_PORT 0
#define DEFAULT_PERIOD_CYCLE 64000
#define DEFAULT_PULSE_CYCLE 32000
#define DEFAULT_PERIOD_USEC 2000
#define DEFAULT_PULSE_USEC 1000

static int test_task(uint32_t port, uint32_t period, uint32_t pulse, bool cycle)
{
	TC_PRINT("[PWM]: %" PRIu8 ", [period]: %" PRIu32 ", [pulse]: %" PRIu32 "\n",
		port, period, pulse);

	struct device *pwm_dev = device_get_binding(PWM_DEV_NAME);

	if (!pwm_dev) {
		TC_PRINT("Cannot get PWM device\n");
		return TC_FAIL;
	}

	if (cycle) {
		/* Verify pwm_pin_set_cycles() */
		if (pwm_pin_set_cycles(pwm_dev, port, period, pulse)) {
			TC_PRINT("Fail to set the period and pulse width\n");
			return TC_FAIL;
		}
	} else {
		/* Verify pwm_pin_set_usec() */
		if (pwm_pin_set_usec(pwm_dev, port, period, pulse)) {
			TC_PRINT("Fail to set the period and pulse width\n");
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

void test_pwm_usec(void)
{
	/* Period : Pulse (2000 : 1000), unit (usec). Voltage : 1.65V */
	assert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_USEC,
				DEFAULT_PULSE_USEC, false) == TC_PASS, NULL);
	k_sleep(1000);

	/* Period : Pulse (2000 : 2000), unit (usec). Voltage : 3.3V */
	assert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_USEC,
				DEFAULT_PERIOD_USEC, false) == TC_PASS, NULL);
	k_sleep(1000);

	/* Period : Pulse (2000 : 0), unit (usec). Voltage : 0V */
	assert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_USEC,
				0, false) == TC_PASS, NULL);
	k_sleep(1000);
}

void test_pwm_cycle(void)
{
	/* Period : Pulse (64000 : 32000), unit (cycle). Voltage : 1.65V */
	assert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_CYCLE,
				DEFAULT_PULSE_CYCLE, true) == TC_PASS, NULL);
	k_sleep(1000);

	/* Period : Pulse (64000 : 64000), unit (cycle). Voltage : 3.3V */
	assert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_CYCLE,
				DEFAULT_PERIOD_CYCLE, true) == TC_PASS, NULL);
	k_sleep(1000);

	/* Period : Pulse (64000 : 0), unit (cycle). Voltage : 0V */
	assert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_CYCLE,
				0, true) == TC_PASS, NULL);
}
