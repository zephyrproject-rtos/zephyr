/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr.h>
#include <device.h>
#include <power/power.h>
#include <hal/nrf_gpio.h>

#define CONSOLE_LABEL DT_LABEL(DT_CHOSEN(zephyr_console))

#define BUSY_WAIT_S 2U
#define SLEEP_S 2U

void main(void)
{
	int rc;
	struct device *cons = device_get_binding(CONSOLE_LABEL);

	printk("\n%s system off demo\n", CONFIG_BOARD);

	/* Configure to generate PORT event (wakeup) on button 1 press. */
	nrf_gpio_cfg_input(DT_GPIO_PIN(DT_NODELABEL(button0), gpios),
			   NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_sense_set(DT_GPIO_PIN(DT_NODELABEL(button0), gpios),
			       NRF_GPIO_PIN_SENSE_LOW);

	printk("Busy-wait %u s\n", BUSY_WAIT_S);
	k_busy_wait(BUSY_WAIT_S * USEC_PER_SEC);

	printk("Busy-wait %u s with UART off\n", BUSY_WAIT_S);
	rc = device_set_power_state(cons, DEVICE_PM_LOW_POWER_STATE, NULL, NULL);
	k_busy_wait(BUSY_WAIT_S * USEC_PER_SEC);
	rc = device_set_power_state(cons, DEVICE_PM_ACTIVE_STATE, NULL, NULL);

	printk("Sleep %u s\n", SLEEP_S);
	k_sleep(K_SECONDS(SLEEP_S));

	printk("Sleep %u s with UART off\n", SLEEP_S);
	rc = device_set_power_state(cons, DEVICE_PM_LOW_POWER_STATE, NULL, NULL);
	k_sleep(K_SECONDS(SLEEP_S));
	rc = device_set_power_state(cons, DEVICE_PM_ACTIVE_STATE, NULL, NULL);

	printk("Entering system off; press BUTTON1 to restart\n");

	/* Force the system to enter the deep sleep state. */
	sys_pm_force_power_state(SYS_POWER_STATE_DEEP_SLEEP_1);
	k_sleep(K_MSEC(1));

	printk("ERROR: System off failed\n");
	while (true) {
		/* spin to avoid fall-off behavior */
	}
}
