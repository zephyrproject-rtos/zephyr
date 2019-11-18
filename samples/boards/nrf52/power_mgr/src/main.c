/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <hal/nrf_gpio.h>
#include "sample.h"

#define BUSY_WAIT_DELAY_S 1

struct device *gpio_port;

const char *uptime_str(void)
{
	static char buf[12];	/* [sss.mmm]:_\0 */
	u32_t now = k_uptime_get_32();

	snprintf(buf, sizeof(buf), "[%3u.%03u]: ", now / MSEC_PER_SEC,
		 now % MSEC_PER_SEC);
	return buf;
}

static void do_delay(u32_t delay_ms)
{
	printk("%sSleep %u ms\n", uptime_str(), delay_ms);
	k_sleep(K_MSEC(delay_ms));
}

/* Application main Thread */
void main(void)
{
	static const u32_t residencies[] = {
		CONFIG_SYS_PM_MIN_RESIDENCY_SLEEP_1,
		CONFIG_SYS_PM_MIN_RESIDENCY_SLEEP_2,
	};

	printk("\n\n*** Power Management Demo on %s ***\n", CONFIG_BOARD);

	printk("Simulate the effect of supporting system sleep states:\n"
	       "* Sleep for durations less than %u ms (S0) leaves LEDs on\n",
	       residencies[0]);
	printk("* Sleep for durations between %u and %u ms (S1) turns off LED 1\n",
	       residencies[0], residencies[1]);
	printk("* Sleep for durations above %u ms (S2) turns off LED 2\n",
	       residencies[1]);
	printk("* Sleep above %u ms enters system off.\n",
	       CONFIG_SYS_PM_MIN_RESIDENCY_DEEP_SLEEP_1);

	printk("%u power states are supported\n", (u32_t)SYS_POWER_STATE_MAX);

	gpio_port = device_get_binding(PORT);

	/* Configure LEDs */
	gpio_pin_configure(gpio_port, LED_1, GPIO_DIR_OUT);
	gpio_pin_write(gpio_port, LED_1, LED_ON);

	gpio_pin_configure(gpio_port, LED_2, GPIO_DIR_OUT);
	gpio_pin_write(gpio_port, LED_2, LED_ON);

	/* Use bits to record enabled state and walk through all four
	 * combinations.
	 */
	int cfg = 0x03;

	while (cfg >= 0) {
		bool enabled;

		/* Nasty logic to configure state because enable/disable are not
		 * idempotent.  See issue #20775.
		 */
		enabled = sys_pm_ctrl_is_state_enabled(SYS_POWER_STATE_SLEEP_1);
		if (enabled != ((cfg & 0x01) != 0)) {
			if (enabled) {
				sys_pm_ctrl_disable_state(SYS_POWER_STATE_SLEEP_1);
			} else {
				sys_pm_ctrl_enable_state(SYS_POWER_STATE_SLEEP_1);
			}
		}

		enabled = sys_pm_ctrl_is_state_enabled(SYS_POWER_STATE_SLEEP_2);
		if (enabled != ((cfg & 0x02) != 0)) {
			if (enabled) {
				sys_pm_ctrl_disable_state(SYS_POWER_STATE_SLEEP_2);
			} else {
				sys_pm_ctrl_enable_state(SYS_POWER_STATE_SLEEP_2);
			}
		}

		printk("%sSys states for %02x: %cS1 %cS2\n", uptime_str(), cfg,
		       sys_pm_ctrl_is_state_enabled(SYS_POWER_STATE_SLEEP_1) ? '+' : '-',
		       sys_pm_ctrl_is_state_enabled(SYS_POWER_STATE_SLEEP_2) ? '+' : '-');

		printk("%sBusy-wait %u s\n", uptime_str(), BUSY_WAIT_DELAY_S);
		k_busy_wait(BUSY_WAIT_DELAY_S * USEC_PER_SEC);

		do_delay(residencies[0] - 1U);
		do_delay((residencies[0] + residencies[1]) / 2U);
		do_delay(residencies[1]);

		--cfg;
	}
	printk("%sCompleted cycle through sleep power state combinations\n",
	       uptime_str());

	/* Configure to generate PORT event (wakeup) on button 1 press. */
	nrf_gpio_cfg_input(BUTTON_1, NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_sense_set(BUTTON_1, NRF_GPIO_PIN_SENSE_LOW);

	printk("%sFalling off main(); press BUTTON1 to restart\n",
	       uptime_str());
}
