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
 *			through usec, nsec, or cycle.
 * @details
 * - Test Steps
 *   -# Bind PWM_0 port 0.
 *   -# Set PWM period and pulse using pwm_pin_set_cycles(),
 *	pwm_pin_set_usec(), or pwm_pin_set_nsec().
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
#include <drivers/pwm.h>
#include <zephyr.h>
#include <ztest.h>

#if defined(DT_ALIAS_PWM_0_LABEL)
#define PWM_DEV_NAME DT_ALIAS_PWM_0_LABEL
#elif defined(DT_ALIAS_PWM_1_LABEL)
#define PWM_DEV_NAME DT_ALIAS_PWM_1_LABEL
#elif defined(DT_ALIAS_PWM_2_LABEL)
#define PWM_DEV_NAME DT_ALIAS_PWM_2_LABEL
#elif defined(DT_ALIAS_PWM_3_LABEL)
#define PWM_DEV_NAME DT_ALIAS_PWM_3_LABEL
#else
#error "Define a PWM device"
#endif

#if defined(CONFIG_BOARD_COLIBRI_IMX7D_M4) || defined(CONFIG_SOC_MK64F12)
#define DEFAULT_PERIOD_CYCLE 1024
#define DEFAULT_PULSE_CYCLE 512
#define DEFAULT_PERIOD_USEC 2000
#define DEFAULT_PULSE_USEC 500
#define DEFAULT_PERIOD_NSEC 2000000
#define DEFAULT_PULSE_NSEC 500000
#else
#define DEFAULT_PERIOD_CYCLE 64000
#define DEFAULT_PULSE_CYCLE 32000
#define DEFAULT_PERIOD_USEC 2000
#define DEFAULT_PULSE_USEC 1000
#define DEFAULT_PERIOD_NSEC 2000000
#define DEFAULT_PULSE_NSEC 1000000
#endif

#if defined CONFIG_BOARD_SAM_E70_XPLAINED
#define DEFAULT_PWM_PORT 2 /* PWM on EXT2 connector, pin 8 */
#else
#define DEFAULT_PWM_PORT 0
#endif

#define UNIT_CYCLES	0
#define UNIT_USECS	1
#define UNIT_NSECS	2

static int test_task(u32_t port, u32_t period, u32_t pulse, u8_t unit)
{
	TC_PRINT("[PWM]: %" PRIu8 ", [period]: %" PRIu32 ", [pulse]: %" PRIu32 "\n",
		port, period, pulse);

	struct device *pwm_dev = device_get_binding(PWM_DEV_NAME);

	if (!pwm_dev) {
		TC_PRINT("Cannot get PWM device\n");
		return TC_FAIL;
	}

	if (unit == UNIT_CYCLES) {
		/* Verify pwm_pin_set_cycles() */
		if (pwm_pin_set_cycles(pwm_dev, port, period, pulse)) {
			TC_PRINT("Fail to set the period and pulse width\n");
			return TC_FAIL;
		}
	} else if (unit == UNIT_USECS) {
		/* Verify pwm_pin_set_usec() */
		if (pwm_pin_set_usec(pwm_dev, port, period, pulse)) {
			TC_PRINT("Fail to set the period and pulse width\n");
			return TC_FAIL;
		}
	} else { /* unit == UNIT_NSECS */
		/* Verify pwm_pin_set_nsec() */
		if (pwm_pin_set_nsec(pwm_dev, port, period, pulse)) {
			TC_PRINT("Fail to set the period and pulse width\n");
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

void test_pwm_usec(void)
{
	/* Period : Pulse (2000 : 1000), unit (usec). Voltage : 1.65V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_USEC,
				DEFAULT_PULSE_USEC, UNIT_USECS) == TC_PASS, NULL);
	k_sleep(K_MSEC(1000));

	/* Period : Pulse (2000 : 2000), unit (usec). Voltage : 3.3V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_USEC,
				DEFAULT_PERIOD_USEC, UNIT_USECS) == TC_PASS, NULL);
	k_sleep(K_MSEC(1000));

	/* Period : Pulse (2000 : 0), unit (usec). Voltage : 0V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_USEC,
				0, UNIT_USECS) == TC_PASS, NULL);
	k_sleep(K_MSEC(1000));
}

void test_pwm_nsec(void)
{
	/* Period : Pulse (2000000 : 1000000), unit (nsec). Voltage : 1.65V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_NSEC,
				DEFAULT_PULSE_NSEC, UNIT_NSECS) == TC_PASS, NULL);
	k_sleep(K_MSEC(1000));

	/* Period : Pulse (2000000 : 2000000), unit (nsec). Voltage : 3.3V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_NSEC,
				DEFAULT_PERIOD_NSEC, UNIT_NSECS) == TC_PASS, NULL);
	k_sleep(K_MSEC(1000));

	/* Period : Pulse (2000000 : 0), unit (nsec). Voltage : 0V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_NSEC,
				0, UNIT_NSECS) == TC_PASS, NULL);
	k_sleep(K_MSEC(1000));
}

void test_pwm_cycle(void)
{
	/* Period : Pulse (64000 : 32000), unit (cycle). Voltage : 1.65V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_CYCLE,
				DEFAULT_PULSE_CYCLE, UNIT_CYCLES) == TC_PASS, NULL);
	k_sleep(K_MSEC(1000));

	/* Period : Pulse (64000 : 64000), unit (cycle). Voltage : 3.3V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_CYCLE,
				DEFAULT_PERIOD_CYCLE, UNIT_CYCLES) == TC_PASS, NULL);
	k_sleep(K_MSEC(1000));

	/* Period : Pulse (64000 : 0), unit (cycle). Voltage : 0V */
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_CYCLE,
				0, UNIT_CYCLES) == TC_PASS, NULL);
}
