/*
 * Copyright (c) 2024 Linumiz 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <zephyr/sys_clock.h>

#define FREE_BUF_MAGIC	0x33
static uint8_t buffer[20] = {0};
static void uart_read_callback(const struct device *dev,
			       struct uart_event *evt, void *user_data)
{
	int err = 0;

	switch (evt->type) {
	case UART_TX_DONE:
		printk("TX Done\n");
		printk("Transmited length = %u\n", evt->data.tx.len);
		break;
	case UART_TX_ABORTED:
		printk("TX ABorted\n");
		break;
	case UART_RX_RDY: {
		uint8_t *buf = evt->data.rx.buf;
		printk("recv length %u\n", evt->data.rx.len);
		printk("%s ", buf);
	}
		break;
	case UART_RX_BUF_RELEASED: {
		uint8_t *buf = evt->data.rx.buf;
		printk("buf released\n");
		memset(buf, 0, 10);
		*buf = FREE_BUF_MAGIC;
		}
		break;
	case UART_RX_BUF_REQUEST: {
		printk("rx buf requested\n");
		uint8_t *buf;

		if (buffer[0] == FREE_BUF_MAGIC) {
			buf = &buffer[0];
		} else {
			buf = &buffer[10];
		}	

		err = uart_rx_buf_rsp(dev, (uint8_t *)buf, 10);
		if (err < 0) {
			printk("Failed to alloc buf %d\n", err);
			break;
		}
		}
		break;
	case UART_RX_DISABLED:
		break;
	case UART_RX_STOPPED:
		break;
	default:
		break;
	}
}

static uint8_t string[] = "This is curel world\n";
int main(void)
{
	int ret;
	const struct device *const uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

	if (!device_is_ready(uart_dev)) {
		printk("UART device not ready\n");
		return -1;
	}

	uart_rx_disable(uart_dev);
	uart_callback_set(uart_dev, uart_read_callback, NULL);
	ret = uart_tx(uart_dev, &string[0], strlen(string), 1000 * USEC_PER_MSEC);
	if (ret < 0) {
		printk("Failed to transfer data %d\n", ret);
		return -1;
	}

	/* sleep for 2 secs */
	k_msleep(2000);
	printk("Ready to receive data\n");

	ret = uart_rx_enable(uart_dev, (uint8_t *)&buffer[0], 10, 500 * USEC_PER_MSEC);
	if (ret < 0) {
		printk("Failed to read uart data %d\n", ret);
		return ret;
	}

	return 0;
}
