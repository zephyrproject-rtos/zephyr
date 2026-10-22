/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/dt-bindings/gpio/stm32-gpio.h>

#define WKUP_SRC_NODE DT_ALIAS(wkup_src)
#if !DT_NODE_HAS_STATUS_OKAY(WKUP_SRC_NODE)
#error "Unsupported board: wkup_src devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(WKUP_SRC_NODE, gpios);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

#if defined(CONFIG_SAMPLE_SCENARIO_S2RAM)
static K_SEM_DEFINE(button_sem, 0, 1);
static struct gpio_callback button_cb_data;

void button_press_cb(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	printk("Hi from button press callback!\n");
	k_sem_give(&button_sem);
}

int configure_irq_callback(void)
{
	int res;

	gpio_init_callback(&button_cb_data, button_press_cb, BIT(button.pin));

	res = gpio_add_callback_dt(&button, &button_cb_data);
	if (res < 0) {
		printk("Failed to add callback for %s pin %d: %d\n",
		       button.port->name, button.pin, res);
		return res;
	}

	res = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (res < 0) {
		printk("Failed to configure %s pin %d as interrupt source: %d\n",
		       button.port->name, button.pin, res);
		return res;
	}

	return 0;
}
#endif /* CONFIG_SAMPLE_SCENARIO_S2RAM */

int main(void)
{
	uint32_t reset_cause;
	int res;

	res = hwinfo_get_reset_cause(&reset_cause);
	if (res == 0) {
		if (reset_cause & RESET_LOW_POWER_WAKE) {
			printk("Reset from low-power state detected.\n");
			printk("Continuing sample execution...\n");
		}
	} else {
		printk("Failed to get reset cause: %d\n", res);
	}

	printk("\nWake-up button is connected to %s pin %d\n", button.port->name, button.pin);

	__ASSERT_NO_MSG(gpio_is_ready_dt(&led));
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	gpio_pin_set(led.port, led.pin, 1);

	/* Setup button GPIO pin as a source for exiting Poweroff */
	res = gpio_pin_configure_dt(&button, GPIO_INPUT | STM32_GPIO_WKUP);
	if (res < 0) {
		printk("Failed to configure %s pin %d as wake-up pin: %d\n",
		       button.port->name, button.pin, res);
		printk("Are you sure this pin is a valid wake-up pin?\n");
		return 0;
	}

	const char *pre_sleep_prompt = "Will wait %d ms before powering the system off\n";

#if defined(CONFIG_SAMPLE_SCENARIO_S2RAM)
	res = configure_irq_callback();
	if (res < 0) {
		printk("Failed to install button press callback: %d\n", res);
		return 0;
	}

	pre_sleep_prompt = "Busy-looping until low-power entry for %d ms...\n";
#endif /* CONFIG_SAMPLE_SCENARIO_S2RAM */

	while (1) {
		printk(pre_sleep_prompt, CONFIG_SAMPLE_BUSY_WAIT_TIME_MS);
		k_busy_wait(CONFIG_SAMPLE_BUSY_WAIT_TIME_MS * USEC_PER_MSEC);

#if defined(CONFIG_SAMPLE_SCENARIO_POWEROFF)
		printk("Powering off\n");
		printk("Press the user button to power the system on\n\n");

		sys_poweroff();
		/* Will remain powered off until wake-up or reset button is pressed */

		return 0;
#endif /* CONFIG_SAMPLE_SCENARIO_POWEROFF */
#if defined(CONFIG_SAMPLE_SCENARIO_S2RAM)
		printk("Waiting up to %d ms in low-power state for button press...\n",
			CONFIG_SAMPLE_BUTTON_PRESS_TIMEOUT_MS);

		res = k_sem_take(&button_sem, K_MSEC(CONFIG_SAMPLE_BUTTON_PRESS_TIMEOUT_MS));
		printk("\n");

		/* HACK: GPIO driver doesn't survive an S2RAM cycle yet... */
		(void)gpio_pin_configure_dt(&button, GPIO_INPUT | STM32_GPIO_WKUP);
		(void)gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);

		if (res == 0) {
			printk("Acquired button press semaphore.\n");
		} else if (res == -EAGAIN) {
			printk("Button was not pressed before timeout.\n");
			printk("Waiting for button press before continuing...\n");
			(void)k_sem_take(&button_sem, K_FOREVER);
		} else {
			printk("Unexpected error occurred: %d\n", res);
			break;
		}
#endif /* CONFIG_SAMPLE_SCENARIO_S2RAM */
	}

	return 0;
}
