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

/* Change this if you have an LED connected to a custom port */
#ifndef DT_ALIAS_LED0_GPIOS_CONTROLLER
#define DT_ALIAS_LED0_GPIOS_CONTROLLER 	LED0_GPIO_PORT
#endif
#ifndef DT_ALIAS_LED1_GPIOS_CONTROLLER
#define DT_ALIAS_LED1_GPIOS_CONTROLLER 	LED1_GPIO_PORT
#endif

#define PORT0	 DT_ALIAS_LED0_GPIOS_CONTROLLER
#define PORT1	 DT_ALIAS_LED1_GPIOS_CONTROLLER


/* Change this if you have an LED connected to a custom pin */
#define LED0    DT_ALIAS_LED0_GPIOS_PIN
#define LED1    DT_ALIAS_LED1_GPIOS_PIN

struct printk_data_t {
	void *fifo_reserved; /* 1st word reserved for use by fifo */
	u32_t led;
	u32_t cnt;
};

K_FIFO_DEFINE(printk_fifo);

void blink(const char *port, u32_t sleep_ms, u32_t led, u32_t id)
{
	int cnt = 0;
	struct device *gpio_dev;

	gpio_dev = device_get_binding(port);
	__ASSERT_NO_MSG(gpio_dev != NULL);

	gpio_pin_configure(gpio_dev, led, GPIO_DIR_OUT);

	while (1) {
		gpio_pin_write(gpio_dev, led, cnt % 2);

		struct printk_data_t tx_data = { .led = id, .cnt = cnt };

		size_t size = sizeof(struct printk_data_t);
		char *mem_ptr = k_malloc(size);
		__ASSERT_NO_MSG(mem_ptr != 0);

		memcpy(mem_ptr, &tx_data, size);

		k_fifo_put(&printk_fifo, mem_ptr);

		k_sleep(sleep_ms);
		cnt++;
	}
}

void blink1(void)
{
	blink(PORT0, 100, LED0, 0);
}

void blink2(void)
{
	blink(PORT1, 1000, LED1, 1);
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
		PRIORITY, 0, K_NO_WAIT);
K_THREAD_DEFINE(blink2_id, STACKSIZE, blink2, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
K_THREAD_DEFINE(uart_out_id, STACKSIZE, uart_out, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
