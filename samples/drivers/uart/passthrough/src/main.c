/*
 * Copyright (c) 2024 Argentum Systems Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

#include <stdio.h>
#include <string.h>

struct patch_info {
	const uint8_t * const name;

	const struct device *rx_dev;
	struct ring_buf *rx_ring_buf;
	bool rx_error;
	bool rx_overflow;

	const struct device *tx_dev;
};

#define DEV_CONSOLE DEVICE_DT_GET(DT_CHOSEN(zephyr_console))
#define DEV_OTHER   DEVICE_DT_GET(DT_CHOSEN(uart_passthrough))

#define RING_BUF_SIZE 64

RING_BUF_DECLARE(rb_console, RING_BUF_SIZE);
struct patch_info patch_c2o = {
	.name = "c2o",

	.rx_dev = DEV_CONSOLE,
	.rx_ring_buf = &rb_console,
	.rx_error = false,
	.rx_overflow = false,

	.tx_dev = DEV_OTHER,
};

RING_BUF_DECLARE(rb_other, RING_BUF_SIZE);
struct patch_info patch_o2c = {
	.name = "o2c",

	.rx_dev = DEV_OTHER,
	.rx_ring_buf = &rb_other,
	.rx_error = false,
	.rx_overflow = false,

	.tx_dev = DEV_CONSOLE,
};

static void uart_cb(const struct device *dev, void *ctx)
{
	struct patch_info *patch = (struct patch_info *)ctx;
	int ret;
	uint8_t *buf;
	uint32_t len;

	while (uart_irq_update(patch->rx_dev) > 0) {
		ret = uart_irq_rx_ready(patch->rx_dev);
		if (ret < 0) {
			patch->rx_error = true;
		}
		if (ret <= 0) {
			break;
		}

		len = ring_buf_put_claim(patch->rx_ring_buf, &buf, RING_BUF_SIZE);
		if (len == 0) {
			/* no space for Rx, disable the IRQ */
			uart_irq_rx_disable(patch->rx_dev);
			patch->rx_overflow = true;
			break;
		}

		ret = uart_fifo_read(patch->rx_dev, buf, len);
		if (ret < 0) {
			patch->rx_error = true;
		}
		if (ret <= 0) {
			break;
		}
		len = ret;

		ret = ring_buf_put_finish(patch->rx_ring_buf, len);
		if (ret != 0) {
			patch->rx_error = true;
			break;
		}
	}
}

static void passthrough(struct patch_info *patch)
{
	int ret;
	uint8_t *buf;
	uint32_t len;

	if (patch->rx_error) {
		printk("<<%s: Rx Error!>>\n", patch->name);
		patch->rx_error = false;
	}

	if (patch->rx_overflow) {
		printk("<<%s: Rx Overflow!>>\n", patch->name);
		patch->rx_overflow = false;
	}

	len = ring_buf_get_claim(patch->rx_ring_buf, &buf, RING_BUF_SIZE);
	if (len == 0) {
		goto done;
	}

	ret = uart_fifo_fill(patch->tx_dev, buf, len);
	if (ret < 0) {
		goto error;
	}
	len = ret;

	ret = ring_buf_get_finish(patch->rx_ring_buf, len);
	if (ret < 0) {
		goto error;
	}

done:
	uart_irq_rx_enable(patch->rx_dev);
	return;

error:
	printk("<<%s: Tx Error!>>\n", patch->name);
}

int main(void)
{
	printk("Console Device: %p\n", patch_c2o.rx_dev);
	printk("Other Device:   %p\n", patch_o2c.rx_dev);

	uart_irq_callback_user_data_set(patch_c2o.rx_dev, uart_cb, (void *)&patch_c2o);
	uart_irq_callback_user_data_set(patch_o2c.rx_dev, uart_cb, (void *)&patch_o2c);

	for (;;) {
		passthrough(&patch_c2o);
		passthrough(&patch_o2c);
	}

	return 0;
}
