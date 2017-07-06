/*
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <watchdog.h>
#include <misc/printk.h>

/*
 * The independent watchdog (IWDG) is clocked by its own dedicated
 * 40kHz (STM32F1X or STM32F3X) or 32kHz (STM32F4X or STM32L4X)
 * low-speed internal clock (LSI).
 *
 * Sample application is dedicated to STM32 family.
 *
 * For STM32F1X/STM32F3X family:
 *     prescaler register should be set to 'divider /16' and the reload
 *     register to 2500.
 *
 * For STM32F4X/STM32L4X family:
 *     prescaler register should be set to 'divider /16' and the reload
 *     register to 2000.
 *
 * Then it takes 1 second a reset signal is generated.
 */

#define TIMEOUT_VALID		900
#define TIMEOUT_INVALID		1100

void main(void)
{
	struct device *iwdg = device_get_binding("IWDG");
	struct k_timer iwdg_timer;
	struct wdt_config config;
	u8_t counter = 0;

	wdt_enable(iwdg);

	k_timer_init(&iwdg_timer, NULL, NULL);

	/* Set IWDG timeout to 1 second. */
	config.timeout = 1000000;
	wdt_set_config(iwdg, &config);

	while (1) {
		printk("Welcome...\n");

		if (counter % 2 == 0) {
			/* Less than IWDG reload value. */
			k_timer_start(&iwdg_timer, K_MSEC(TIMEOUT_VALID), 0);
		} else {
			/* More than IWDG reload value. */
			k_timer_start(&iwdg_timer, K_MSEC(TIMEOUT_INVALID), 0);
		}

		counter++;
		/* Wait till application timer expires. */
		k_timer_status_sync(&iwdg_timer);

		printk("Watchdog refreshed...\n");
		k_sleep(2);
		wdt_reload(iwdg);
	}
}
