/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/printk.h>
#include <string.h>

#define RING_BUF_SIZE 1000
#define RX_BUF_SIZE   10

#define RECEIVE_TIMEOUT 0

#define STACK_SIZE 1024

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* uart configuration structure */
const struct uart_config uart_cfg = {.baudrate = 115200,
				     .parity = UART_CFG_PARITY_NONE,
				     .stop_bits = UART_CFG_STOP_BITS_1,
				     .data_bits = UART_CFG_DATA_BITS_8,
				     .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};

/* define a ring buffer to get raw bytes*/
RING_BUF_DECLARE(ring_buf, RING_BUF_SIZE);

/* define uart rx buffer */
static uint8_t rx_buffer[RX_BUF_SIZE];

/* define thread  stack size */
static K_THREAD_STACK_DEFINE(uart_rx_stack, STACK_SIZE);

/* struct uart_event async_evt */
static struct k_thread uart_rx_thread_data = {0};

void print_uart(char *buf, int len)
{
	for (int i = 0; i < len; i++) {

		if ((buf[i] == '\n' || buf[i] == '\r')) {
			uart_poll_out(uart_dev, '\n');
		} else {
			uart_poll_out(uart_dev, buf[i]);
		}
	}
}

/* Data processing thread */
static void uart_rx_thread(void *p1, void *p2, void *p3)
{
	uint8_t rx_data[RX_BUF_SIZE];
	size_t len;

	while (1) {

		/* Check if there's data in the ring buffer */
		len = ring_buf_get(&ring_buf, rx_data, sizeof(rx_data));

		if (len > 0) {

			/* Process `len` bytes of data */
			print_uart(rx_data, len);
		}
	}
}

void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_RX_RDY:
		/* Data received; place into ring buffer */
		ring_buf_put(&ring_buf, evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len);

		break;

	case UART_RX_DISABLED:
		/* Re-enable RX */
		uart_rx_enable(uart_dev, rx_buffer, sizeof(rx_buffer), RECEIVE_TIMEOUT);

		break;

	default:
		break;
	}
}

int main(void)
{
	if (!uart_dev) {
		printk("Failed to get UART device");
		return 1;
	}

	/* uart configuration parameters */
	int err = uart_configure(uart_dev, &uart_cfg);

	if (err == -ENOSYS) {
		printk("Configuration is not supported by device or driver,"
			" check the UART settings configuration\n");
		return -ENOSYS;
	}

	/* Configure uart callback */
	uart_callback_set(uart_dev, uart_cb, NULL);

	/* enable uart reception */
	uart_rx_enable(uart_dev, rx_buffer, sizeof(rx_buffer), RECEIVE_TIMEOUT);

	printk("\n Enter message to fill RX buffer size :\n");

	/* start uart data processing thread */
	k_tid_t tid = k_thread_create(&uart_rx_thread_data, uart_rx_stack,
				      K_THREAD_STACK_SIZEOF(uart_rx_stack), uart_rx_thread, NULL,
				      NULL, NULL, 5, 0, K_NO_WAIT);
	k_thread_name_set(tid, "RX_thread");
}
