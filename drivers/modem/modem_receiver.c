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

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mdm_receiver, CONFIG_MODEM_LOG_LEVEL);

#include "modem_receiver.h"

#define MAX_MDM_CTX	CONFIG_MODEM_RECEIVER_MAX_CONTEXTS
#define MAX_READ_SIZE	128

static struct mdm_receiver_context *contexts[MAX_MDM_CTX];

/**
 * @brief  Finds receiver context which manages provided device.
 *
 * @param  *dev: device used by the receiver context.
 *
 * @retval Receiver context or NULL.
 */
static struct mdm_receiver_context *context_from_dev(const struct device *dev)
{
	int i;

	for (i = 0; i < MAX_MDM_CTX; i++) {
		if (contexts[i] && contexts[i]->uart_dev == dev) {
			return contexts[i];
		}
	}

	return NULL;
}

/**
 * @brief  Persists receiver context if there is a free place.
 *
 * @note   Amount of stored receiver contexts is determined by
 *         MAX_MDM_CTX.
 *
 * @param  *ctx: receiver context to persist.
 *
 * @retval 0 if ok, < 0 if error.
 */
static int mdm_receiver_get(struct mdm_receiver_context *ctx)
{
	int i;

	for (i = 0; i < MAX_MDM_CTX; i++) {
		if (!contexts[i]) {
			contexts[i] = ctx;
			return 0;
		}
	}

	return -ENOMEM;
}

/**
 * @brief  Drains UART.
 *
 * @note   Discards remaining data.
 *
 * @param  *ctx: receiver context.
 *
 * @retval None.
 */
static void mdm_receiver_flush(struct mdm_receiver_context *ctx)
{
	uint8_t c;

	__ASSERT(ctx, "invalid ctx");
	__ASSERT(ctx->uart_dev, "invalid ctx device");

	while (uart_fifo_read(ctx->uart_dev, &c, 1) > 0) {
		continue;
	}
}

/**
 * @brief  Receiver UART interrupt handler.
 *
 * @note   Fills contexts ring buffer with received data.
 *         When ring buffer is full the data is discarded.
 *
 * @param  *uart_dev: uart device.
 *
 * @retval None.
 */
static void mdm_receiver_isr(const struct device *uart_dev, void *user_data)
{
	struct mdm_receiver_context *ctx;
	int rx, ret;
	static uint8_t read_buf[MAX_READ_SIZE];

	ARG_UNUSED(user_data);

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
			ret = ring_buf_put(&ctx->rx_rb, read_buf, rx);
			if (ret != rx) {
				LOG_ERR("Rx buffer doesn't have enough space. "
						"Bytes pending: %d, written: %d",
						rx, ret);
				mdm_receiver_flush(ctx);
				k_sem_give(&ctx->rx_sem);
				break;
			}
			k_sem_give(&ctx->rx_sem);
		}
	}
}

/**
 * @brief  Configures receiver context and assigned device.
 *
 * @param  *ctx: receiver context.
 *
 * @retval None.
 */
static void mdm_receiver_setup(struct mdm_receiver_context *ctx)
{
	__ASSERT(ctx, "invalid ctx");

	uart_irq_rx_disable(ctx->uart_dev);
	uart_irq_tx_disable(ctx->uart_dev);
	mdm_receiver_flush(ctx);
	uart_irq_callback_set(ctx->uart_dev, mdm_receiver_isr);
	uart_irq_rx_enable(ctx->uart_dev);
}

struct mdm_receiver_context *mdm_receiver_context_from_id(int id)
{
	if (id >= 0 && id < MAX_MDM_CTX) {
		return contexts[id];
	} else {
		return NULL;
	}
}

int mdm_receiver_recv(struct mdm_receiver_context *ctx,
		      uint8_t *buf, size_t size, size_t *bytes_read)
{
	if (!ctx) {
		return -EINVAL;
	}

	if (size == 0) {
		*bytes_read = 0;
		return 0;
	}

	*bytes_read = ring_buf_get(&ctx->rx_rb, buf, size);
	return 0;
}

int mdm_receiver_send(struct mdm_receiver_context *ctx,
		      const uint8_t *buf, size_t size)
{
	if (!ctx) {
		return -EINVAL;
	}

	if (size == 0) {
		return 0;
	}

	do {
		uart_poll_out(ctx->uart_dev, *buf++);
	} while (--size);

	return 0;
}

int mdm_receiver_sleep(struct mdm_receiver_context *ctx)
{
	uart_irq_rx_disable(ctx->uart_dev);
#ifdef CONFIG_PM_DEVICE
	pm_device_action_run(ctx->uart_dev, PM_DEVICE_ACTION_SUSPEND);
#endif
	return 0;
}

int mdm_receiver_wake(struct mdm_receiver_context *ctx)
{
#ifdef CONFIG_PM_DEVICE
	pm_device_action_run(ctx->uart_dev, PM_DEVICE_ACTION_SUSPEND);
#endif
	uart_irq_rx_enable(ctx->uart_dev);

	return 0;
}

int mdm_receiver_register(struct mdm_receiver_context *ctx,
			  const struct device *uart_dev,
			  uint8_t *buf, size_t size)
{
	int ret;

	if ((!ctx) || (size == 0)) {
		return -EINVAL;
	}

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("Device is not ready: %s",
			uart_dev ? uart_dev->name : "<null>");
		return -ENODEV;
	}

	ctx->uart_dev = uart_dev;
	ring_buf_init(&ctx->rx_rb, size, buf);
	k_sem_init(&ctx->rx_sem, 0, 1);

	ret = mdm_receiver_get(ctx);
	if (ret < 0) {
		return ret;
	}

	mdm_receiver_setup(ctx);
	return 0;
}
