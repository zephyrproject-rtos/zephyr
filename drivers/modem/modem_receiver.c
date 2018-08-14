/** @file
 * @brief Modem receiver driver
 *
 * A modem receiver driver allowing application to handle all
 * aspects of received protocol data.
 */

/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "mdm_receiver"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_MODEM_LEVEL

#include <kernel.h>
#include <init.h>
#include <uart.h>

#include <logging/sys_log.h>
#include <drivers/modem/modem_receiver.h>

#define MAX_MDM_CTX	CONFIG_MODEM_RECEIVER_MAX_CONTEXTS
#define MAX_READ_SIZE	128

static struct mdm_receiver_context *contexts[MAX_MDM_CTX];

struct mdm_receiver_context *mdm_receiver_context_from_id(int id)
{
	if (id >= 0 && id < MAX_MDM_CTX) {
		return contexts[id];
	} else {
		return NULL;
	}
}

static struct mdm_receiver_context *context_from_dev(struct device *dev)
{
	int i;

	for (i = 0; i < MAX_MDM_CTX; i++) {
		if (contexts[i] && contexts[i]->uart_dev == dev) {
			return contexts[i];
		}
	}

	return NULL;
}

static int mdm_receiver_get(struct mdm_receiver_context *ctx)
{
	int i;

	/* find a free modem_context */
	for (i = 0; i < MAX_MDM_CTX; i++) {
		if (!contexts[i]) {
			contexts[i] = ctx;
			return 0;
		}
	}

	return -ENOMEM;
}

static void mdm_receiver_flush(struct mdm_receiver_context *ctx)
{
	u8_t c;

	if (!ctx) {
		return;
	}

	/* Drain the fifo */
	while (uart_fifo_read(ctx->uart_dev, &c, 1) > 0) {
		continue;
	}

	/* clear the UART pipe */
	k_pipe_init(&ctx->uart_pipe, ctx->uart_pipe_buf, ctx->uart_pipe_size);
}

static void mdm_receiver_isr(struct device *uart_dev)
{
	struct mdm_receiver_context *ctx;
	int rx, ret;
	size_t bytes_written;
	static u8_t read_buf[MAX_READ_SIZE];

	/* lookup the device */
	ctx = context_from_dev(uart_dev);
	if (!ctx) {
		return;
	}

	/* get all of the data off UART as fast as we can */
	while (uart_irq_update(ctx->uart_dev) &&
	       uart_irq_rx_ready(ctx->uart_dev)) {
		rx = uart_fifo_read(ctx->uart_dev, read_buf, sizeof(read_buf));
		if (rx > 0) {
			ret = k_pipe_put(&ctx->uart_pipe, read_buf, rx,
					 &bytes_written, rx, K_NO_WAIT);
			if (ret < 0) {
				SYS_LOG_ERR("UART buffer write error (%d)! "
					    "Flushing UART!", ret);
				mdm_receiver_flush(ctx);
				return;
			}

			k_sem_give(&ctx->rx_sem);
		}
	}
}

int mdm_receiver_recv(struct mdm_receiver_context *ctx,
		      u8_t *buf, size_t size, size_t *bytes_read)
{
	if (!ctx) {
		return -EINVAL;
	}

	return k_pipe_get(&ctx->uart_pipe, buf, size, bytes_read, 1, K_NO_WAIT);
}

int mdm_receiver_send(struct mdm_receiver_context *ctx,
		      const u8_t *buf, size_t size)
{
	if (!ctx) {
		return -EINVAL;
	}

	while (size)  {
		int written;

		written = uart_fifo_fill(ctx->uart_dev,
					 (const u8_t *)buf, size);
		if (written < 0) {
			/* error */
			uart_irq_tx_disable(ctx->uart_dev);
			return written;
		} else if (written < size) {
			k_yield();
		}

		size -= written;
		buf += written;
	}

	return 0;
}

static void mdm_receiver_setup(struct mdm_receiver_context *ctx)
{
	if (!ctx) {
		return;
	}

	uart_irq_rx_disable(ctx->uart_dev);
	uart_irq_tx_disable(ctx->uart_dev);
	mdm_receiver_flush(ctx);
	uart_irq_callback_set(ctx->uart_dev, mdm_receiver_isr);
	uart_irq_rx_enable(ctx->uart_dev);
}

int mdm_receiver_register(struct mdm_receiver_context *ctx,
			  const char *uart_dev_name,
			  u8_t *buf, size_t size)
{
	int ret;

	if (!ctx) {
		return -EINVAL;
	}

	ctx->uart_dev = device_get_binding(uart_dev_name);
	if (!ctx->uart_dev) {
		return -ENOENT;
	}

	/* k_pipe is setup later in mdm_receiver_flush() */
	ctx->uart_pipe_buf = buf;
	ctx->uart_pipe_size = size;
	k_sem_init(&ctx->rx_sem, 0, 1);

	ret = mdm_receiver_get(ctx);
	if (ret < 0) {
		return ret;
	}

	mdm_receiver_setup(ctx);
	return 0;
}
