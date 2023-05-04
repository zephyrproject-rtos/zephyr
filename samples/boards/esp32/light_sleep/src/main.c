/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/drivers/gpio.h>
#include "esp_sleep.h"

/* Most development boards have "boot" button attached to GPIO0.
 * You can also change this to another pin.
 */
#define SW0_NODE	DT_ALIAS(sw0)

#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "unsupported board: sw0 devicetree alias is not defined"
#endif

/* Add an extra delay when sleeping to make sure that light sleep
 * is chosen.
 */
#define LIGHT_SLP_EXTRA_DELAY	(50UL)

static const struct gpio_dt_spec button =
	GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});

int main(void)
{
	if (!device_is_ready(button.port)) {
		printk("Error: button device %s is not ready\n", button.port->name);
		return 0;
	}

	const int wakeup_level = (button.dt_flags & GPIO_ACTIVE_LOW) ? 0 : 1;

	esp_gpio_wakeup_enable(button.pin,
			wakeup_level == 0 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);

	while (true) {
		/* Wake up in 2 seconds, or when button is pressed */
		esp_sleep_enable_timer_wakeup(2000000);
		esp_sleep_enable_gpio_wakeup();

		/* Wait until GPIO goes high */
		if (gpio_pin_get_dt(&button) == wakeup_level) {
			printk("Waiting for GPIO%d to go high...\n", button.pin);
			do {
				k_busy_wait(10000);
			} while (gpio_pin_get_dt(&button) == wakeup_level);
		}

		printk("Entering light sleep\n");
		/* To make sure the complete line is printed before entering sleep mode,
		 * need to wait until UART TX FIFO is empty
		 */
		k_busy_wait(10000);

		/* Get timestamp before entering sleep */
		int64_t t_before_ms = k_uptime_get();

		/* Sleep triggers the idle thread, which makes the pm subsystem select some
		 * pre-defined power state. Light sleep is used here because there is enough
		 * time to consider it, energy-wise, worthy.
		 */
		k_sleep(K_USEC(DT_PROP(DT_NODELABEL(light_sleep), min_residency_us) +
					LIGHT_SLP_EXTRA_DELAY));
		/* Execution continues here after wakeup */

		/* Get timestamp after waking up from sleep */
		int64_t t_after_ms = k_uptime_get();

		/* Determine wake up reason */
		const char *wakeup_reason;

		switch (esp_sleep_get_wakeup_cause()) {
		case ESP_SLEEP_WAKEUP_TIMER:
			wakeup_reason = "timer";
			break;
		case ESP_SLEEP_WAKEUP_GPIO:
			wakeup_reason = "pin";
			break;
		default:
			wakeup_reason = "other";
			break;
		}

		printk("Returned from light sleep, reason: %s, t=%lld ms, slept for %lld ms\n",
				wakeup_reason, t_after_ms, (t_after_ms - t_before_ms));
	}

	return 0;
}
