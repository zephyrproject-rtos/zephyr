/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

uint8_t DEBUG; /* Set to 1 to see buffer switching details */

/* UART nodes */
#define UART0_NODE DT_NODELABEL(uart0) /* Console - PA10/PA11 */
#define UART1_NODE DT_NODELABEL(uart1) /* Receiver - PA18 (RX) */

/* Dual RX buffers for buffer switching */
#define RX_BUF_SIZE 128
static uint8_t rx_buf[2][RX_BUF_SIZE];         /* Two buffers in an array like async_api */
static volatile uint8_t rx_buf_idx;        /* Tracks which buffer index to provide next */
static volatile uint8_t active_buf_idx;    /* Tracks which buffer index is active */
static volatile uint8_t active_buf_offset; /* Tracks valid data offset */
static volatile uint8_t active_buf_len;    /* Tracks valid data length */

/* UART devices */
static const struct device *uart0_dev; /* For console output */
static const struct device *uart1_dev; /* For RX on PA18 */

/* Semaphore for signaling data ready */
static K_SEM_DEFINE(rx_data_sem, 0, 1); /* Initial count 0, max count 1 */

/* Tracking stats */
static volatile int rx_bytes_received;
static volatile int rx_events_count;

/* Print characters to console, interpreting common control chars */
static void print_received_data(const uint8_t *data, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		char c = data[i];

		/* Handle specific control characters */
		if (c == 0x0A) {
			/* Line feed (LF) - print actual newline */
			printk("\n");
		} else if (c == 0x0D) {
			/* Carriage return (CR) - print actual carriage return */
			printk("\r");
		} else if (c < 32 || c > 126) {
			/* Other non-printable characters - escape them */
			printk("\\x%02X", c);
		} else {
			/* Normal printable characters */
			printk("%c", c);
		}
	}
}

/* UART1 callback for async reception */
static void uart1_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	int ret;

	/* Increment event counter */
	rx_events_count++;

	if (!evt) {
		printk("Error: NULL event received\n");
		printk("DEBUG value: %d\n", DEBUG);
		return;
	}

	if (DEBUG == 1) {
		printk("===============================================\n");
		printk("DEBUG value: %d\n", DEBUG);
	}

	switch (evt->type) {
	case UART_TX_DONE:
		/* No TX in this example, but keep for completeness */
		break;

	case UART_RX_RDY:
		/* Data is ready in the buffer */
		if (DEBUG == 1) {
			printk("RX data ready: %d bytes (offset: %d)\n", evt->data.rx.len,
			       evt->data.rx.offset);
		}

		/* Identify which buffer is being used */
		if (DEBUG == 1) {
			if (evt->data.rx.buf == rx_buf[0]) {
				printk("Using buffer 0 for RX_RDY\n");
			} else if (evt->data.rx.buf == rx_buf[1]) {
				printk("Using buffer 1 for RX_RDY\n");
			} else {
				printk("Using unknown buffer for RX_RDY\n");
			}
			printk("DEBUG value: %d\n", DEBUG);
		}

		if (evt->data.rx.buf == rx_buf[0]) {
			active_buf_idx = 0;
		} else {
			active_buf_idx = 1;
		}
		active_buf_len = evt->data.rx.len;
		active_buf_offset = evt->data.rx.offset;

		/* Signal that data is ready to be processed */
		k_sem_give(&rx_data_sem);
		break;

	case UART_RX_BUF_REQUEST:
		/* Driver is requesting another buffer - provide the buffer at the current index */
		if (DEBUG == 1) {
			printk("Providing buffer index %d as next buffer to use\n", rx_buf_idx);
			printk("DEBUG value: %d\n", DEBUG);
		}

		ret = uart_rx_buf_rsp(dev, rx_buf[rx_buf_idx], RX_BUF_SIZE);
		__ASSERT_NO_MSG(ret == 0);

		/* Toggle buffer index using ternary operator exactly like async_api */
		rx_buf_idx = rx_buf_idx ? 0 : 1;
		break;

	case UART_RX_BUF_RELEASED:
		/* Identify which buffer is being released */
		if (DEBUG == 1) {
			if (evt->data.rx_buf.buf == rx_buf[0]) {
				printk("Buffer 0 released\n");
			} else if (evt->data.rx_buf.buf == rx_buf[1]) {
				printk("Buffer 1 released\n");
			} else {
				printk("Unknown buffer released\n");
			}
			printk("DEBUG value: %d\n", DEBUG);
		}
		/* No additional handling needed here like in async_api */
		break;

	case UART_RX_DISABLED:
		/* Like in async_api, we don't need to handle this specially */
		break;

	case UART_RX_STOPPED:
		if (DEBUG == 0) {
			printk("RX stopped due to error: %d\n", evt->data.rx_stop.reason);

			/* Identify which buffer was active when stopped */
			if (evt->data.rx_stop.data.buf == rx_buf[0]) {
				printk("Buffer 0 was active during error\n");
			} else if (evt->data.rx_stop.data.buf == rx_buf[1]) {
				printk("Buffer 1 was active during error\n");
			} else {
				printk("Unknown buffer was active during error\n");
			}
		}

		/* Restart reception with buffer 0 */
		rx_buf_idx = 1;
		ret = uart_rx_enable(dev, rx_buf[0], RX_BUF_SIZE, 1000000);
		if (ret) {
			printk("Failed to restart RX after error: %d\n", ret);
		}
		break;

	default:
		if (DEBUG == 1) {
			printk("Unhandled event %d\n", evt->type);
			printk("DEBUG value: %d\n", DEBUG);
		}
		break;
	}
}

