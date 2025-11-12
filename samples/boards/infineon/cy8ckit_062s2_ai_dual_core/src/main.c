/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

/* Aliases defined in the board's device tree */
#define LED_NODE    DT_ALIAS(led0)

LOG_MODULE_REGISTER(main_CM0);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

int main(void)
{
	int ret;

	LOG_INF("Hello from CM0+ - Dual Core Sample");

	/* Initialize LED0 */
	if (!device_is_ready(led.port)) {
		LOG_ERR("LED0 device not ready");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure LED0");
		return 0;
	}

	/* Blink LED0 5 times to indicate CM0+ is running */
	for (int i = 0; i < 5; i++) {
		gpio_pin_toggle_dt(&led);
		k_sleep(K_MSEC(500));
	}

	LOG_INF("Starting CM4 core...");
	Cy_SysEnableCM4(0x10080000);

	k_sleep(K_SECONDS(1));
	LOG_INF("CM4 core started");

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
