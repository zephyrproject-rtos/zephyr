/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/printk.h>
#include <sys/__assert.h>
#include <string.h>

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

struct printk_data_t {
	void *fifo_reserved; /* 1st word reserved for use by fifo */
	u32_t led;
	u32_t cnt;
};

K_FIFO_DEFINE(printk_fifo);

#ifndef DT_ALIAS_LED0_GPIOS_FLAGS
#define DT_ALIAS_LED0_GPIOS_FLAGS 0
#endif

#ifndef DT_ALIAS_LED1_GPIOS_FLAGS
#define DT_ALIAS_LED1_GPIOS_FLAGS 0
#endif

struct led {
	const char *gpio_dev_name;
	const char *gpio_pin_name;
	unsigned int gpio_pin;
	unsigned int gpio_flags;
};

void blink(const struct led *led, u32_t sleep_ms, u32_t id)
{
	struct device *gpio_dev;
	int cnt = 0;
	int ret;

	gpio_dev = device_get_binding(led->gpio_dev_name);
	__ASSERT(gpio_dev != NULL, "Error: didn't find %s device\n",
			led->gpio_dev_name);

	ret = gpio_pin_configure(gpio_dev, led->gpio_pin, led->gpio_flags);
	if (ret != 0) {
		printk("Error %d: failed to configure pin %d '%s'\n",
			ret, led->gpio_pin, led->gpio_pin_name);
		return;
	}

	while (1) {
		gpio_pin_set(gpio_dev, led->gpio_pin, cnt % 2);

		struct printk_data_t tx_data = { .led = id, .cnt = cnt };

		size_t size = sizeof(struct printk_data_t);
		char *mem_ptr = k_malloc(size);
		__ASSERT_NO_MSG(mem_ptr != 0);

		memcpy(mem_ptr, &tx_data, size);

		k_fifo_put(&printk_fifo, mem_ptr);

		k_msleep(sleep_ms);
		cnt++;
	}
}

void blink1(void)
{
	const struct led led1 = {
		.gpio_dev_name = DT_ALIAS_LED0_GPIOS_CONTROLLER,
		.gpio_pin_name = DT_ALIAS_LED0_LABEL,
		.gpio_pin = DT_ALIAS_LED0_GPIOS_PIN,
		.gpio_flags = GPIO_OUTPUT | DT_ALIAS_LED0_GPIOS_FLAGS,
	};

	blink(&led1, 100, 0);
}

void blink2(void)
{
	const struct led led2 = {
		.gpio_dev_name = DT_ALIAS_LED1_GPIOS_CONTROLLER,
		.gpio_pin_name = DT_ALIAS_LED1_LABEL,
		.gpio_pin = DT_ALIAS_LED1_GPIOS_PIN,
		.gpio_flags = GPIO_OUTPUT | DT_ALIAS_LED1_GPIOS_FLAGS,
	};

	blink(&led2, 1000, 1);
}

void uart_out(void)
{
	while (1) {
		struct printk_data_t *rx_data = k_fifo_get(&printk_fifo, K_FOREVER);
		printk("Toggle USR%d LED: Counter = %d\n", rx_data->led, rx_data->cnt);
		k_free(rx_data);
	}
}

K_THREAD_DEFINE(blink1_id, STACKSIZE, blink1, NULL, NULL, NULL,
		PRIORITY, 0, 0);
K_THREAD_DEFINE(blink2_id, STACKSIZE, blink2, NULL, NULL, NULL,
		PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_out_id, STACKSIZE, uart_out, NULL, NULL, NULL,
		PRIORITY, 0, 0);
