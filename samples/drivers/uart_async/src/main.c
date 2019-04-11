/*
 * Copyright 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <uart.h>
#include <string.h>
#include <kernel.h>

#define ASYNC_BUF_SZ 128

static struct main_data {
	struct device *uart_dev;
	u8_t rx_buf[ASYNC_BUF_SZ];
	u8_t tx_buf[ASYNC_BUF_SZ];
	struct k_sem tx_sem;
} _main_data;

static void uart_async_callback(struct uart_event *evt, void *priv_data)
{
	struct main_data *data = priv_data;

	if (evt->type == UART_TX_DONE) {
		printk("TX done: %d\n", evt->data.tx.len);
		memset(data->tx_buf, 0, sizeof(data->tx_buf));

	} else if (evt->type == UART_RX_RDY) {
		u8_t *src_buf = evt->data.rx.buf
				+ evt->data.rx.offset;
		memcpy(data->tx_buf, src_buf, evt->data.rx.len);
		printk("RX done: %d - %s\n", evt->data.rx.len, data->tx_buf);
		if (evt->data.rx.len > 0) {
			k_sem_give(&data->tx_sem);
		}
	}
}

int main(void)
{
	u32_t version = sys_kernel_version_get();

	printk(
		"UART-Async API Echo ARCH: %s, BOARD: %s, version: %d.%d.%d\n",
		CONFIG_ARCH, CONFIG_BOARD,
		SYS_KERNEL_VER_MAJOR(version),
		SYS_KERNEL_VER_MINOR(version),
		SYS_KERNEL_VER_PATCHLEVEL(version));

	struct device *uart = device_get_binding(DT_UART_STM32_USART_6_NAME);

	if (uart == NULL) {
		printk("UART open failed!\n");
		return -1;
	}

	struct main_data *data = &_main_data;

	data->uart_dev = uart;

	u8_t *buf = data->rx_buf;

	memset(buf, 0, sizeof(ASYNC_BUF_SZ));


	uart_callback_set(uart, uart_async_callback, data);
	k_sem_init(&data->tx_sem, 0, 1);

	/* start rx */
	uart_rx_enable(uart, buf, ASYNC_BUF_SZ, K_FOREVER);
	while (1) {
		k_sem_take(&data->tx_sem, K_FOREVER);
		int len = strlen(data->tx_buf);

		if (len > 0) {
			uart_tx(data->uart_dev, data->tx_buf,
					len, K_FOREVER);
		}
	}

	return 0;
}
