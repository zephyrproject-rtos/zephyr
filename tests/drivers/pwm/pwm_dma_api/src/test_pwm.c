/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Verify PWM can work well with values fed with DMA from
 * a table with both nsec and cycle values.
 *
 * @details
 * - Test Steps
 *   -# Set PWM period and pulse with DMA using pwm_set_cycles_dma()
 *      or pwm_set_dma().
 * - Expected Results
 *   -# The led on the board shall change its brightness peridically
 *      (breathing) fast for 5s than slower
 */

#include <zephyr/device.h>
#include <inttypes.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(pwm_test))
#define PWM_DEV_NODE DT_ALIAS(pwm_test)
#else
#error "Test requires a pwm-test alias in DTS"
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(dma_test))
#define DMA_DEV_NODE DT_ALIAS(dma_test)
#else
#error "Test requires a dma-test alias in DTS"
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

#define UNIT_CYCLES	0
#define UNIT_NSECS	1

uint32_t pwm_table[CONFIG_DEFAULT_STEP_NUMBER + 1];

const struct device *get_pwm_device(void)
{
	return DEVICE_DT_GET(PWM_DEV_NODE);
}

const struct device *get_dma_device(void)
{
	return DEVICE_DT_GET(DMA_DEV_NODE);
}

static int test_task(uint32_t port, uint32_t period, uint32_t pulse, uint8_t unit)
{
	int num_steps = CONFIG_DEFAULT_STEP_NUMBER;

	TC_PRINT("[PWM]: %" PRIu8 ", [period]: %" PRIu32 ", [steps]: %" PRIu32 "\n",
		port, period, num_steps);

	/* create a dma table with intermediate values */		
	uint32_t step = period / num_steps;
	pwm_table[0] = 0;
	for(int i=1; i <= num_steps; i++) {
		pwm_table[i] = i * step;
	}

	const struct device *pwm_dev = get_pwm_device();

	if (!device_is_ready(pwm_dev)) {
		TC_PRINT("PWM device is not ready\n");
		return TC_FAIL;
	}

	if (unit == UNIT_CYCLES) {
		/* Verify pwm_set_cycles_dma() */
		if(pwm_set_cycles_dma(pwm_dev, port, period, pwm_table, num_steps+1, PWM_POLARITY_NORMAL)) {
			TC_PRINT("Fail to set the period and pulse width\n");
			return TC_FAIL;
		}
	} else { /* unit == UNIT_NSECS */
		/* Verify pwm_set_dma() */
		TC_PRINT("WTF\n");
		if(pwm_set_dma(pwm_dev, port, period, pwm_table, num_steps+1, PWM_POLARITY_NORMAL)) {
			TC_PRINT("Fail to set the period and pulse width\n");
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

ZTEST(pwm_dma, test_pwm_dma_nsec)
{
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_NSEC,
				DEFAULT_PERIOD_NSEC/3, UNIT_NSECS) == TC_PASS);
	k_sleep(K_MSEC(1000));
}

ZTEST(pwm_dma, test_pwm_dma_cycle)
{
	zassert_true(test_task(DEFAULT_PWM_PORT, DEFAULT_PERIOD_CYCLE,
				DEFAULT_PERIOD_CYCLE/2, UNIT_CYCLES) == TC_PASS);
	k_sleep(K_MSEC(5000));
}