/* Thread for processing received data */
static void rx_processing_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct uart_event evt;
	int ret;

	while (1) {
		/* Wait for data to be ready */
		k_sem_take(&rx_data_sem, K_FOREVER);

		/* READY EVENT PROCESSING*/
		/* Print the actual data */
		print_received_data(&rx_buf[active_buf_idx][active_buf_offset], active_buf_len);

		/* Update total bytes received */
		rx_bytes_received += active_buf_len;
	}
}

/* Define thread stack and thread data */
K_THREAD_STACK_DEFINE(rx_processing_thread_stack, 1024);
static struct k_thread rx_processing_thread_data;

void main(void)
{
	int ret;

	/* Get UART device pointers */
	uart0_dev = DEVICE_DT_GET(UART0_NODE);
	uart1_dev = DEVICE_DT_GET(UART1_NODE);

	printk("\n\n===================================\n");
	printk("Simple Async RX Test\n");
	printk("Receiving on PA18 (UART1 RX)\n");
	printk("===================================\n\n");

	/* Check if UART devices are ready */
	if (!device_is_ready(uart0_dev) || !device_is_ready(uart1_dev)) {
		printk("ERROR: UART not ready\n");
		return;
	}

	/* Set callback for async UART events */
	ret = uart_callback_set(uart1_dev, uart1_callback, NULL);
	if (ret) {
		printk("ERROR: Failed to set callback: %d\n", ret);
		return;
	}

	/* Start async reception with a timeout of 1000000 ms */
	printk("Starting async RX with buffer 0...\n");
	/* Set rx_buf_idx to 1 exactly like in async_api to match the callback logic */
	rx_buf_idx = 1; /* This ensures the next buffer request uses buffer 1 */
	ret = uart_rx_enable(uart1_dev, rx_buf[0], RX_BUF_SIZE, 1000000);
	if (ret) {
		printk("ERROR: Failed to enable RX: %d\n", ret);
		return;
	}

	printk("RX enabled and listening on PA18\n");
	printk("Waiting for data...\n\n");

	/* Start the RX processing thread */
	k_thread_create(&rx_processing_thread_data, rx_processing_thread_stack,
			K_THREAD_STACK_SIZEOF(rx_processing_thread_stack), rx_processing_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	/* Main loop - periodically report status */
	while (1) {
		/* Sleep and report stats every 3 seconds */
		k_sleep(K_SECONDS(3));
	}
}
