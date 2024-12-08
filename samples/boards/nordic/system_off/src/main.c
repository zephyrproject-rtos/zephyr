/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "retained.h"

#include <inttypes.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/util.h>

#if defined(CONFIG_GRTC_WAKEUP_ENABLE)
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#define DEEP_SLEEP_TIME_S 2
#else
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
#endif

int main(void)
{
	int rc;
	const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(cons)) {
		printf("%s: device not ready.\n", cons->name);
		return 0;
	}

	printf("\n%s system off demo\n", CONFIG_BOARD);

	if (IS_ENABLED(CONFIG_APP_USE_NRF_RETENTION) || IS_ENABLED(CONFIG_APP_USE_RETAINED_MEM)) {
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

#if defined(CONFIG_GRTC_WAKEUP_ENABLE)
	int err = z_nrf_grtc_wakeup_prepare(DEEP_SLEEP_TIME_S * USEC_PER_SEC);

	if (err < 0) {
		printk("Unable to prepare GRTC as a wake up source (err = %d).\n", err);
	} else {
		printk("Entering system off; wait %u seconds to restart\n", DEEP_SLEEP_TIME_S);
	}
#else
	/* configure sw0 as input, interrupt as level active to allow wake-up */
	rc = gpio_pin_configure_dt(&sw0, GPIO_INPUT);
	if (rc < 0) {
		printf("Could not configure sw0 GPIO (%d)\n", rc);
		return 0;
	}

	rc = gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_LEVEL_ACTIVE);
	if (rc < 0) {
		printf("Could not configure sw0 GPIO interrupt (%d)\n", rc);
		return 0;
	}

	printf("Entering system off; press sw0 to restart\n");
#endif

	rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	if (rc < 0) {
		printf("Could not suspend console (%d)\n", rc);
		return 0;
	}

	if (IS_ENABLED(CONFIG_APP_USE_NRF_RETENTION) || IS_ENABLED(CONFIG_APP_USE_RETAINED_MEM)) {
		/* Update the retained state */
		retained.off_count += 1;
		retained_update();
	}

	sys_poweroff();

	return 0;
}
