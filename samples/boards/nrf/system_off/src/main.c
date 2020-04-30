/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr.h>
#include <power/power.h>
#include <hal/nrf_gpio.h>

#define BUSY_WAIT_S 2U
#define SLEEP_S 2U

void main(void)
{
	printk("\n%s system off demo\n", CONFIG_BOARD);

	/* Configure to generate PORT event (wakeup) on button 1 press. */
	nrf_gpio_cfg_input(DT_GPIO_PIN(DT_NODELABEL(button0), gpios),
			   NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_sense_set(DT_GPIO_PIN(DT_NODELABEL(button0), gpios),
			       NRF_GPIO_PIN_SENSE_LOW);

	/* Prevent deep sleep (system off) from being entered on long
	 * timeouts.
	 */
	sys_pm_ctrl_disable_state(SYS_POWER_STATE_DEEP_SLEEP_1);

	printk("Busy-wait %u s\n", BUSY_WAIT_S);
	k_busy_wait(BUSY_WAIT_S * USEC_PER_SEC);

	printk("Sleep %u s\n", SLEEP_S);
	k_sleep(K_SECONDS(SLEEP_S));

	printk("Sleep %u ms (deep sleep minimum)\n",
	       CONFIG_SYS_PM_MIN_RESIDENCY_DEEP_SLEEP_1);
	k_sleep(K_MSEC(CONFIG_SYS_PM_MIN_RESIDENCY_DEEP_SLEEP_1));

	printk("Entering system off; press BUTTON1 to restart\n");

	/* Above we disabled entry to deep sleep based on duration of
	 * controlled delay.  Here we need to override that, then
	 * force a sleep so that the deep sleep takes effect.
	 */
	sys_pm_force_power_state(SYS_POWER_STATE_DEEP_SLEEP_1);
	k_sleep(K_MSEC(1));

	printk("ERROR: System off failed\n");
	while (true) {
		/* spin to avoid fall-off behavior */
	}
}
