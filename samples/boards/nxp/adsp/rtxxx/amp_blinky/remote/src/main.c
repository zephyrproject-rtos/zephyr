/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2s.h>

#define SLEEP_TIME_MS 1000
#define LED_NODE      DT_ALIAS(led1)
#define BTN_NODE      DT_ALIAS(sw0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET_OR(BTN_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

static bool blink = true;

static struct k_thread th_blink;
static K_THREAD_STACK_DEFINE(th_blink_stack, 4096);

void btn_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("[DSP] Button pressed!\n");
	blink = !blink;
}

void thread_blink(void *unused0, void *unused1, void *unused2)
{
	int ret = 0;

	while (1) {
		if (blink) {
			ret = gpio_pin_toggle_dt(&led);
			if (ret < 0) {
				return;
			}
		} else {
			ret = gpio_pin_set_dt(&led, 0);
			if (ret < 0) {
				return;
			}
		}
		k_msleep(SLEEP_TIME_MS);
	}
}

int main(void)
{
	int ret;

	printk("[DSP] Hello World! %s\n", CONFIG_BOARD_TARGET);

	if (!gpio_is_ready_dt(&btn)) {
		return 0;
	}

	if (gpio_pin_configure_dt(&btn, GPIO_INPUT) != 0) {
		return 0;
	}

	if (gpio_pin_interrupt_configure_dt(&btn, GPIO_INT_EDGE_TO_ACTIVE) != 0) {
		return 0;
	}

	gpio_init_callback(&button_cb_data, &btn_isr, BIT(btn.pin));
	gpio_add_callback(btn.port, &button_cb_data);

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	k_thread_create(&th_blink, th_blink_stack, K_THREAD_STACK_SIZEOF(th_blink_stack),
			thread_blink, NULL, NULL, NULL, 5, 0, K_NO_WAIT);

	return 0;
}
