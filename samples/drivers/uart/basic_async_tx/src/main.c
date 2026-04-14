/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

/* UART1 device (will use PA17 as TX pin) */
#define UART1_NODE DT_NODELABEL(uart1)

/* Message counter */
static volatile int msg_counter = 1;

/* TX in progress flag */
static volatile bool tx_in_progress;

/* TX buffer */
static char tx_buffer[64];

/* UART callback function - silent version */
static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		/* TX completed - clear the flag so next message can be sent */
		tx_in_progress = false;
		break;

	case UART_TX_ABORTED:
		/* TX aborted - clear the flag so next message can be sent */
		tx_in_progress = false;
		break;

	default:
		/* Ignore other UART events */
		break;
	}
}

/* Function to send a message asynchronously */
static void send_message_async(const struct device *dev, const char *message)
{
	size_t len = strlen(message);

	/* Copy message to TX buffer */
	strncpy(tx_buffer, message, sizeof(tx_buffer) - 1);
	tx_buffer[sizeof(tx_buffer) - 1] = '\0';

	/* Set flag to indicate TX in progress */
	tx_in_progress = true;

	/* Send the message asynchronously */
	int ret = uart_tx(dev, tx_buffer, len, 500000); /* 500ms timeout */
	if (ret < 0) {
		/* Failed to start transmission - clear flag */
		tx_in_progress = false;
	}
}

void main(void)
{
	char message[64];
	const struct device *uart1_dev = DEVICE_DT_GET(UART1_NODE);

	/* Wait for device to be ready */
	if (!device_is_ready(uart1_dev)) {
		return;
	}

	/* Register callback */
	uart_callback_set(uart1_dev, uart_callback, NULL);

	/* Main loop - send a message every 2 seconds */
	while (1) {
		/* Wait until any previous TX is complete */
		if (!tx_in_progress) {
			/* Format a simple message */
			snprintk(message, sizeof(message), "Test %d\r\n", msg_counter++);

			/* Send it asynchronously */
			send_message_async(uart1_dev, message);
		}

		/* Wait 2 seconds before sending the next message */
		k_sleep(K_SECONDS(1));
	}
}
