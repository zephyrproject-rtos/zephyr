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
#include <zephyr/zephyr.h>
#include <ztest.h>

#if DT_NODE_HAS_STATUS(DT_ALIAS(pwm_0), okay)
#define PWM_DEV_NODE DT_ALIAS(pwm_0)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(pwm_1), okay)
#define PWM_DEV_NODE DT_ALIAS(pwm_1)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(pwm_2), okay)
#define PWM_DEV_NODE DT_ALIAS(pwm_2)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(pwm_3), okay)
#define PWM_DEV_NODE DT_ALIAS(pwm_3)

#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_pwm)
#define PWM_DEV_NODE DT_INST(0, st_stm32_pwm)

#elif DT_HAS_COMPAT_STATUS_OKAY(xlnx_xps_timer_1_00_a_pwm)
#define PWM_DEV_NODE DT_INST(0, xlnx_xps_timer_1_00_a_pwm)

#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_kinetis_ftm_pwm)
#define PWM_DEV_NODE DT_INST(0, nxp_kinetis_ftm_pwm)

#else
#error "Define a PWM device"
#endif

#if defined(CONFIG_BOARD_COLIBRI_IMX7D_M4) || defined(CONFIG_SOC_MK64F12) || \
	defined(CONFIG_SOC_MKW41Z4)
#define DEFAULT_PERIOD_CYCLE 1024
#define DEFAULT_PULSE_CYCLE 512
#define DEFAULT_PERIOD_NSEC 2000000
#define DEFAULT_PULSE_NSEC 500000
#else
#define DEFAULT_PERIOD_CYCLE 64000
#define DEFAULT_PULSE_CYCLE 32000
#define DEFAULT_PERIOD_NSEC 2000000
#define DEFAULT_PULSE_NSEC 1000000
#endif

#if defined CONFIG_BOARD_SAM_E70_XPLAINED
#define DEFAULT_PWM_PORT 2 /* PWM on EXT2 connector, pin 8 */
#elif defined CONFIG_PWM_NRFX
#define DEFAULT_PWM_PORT DT_PROP(DT_ALIAS(pwm_0), ch0_pin)
#elif defined CONFIG_BOARD_ADAFRUIT_ITSYBITSY_M4_EXPRESS
#define DEFAULT_PWM_PORT 2 /* TCC1/WO[2] on PA18 (D7) */
#elif defined CONFIG_BOARD_MIMXRT685_EVK
#define DEFAULT_PWM_PORT 7 /* D3 on Arduino connector J27 */
#elif defined CONFIG_BOARD_LPCXPRESSO55S69_CPU0
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
