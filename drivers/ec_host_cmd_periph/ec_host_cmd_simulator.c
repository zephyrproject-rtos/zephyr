/*
 * Copyright (c) 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_sim_ec_host_cmd_periph

#include <device.h>
#include <drivers/ec_host_cmd_periph.h>
#include <string.h>

#ifndef CONFIG_ARCH_POSIX
#error Simulator only valid on posix
#endif

static uint8_t rx_buffer[256];
static size_t rx_buffer_len;

/* Allow writing to rx buff at startup and block on reading. */
static K_SEM_DEFINE(handler_owns, 0, 1);
static K_SEM_DEFINE(dev_owns, 1, 1);

static ec_host_cmd_periph_api_send tx;

int ec_host_cmd_periph_sim_init(const struct device *dev,
				struct ec_host_cmd_periph_rx_ctx *rx_ctx)
{
	if (rx_ctx == NULL) {
		return -EINVAL;
	}

	rx_ctx->buf = rx_buffer;
	rx_ctx->len = &rx_buffer_len;
	rx_ctx->dev_owns = &dev_owns;
	rx_ctx->handler_owns = &handler_owns;

	return 0;
}

int ec_host_cmd_periph_sim_send(const struct device *dev,
				const struct ec_host_cmd_periph_tx_buf *buf)
{
	if (tx != NULL) {
		return tx(dev, buf);
	}

	return 0;
}

void ec_host_cmd_periph_sim_install_send_cb(ec_host_cmd_periph_api_send cb)
{
	tx = cb;
}

int ec_host_cmd_periph_sim_data_received(const uint8_t *buffer, size_t len)
{
	if (sizeof(rx_buffer) < len) {
		return -ENOMEM;
	}
	if (k_sem_take(&dev_owns, K_NO_WAIT) != 0) {
		return -EBUSY;
	}

	memcpy(rx_buffer, buffer, len);
	rx_buffer_len = len;

	k_sem_give(&handler_owns);
	return 0;
}

static const struct ec_host_cmd_periph_api ec_host_cmd_api = {
	.init = &ec_host_cmd_periph_sim_init,
	.send = &ec_host_cmd_periph_sim_send,
};

static int ec_host_cmd_sim_init(const struct device *dev)
{
	return 0;
}

/* Assume only one simulator */
DEVICE_DT_INST_DEFINE(0, ec_host_cmd_sim_init, device_pm_control_nop,
		    NULL, NULL, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ec_host_cmd_api);
