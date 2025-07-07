/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/misc/interconn/renesas_elc/renesas_elc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

/* Define DT aliases for PWM and ELC devices */
#define PWM_GEN_NODE  DT_ALIAS(pwm_gen)
#define PWM_CAP_NODE  DT_ALIAS(pwm_cap)
#define ELC_LINK_NODE DT_ALIAS(elc_link)

/* PWM channel definitions for generator and capture */
#define PWM_GEN_CHANNEL 0
#define PWM_CAP_CHANNEL 0

/* PWM generator software events:
 * - Software Event 0 starts the PWM generator.
 * - Software Event 1 stops the PWM generator.
 */
#define PWM_GEN_EVENT_START FSP_SIGNAL_ELC_SOFTWARE_EVENT_0
#define PWM_GEN_EVENT_STOP  FSP_SIGNAL_ELC_SOFTWARE_EVENT_1

/* Set the PWM period to 1,000,000 nanoseconds (1 kHz) */
#define PWM_PERIOD_NSEC 1000000U

#define WITHIN(a, b, d) ((a) >= ((b) - (d))) && ((a) <= ((b) + (d)))

/**
 * @brief Sample Renesas ELC device functionality.
 *
 * This sample verifies the functionality of the Renesas ELC device.
 * It obtains the required devices from the device tree, configures a PWM
 * generator, captures its signal, stops and restarts the generator via
 * the ELC device, and verifies PWM operation via PWM capture.
 *
 * @return 0 on success.
 */
