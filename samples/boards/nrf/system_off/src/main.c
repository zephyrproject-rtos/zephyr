/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "retained.h"

#include <inttypes.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/util.h>

#include <hal/nrf_gpio.h>
#include <soc.h>

#define BUSY_WAIT_S 2U
#define SLEEP_S 2U

int main(void)
{
	int rc;
	const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(cons)) {
		printf("%s: device not ready.\n", cons->name);
		return 0;
	}

	printf("\n%s system off demo\n", CONFIG_BOARD);

	if (IS_ENABLED(CONFIG_APP_RETENTION)) {
		bool retained_ok = retained_validate();

		/* Increment for this boot attempt and update. */
		retained.boots += 1;
		retained_update();

		printf("Retained data: %s\n", retained_ok ? "valid" : "INVALID");
		printf("Boot count: %u\n", retained.boots);
		printf("Off count: %u\n", retained.off_count);
		printf("Active Ticks: %" PRIu64 "\n", retained.uptime_sum);
	} else {
		printf("Retained data not supported\n");
	}

	/* Configure to generate PORT event (wakeup) on button 1 press. */
	nrf_gpio_cfg_input(NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(sw0), gpios),
			   NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_sense_set(NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(sw0), gpios),
			       NRF_GPIO_PIN_SENSE_LOW);

	printf("Busy-wait %u s\n", BUSY_WAIT_S);
	k_busy_wait(BUSY_WAIT_S * USEC_PER_SEC);

	printf("Busy-wait %u s with UART off\n", BUSY_WAIT_S);
	rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	k_busy_wait(BUSY_WAIT_S * USEC_PER_SEC);
	rc = pm_device_action_run(cons, PM_DEVICE_ACTION_RESUME);

	printf("Sleep %u s\n", SLEEP_S);
	k_sleep(K_SECONDS(SLEEP_S));

	printf("Sleep %u s with UART off\n", SLEEP_S);
	rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	k_sleep(K_SECONDS(SLEEP_S));
	rc = pm_device_action_run(cons, PM_DEVICE_ACTION_RESUME);

	printf("Entering system off; press BUTTON1 to restart\n");

	if (IS_ENABLED(CONFIG_APP_RETENTION)) {
		/* Update the retained state */
		retained.off_count += 1;
		retained_update();
	}

	sys_poweroff();

	return 0;
}
