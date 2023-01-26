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

#define WAKEUP_TIME_SEC		(20)

#ifdef CONFIG_EXAMPLE_EXT1_WAKEUP
#define EXT_WAKEUP_PIN_1	(2)
#define EXT_WAKEUP_PIN_2	(4)
#endif

#ifdef CONFIG_EXAMPLE_GPIO_WAKEUP
#if !DT_NODE_HAS_STATUS(DT_ALIAS(wakeup_button), okay)
#error "Unsupported: wakeup-button alias is not defined"
#else
static const struct gpio_dt_spec wakeup_button = GPIO_DT_SPEC_GET(DT_ALIAS(wakeup_button), gpios);
#endif
#endif

void main(void)
{
	switch (esp_sleep_get_wakeup_cause()) {
#ifdef CONFIG_EXAMPLE_EXT1_WAKEUP
	case ESP_SLEEP_WAKEUP_EXT1:
	{
		uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();

		if (wakeup_pin_mask != 0) {
			int pin = __builtin_ffsll(wakeup_pin_mask) - 1;

			printk("Wake up from GPIO %d\n", pin);
		} else {
			printk("Wake up from GPIO\n");
		}
		break;
	}
#endif /* CONFIG_EXAMPLE_EXT1_WAKEUP */
#ifdef CONFIG_EXAMPLE_GPIO_WAKEUP
	case ESP_SLEEP_WAKEUP_GPIO:
	{
		uint64_t wakeup_pin_mask = esp_sleep_get_gpio_wakeup_status();

		if (wakeup_pin_mask != 0) {
			int pin = __builtin_ffsll(wakeup_pin_mask) - 1;

			printk("Wake up from GPIO %d\n", pin);
		} else {
			printk("Wake up from GPIO\n");
		}
		break;
	}
#endif /* CONFIG_EXAMPLE_GPIO_WAKEUP */
	case ESP_SLEEP_WAKEUP_TIMER:
		printk("Wake up from timer.\n");
		break;
	case ESP_SLEEP_WAKEUP_UNDEFINED:
	default:
		printk("Not a deep sleep reset\n");
	}

	k_busy_wait(1000000);

	const int wakeup_time_sec = WAKEUP_TIME_SEC;

	printk("Enabling timer wakeup, %ds\n", wakeup_time_sec);
	esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);

#ifdef CONFIG_EXAMPLE_EXT1_WAKEUP
	const int ext_wakeup_pin_1 = EXT_WAKEUP_PIN_1;
	const uint64_t ext_wakeup_pin_1_mask = BIT64(ext_wakeup_pin_1);
	const int ext_wakeup_pin_2 = EXT_WAKEUP_PIN_2;
	const uint64_t ext_wakeup_pin_2_mask = BIT64(ext_wakeup_pin_2);

	printk("Enabling EXT1 wakeup on pins GPIO%d, GPIO%d\n",
			ext_wakeup_pin_1, ext_wakeup_pin_2);
	esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask | ext_wakeup_pin_2_mask,
			ESP_EXT1_WAKEUP_ANY_HIGH);
#endif /* CONFIG_EXAMPLE_EXT1_WAKEUP */
#ifdef CONFIG_EXAMPLE_GPIO_WAKEUP
	if (!device_is_ready(wakeup_button.port)) {
		printk("Error: wakeup button device %s is not ready\n", wakeup_button.port->name);
		return;
	}

	int ret = gpio_pin_configure_dt(&wakeup_button, GPIO_INPUT);

	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
				ret, wakeup_button.port->name, wakeup_button.pin);
		return;
	}

	esp_deep_sleep_enable_gpio_wakeup(BIT(wakeup_button.pin), ESP_GPIO_WAKEUP_GPIO_HIGH);
	printk("Enabling GPIO wakeup on pins GPIO%d\n", wakeup_button.pin);
#endif /* CONFIG_EXAMPLE_GPIO_WAKEUP */
	printk("Entering deep sleep\n");

	/* Sleep triggers the idle thread, which makes the pm subsystem apply the selected
	 * power state. Deep sleep is forced here because power states' timings differ for
	 * different SoCs.
	 */
	pm_state_force(0u, &(struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
	k_sleep(K_SECONDS(2));

	printk("ERROR: Deep Sleep failed\n");

	while (true) {
		/* Never reaches here. Spins to avoid fall-off behavior */
	}
}
