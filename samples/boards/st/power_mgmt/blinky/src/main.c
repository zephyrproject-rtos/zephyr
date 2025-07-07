/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/pm/device_runtime.h>

/* define SLEEP_TIME_MS higher than <st,counter-value> in ms */
#if DT_PROP(DT_NODELABEL(stm32_lp_tick_source), st_counter_value)
#define SLEEP_TIME_MS   (DT_PROP(DT_NODELABEL(stm32_lp_tick_source), st_counter_value) * 1400)
#else
#define SLEEP_TIME_MS   2000
#endif

#define STM32_GPIO_PM_ENABLE(node_id) \
	pm_device_runtime_enable(DEVICE_DT_GET(node_id));

static const struct gpio_dt_spec led =
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
	bool led_is_on = true;

	__ASSERT_NO_MSG(gpio_is_ready_dt(&led));

	/* Enable device runtime PM on each GPIO port.
	 * GPIO configuration is lost but it may result in lower power consumption.
	 */
	DT_FOREACH_STATUS_OKAY(st_stm32_gpio, STM32_GPIO_PM_ENABLE)

	printk("Device ready\n");

	while (true) {
		gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		gpio_pin_set(led.port, led.pin, (int)led_is_on);
		if (led_is_on == false) {
			/* Release resource to release device clock */
			gpio_pin_configure(led.port, led.pin, GPIO_DISCONNECTED);
		}
		k_msleep(SLEEP_TIME_MS);
		if (led_is_on == true) {
			/* Release resource to release device clock */
			gpio_pin_configure(led.port, led.pin, GPIO_DISCONNECTED);
		}
		led_is_on = !led_is_on;
	}
	return 0;
}