int main(void)
{
	int ret;
	const struct device *pwm_gen_dev;
	const struct device *pwm_cap_dev;
	const struct device *elc_dev;

	/*
	 * Obtain device pointers using DEVICE_DT_GET():
	 * - pwm_gen_dev: PWM generator from PWM_GEN_NODE.
	 * - pwm_cap_dev: PWM capture from PWM_CAP_NODE.
	 * - elc_dev:     ELC device from ELC_LINK_NODE.
	 *
	 * Check that each device is ready with device_is_ready().
	 * If a device is not ready, print an error and return.
	 * Otherwise, print device names for confirmation.
	 */
	pwm_gen_dev = DEVICE_DT_GET(PWM_GEN_NODE);
	pwm_cap_dev = DEVICE_DT_GET(PWM_CAP_NODE);
	elc_dev = DEVICE_DT_GET(ELC_LINK_NODE);

	if (!device_is_ready(pwm_gen_dev)) {
		printk("PWM generator device is not ready");
		return 0;
	}
	if (!device_is_ready(pwm_cap_dev)) {
		printk("PWM capture device is not ready");
		return 0;
	}
	if (!device_is_ready(elc_dev)) {
		printk("ELC device is not ready");
		return 0;
	}
	printk("PWM generator device: %s\n", pwm_gen_dev->name);
	printk("PWM capture device: %s\n", pwm_cap_dev->name);
	printk("ELC device: %s\n\n", elc_dev->name);

	/*
	 * Enable the ELC device.
	 * Software Event 0 is linked to starting the PWM generator,
	 * Software Event 1 is linked to stopping the PWM generator.
	 */
	ret = renesas_elc_enable(elc_dev);
	if (ret) {
		printk("Error: Failed to enable the ELC device. Err=%d\n", ret);
		return 0;
	}

	/*
	 * Configure the PWM generator device.
	 * - PWM_PERIOD_NSEC defines the period (1,000,000 ns).
	 * - 'period' sets the full cycle time.
	 * - 'pulse' is set to half of 'period', yielding a 50%% duty cycle.
	 *
	 * After configuration, the PWM generator starts automatically.
	 */
	uint32_t period = PWM_PERIOD_NSEC;
	uint32_t pulse = PWM_PERIOD_NSEC / 2;

	ret = pwm_set(pwm_gen_dev, PWM_GEN_CHANNEL, period, pulse, PWM_POLARITY_NORMAL);
	if (ret) {
		printk("Error: Failed to set the PWM generator. Err=%d\n", ret);
		return 0;
	}
	printk("PWM generator configured: period=%u ns, pulse=%u ns\n", period, pulse);

	/*
	 * Capture the PWM period to verify PWM generator device operation.
	 * The error between the captured period and the configured period should
	 * be less than 1%.
	 */
	uint64_t period_capture;
	uint64_t pulse_capture;

	ret = pwm_capture_nsec(pwm_cap_dev, PWM_CAP_CHANNEL, PWM_CAPTURE_TYPE_PERIOD,
			       &period_capture, &pulse_capture, K_NSEC(period * 10));
	if (ret) {
		printk("Error: Failed to capture the period value of the PWM generator output. "
		       "Err=%d\n",
		       ret);
		return 0;
	}
	ret = pwm_disable_capture(pwm_cap_dev, PWM_CAP_CHANNEL);
	if (ret) {
		printk("Error: Failed to disable the PWM capture device. Err=%d\n", ret);
		return 0;
	}
	if (!WITHIN(period_capture, period, period / 100)) {
		printk("Error: Period capture off by more than 1%%\n");
		return 0;
	}
	printk("PWM captured: period=%llu ns\n", period_capture);

	/*
	 * Stop the PWM generator using ELC software event 1.
	 * Then, attempt to capture the PWM generator's period.
	 * The capture API is expected to return a timeout (-EAGAIN).
	 */
	printk("\nGenerate ELC software event to stop PWM generator.\n");
	ret = renesas_elc_software_event_generate(elc_dev, PWM_GEN_EVENT_STOP);
	if (ret) {
		printk("Error: Failed to generate ELC software event. Err=%d\n", ret);
		return 0;
	}
	ret = pwm_capture_nsec(pwm_cap_dev, PWM_CAP_CHANNEL, PWM_CAPTURE_TYPE_PERIOD,
			       &period_capture, &pulse_capture, K_NSEC(period * 10));
	if (ret != -EAGAIN) {
		printk("Error: PWM generator cannot be stopped by the ELC link as expected\n");
		return 0;
	}
	ret = pwm_disable_capture(pwm_cap_dev, PWM_CAP_CHANNEL);
	if (ret) {
		printk("Error: Failed to disable the PWM capture device. Err=%d\n", ret);
		return 0;
	}
	printk("PWM generator stopped by the ELC link as expected.\n");

	/*
	 * Restart the PWM generator using ELC software event 0.
	 * Then, capture the PWM generator's period to verify correct operation.
	 * The error between the captured period and the configured period should
	 * be less than 1%.
	 */
	printk("\nGenerate ELC software event to start PWM generator.\n");
	ret = renesas_elc_software_event_generate(elc_dev, PWM_GEN_EVENT_START);
	if (ret) {
		printk("Error: Failed to generate ELC software event. Err=%d\n", ret);
		return 0;
	}
	ret = pwm_capture_nsec(pwm_cap_dev, PWM_CAP_CHANNEL, PWM_CAPTURE_TYPE_PERIOD,
			       &period_capture, &pulse_capture, K_NSEC(period * 10));
	if (ret) {
		printk("Error: PWM generator cannot be started by the ELC link as expected\n");
		return 0;
	}
	ret = pwm_disable_capture(pwm_cap_dev, PWM_CAP_CHANNEL);
	if (ret) {
		printk("Error: Failed to disable the PWM capture device. Err=%d\n", ret);
		return 0;
	}
	if (!WITHIN(period_capture, period, period / 100)) {
		printk("Error: Period capture off by more than 1%%\n");
		return 0;
	}
	printk("PWM captured: period=%llu ns\n", period_capture);
	printk("PWM generator started by the ELC link as expected.\n");

	while (1) {
		k_sleep(K_FOREVER);
	}
	return 0;
}
