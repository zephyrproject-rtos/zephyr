/*
 * Copyright (C) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(adi_pm);

/* LED from devicetree */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* Get residency info */
static const struct pm_state_info residency_info[] =
	PM_STATE_INFO_LIST_FROM_DT_CPU(DT_NODELABEL(cpu0));

int main(void)
{
	/* Configure LED to show device is operational */
	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE)) {
		LOG_ERR("Error configuring LED pin!");
		return -1;
	}

	int sleep_time_us;
	enum pm_state state = PM_STATE_ACTIVE;

	while (1) {

		/* Iterate through each power mode of the system */
		for (int i = 0; i < DT_NUM_CPU_POWER_STATES(DT_NODELABEL(cpu0)); i++) {

			if (state == PM_STATE_SUSPEND_TO_RAM) {
				if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE)) {
					LOG_ERR("Error configuring LED pin!");
					return -1;
				}
			}

			/* Get sleep time and state name of new power mode */
			sleep_time_us = residency_info[i].min_residency_us +
					residency_info[i].exit_latency_us + 100;
			state = residency_info[i].state;

			/* Blink LED! */
			for (int j = 0; j < 4; j++) {
				if (gpio_pin_toggle_dt(&led)) {
					LOG_ERR("Error toggling LED pin!");
					return -1;
				}

				/* Busy waiting doesn't trigger PM transition */
				k_busy_wait(50000);
			}

			/* Enter power mode */
			k_usleep(sleep_time_us);
		}
	}

	return 0;
}
