/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <stm32u5xx_ll_pwr.h>
#include <gpio/gpio_stm32.h>

#define SLEEP_TIME_MS   2000

static const struct gpio_dt_spec led =
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
	bool led_is_on = true;

	/* Compute GPIO port to be passed to LL_PWR_EnableGPIOPullUp etc. */
	const struct gpio_stm32_config *cfg = led.port->config;
	uint32_t pwr_port = ((uint32_t)LL_PWR_GPIO_PORTA) + (cfg->port * 8);

	__ASSERT_NO_MSG(gpio_is_ready_dt(&led));

	printk("Device ready\n");
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

	/* Enable the pull-up/pull-down feature globally.
	 * User can decide to not use this feature to further reduce power consumption
	 *
	 * This configuration is active only in STOP3 mode, so no need to disable it
	 * during wake up
	 */
	LL_PWR_EnablePUPDConfig();

	while (true) {
		gpio_pin_set(led.port, led.pin, (int)led_is_on);
		/* In STOP3, GPIOs are disabled. Only pull-up/pull-down can be enabled.
		 * So we enable pull-up/pull-down based on LED status
		 */
		if (led_is_on) {
			LL_PWR_DisableGPIOPullDown(pwr_port, (1 << led.pin));
			LL_PWR_EnableGPIOPullUp(pwr_port, (1 << led.pin));
		} else {
			LL_PWR_DisableGPIOPullUp(pwr_port, (1 << led.pin));
			LL_PWR_EnableGPIOPullDown(pwr_port, (1 << led.pin));
		}

		k_msleep(SLEEP_TIME_MS);
		led_is_on = !led_is_on;
	}
	return 0;
}
