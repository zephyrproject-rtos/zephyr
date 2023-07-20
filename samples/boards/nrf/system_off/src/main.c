/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/poweroff.h>
#include <soc.h>
#include "retained.h"
#include <hal/nrf_gpio.h>

#define BUSY_WAIT_S 2U
#define SLEEP_S 2U

int main(void)
{
	int rc;
	const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(cons)) {
		printk("%s: device not ready.\n", cons->name);
		return 0;
	}

	printk("\n%s system off demo\n", CONFIG_BOARD);

	if (IS_ENABLED(CONFIG_APP_RETENTION)) {
		bool retained_ok = retained_validate();

		/* Increment for this boot attempt and update. */
		retained.boots += 1;
		retained_update();

		printk("Retained data: %s\n", retained_ok ? "valid" : "INVALID");
		printk("Boot count: %u\n", retained.boots);
		printk("Off count: %u\n", retained.off_count);
		printk("Active Ticks: %" PRIu64 "\n", retained.uptime_sum);
	} else {
		printk("Retained data not supported\n");
	}

	/* Configure to generate PORT event (wakeup) on button 1 press. */
	nrf_gpio_cfg_input(NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(sw0), gpios),
			   NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_sense_set(NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(sw0), gpios),
			       NRF_GPIO_PIN_SENSE_LOW);

	printk("Busy-wait %u s\n", BUSY_WAIT_S);
	k_busy_wait(BUSY_WAIT_S * USEC_PER_SEC);

	printk("Busy-wait %u s with UART off\n", BUSY_WAIT_S);
	rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	k_busy_wait(BUSY_WAIT_S * USEC_PER_SEC);
	rc = pm_device_action_run(cons, PM_DEVICE_ACTION_RESUME);

	printk("Sleep %u s\n", SLEEP_S);
	k_sleep(K_SECONDS(SLEEP_S));

	printk("Sleep %u s with UART off\n", SLEEP_S);
	rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	k_sleep(K_SECONDS(SLEEP_S));
	rc = pm_device_action_run(cons, PM_DEVICE_ACTION_RESUME);

	printk("Entering system off; press BUTTON1 to restart\n");

	if (IS_ENABLED(CONFIG_APP_RETENTION)) {
		/* Update the retained state */
		retained.off_count += 1;
		retained_update();
	}

	sys_poweroff();

	return 0;
}
