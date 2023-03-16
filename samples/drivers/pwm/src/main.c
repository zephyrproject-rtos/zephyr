/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample Application for PWM
 * This example demonstrates the following
 * a. Derive the clock rate for the PWM device
 * b. Generate the PWM output by setting the period and pulse width
 *    in nanoseconds
 * c. Generate the PWM output by setting the period and pulse width
 *    in terms of clock cycles
 * @{
 */

/* Local Includes */
#include <zephyr/sys/util.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PWM_LABEL   DT_NODELABEL(pwm0)
#define PWM_CHANNEL 1

#define PERIOD_USEC 100000000 /* 100 ms */
#define PULSE_USEC  20000000  /* 20% duty cycle */

#define PERIOD_HW 8192 /* Period in hw cycles */
#define PULSE_HW  4096 /* 50% Duty in hw cycles */

/* @brief main function
 * PWM Sample Application
 * The sample app tests the PWM IP
 * The user is required to connect a probing device like oscilloscope on
 * PWM_CHANNEL pin on PWM_LABEL dev to check the PWM output.
 */
void main(void)
{
	const struct device *pwm_dev;
	uint64_t cl;
	int ret;
	uint8_t pwm_flag = 0;

	/* Get the device handle for PWM instance  */
	pwm_dev = DEVICE_DT_GET(PWM_LABEL);
	if (!device_is_ready(pwm_dev)) {
		printk("[PWM] Bind failed\n");
		return;
	}
	printk("[PWM] Bind Success\n");

	/* Gets the clock rate(cycles per second) for the PWM device */
	ret = pwm_get_cycles_per_sec(pwm_dev, 0, &cl);
	if (ret != 0) {
		printk("[PWM] Get clock rate failed\n");
		return;
	}
	printk("[PWM] Running rate: %lld\n", cl);

	/* Set the period and pulse width of the PWM output in
	 * micro seconds. Expected output-Pulse with 50% duty cycle
	 */
	printk("[PWM] Generating pulses-Period: %d nsec, Pulse: %d nsec\n", PERIOD_USEC,
	       PULSE_USEC);
	ret = pwm_set(pwm_dev, PWM_CHANNEL, PERIOD_USEC, PULSE_USEC, pwm_flag);
	if (ret != 0) {
		printk("[PWM] Pin set nsec failed\n");
		return;
	}

	k_sleep(K_SECONDS(10));

	printk("[PWM] Generating pulses-Period: %d cycles, Pulse: %d cycles\n", PERIOD_HW,
	       PULSE_HW);
	ret = pwm_set_cycles(pwm_dev, PWM_CHANNEL, PERIOD_HW, PULSE_HW, pwm_flag);
	if (ret != 0) {
		printk("[PWM] Pin set cycles failed\n");
		return;
	}

	while (1) {
		k_sleep(K_MSEC(100));
	}
}
