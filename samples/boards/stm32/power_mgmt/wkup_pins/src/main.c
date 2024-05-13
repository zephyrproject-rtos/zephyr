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
#include <zephyr/sys/poweroff.h>

#define WAIT_TIME_US 4000000

#define WKUP_SRC_NODE DT_ALIAS(wkup_src)
#if !DT_NODE_HAS_STATUS(WKUP_SRC_NODE, okay)
#error "Unsupported board: wkup_src devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(WKUP_SRC_NODE, gpios);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
	__ASSERT_NO_MSG(gpio_is_ready_dt(&button));
	printk("\nWake-up button set up at %s pin %d\n", button.port->name, button.pin);

	__ASSERT_NO_MSG(gpio_is_ready_dt(&led));
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	gpio_pin_set(led.port, led.pin, 1);

	printk("Device is ready\n");

	printk("Will wait %ds before powering the system off\n", (WAIT_TIME_US / 1000000));
	k_busy_wait(WAIT_TIME_US);

	printk("Powering off\n");
	printk("Press the user button to power the system on\n\n");

	sys_poweroff();
	/* Will remain powered off until wake-up or reset button is pressed */

	return 0;
}
