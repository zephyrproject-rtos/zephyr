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
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/util.h>

#define NON_WAKEUP_RESET_REASON (RESET_PIN | RESET_SOFTWARE | RESET_POR | RESET_DEBUG)

#if defined(CONFIG_GRTC_WAKEUP_ENABLE)
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#define DEEP_SLEEP_TIME_S 2
#endif
#if defined(CONFIG_GPIO_WAKEUP_ENABLE)
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
#endif
#if defined(CONFIG_LPCOMP_WAKEUP_ENABLE)
static const struct device *comp_dev = DEVICE_DT_GET(DT_NODELABEL(comp));
#endif

void print_reset_cause(uint32_t reset_cause)
{
	if (reset_cause & RESET_DEBUG) {
		printf("Reset by debugger.\n");
	} else if (reset_cause & RESET_CLOCK) {
		printf("Wakeup from System OFF by GRTC.\n");
	} else if (reset_cause & RESET_LOW_POWER_WAKE) {
		printf("Wakeup from System OFF by GPIO.\n");
	} else  {
		printf("Other wake up cause 0x%08X.\n", reset_cause);
	}
}

int main(void)
{
	int rc;
	uint32_t reset_cause;
	const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(cons)) {
		printf("%s: device not ready.\n", cons->name);
		return 0;
	}

	printf("\n%s system off demo\n", CONFIG_BOARD);
	hwinfo_get_reset_cause(&reset_cause);
	print_reset_cause(reset_cause);

	if (IS_ENABLED(CONFIG_APP_USE_RETAINED_MEM)) {
		bool retained_ok = retained_validate();

		if (reset_cause & NON_WAKEUP_RESET_REASON) {
			retained.boots = 0;
			retained.off_count = 0;
			retained.uptime_sum = 0;
			retained.uptime_latest = 0;
			retained_ok = true;
		}
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
#endif
#if defined(CONFIG_GPIO_WAKEUP_ENABLE)
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
#if defined(CONFIG_LPCOMP_WAKEUP_ENABLE)
	comparator_set_trigger(comp_dev, COMPARATOR_TRIGGER_BOTH_EDGES);
	comparator_trigger_is_pending(comp_dev);
	printf("Entering system off; change signal level at comparator input to restart\n");
#endif

	rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	if (rc < 0) {
		printf("Could not suspend console (%d)\n", rc);
		return 0;
	}

	if (IS_ENABLED(CONFIG_APP_USE_RETAINED_MEM)) {
		/* Update the retained state */
		retained.off_count += 1;
		retained_update();
	}

	hwinfo_clear_reset_cause();
	sys_poweroff();

	return 0;
}
