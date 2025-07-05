/*
 * Copyright (c) 2025 Dan Collins
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdint>
#define DT_DRV_COMPAT rpi_pico_ipc_mailbox

#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ipm_rpi_pico, CONFIG_IPM_LOG_LEVEL);

#include <mailbox.h>

struct rpi_pico_mailbox_config {
	void (*irq_config_func)(const struct device *dev);
};

struct rpi_pico_mailbox_data {
	ipm_callback_t cb;
	void *user_data;
};

static struct rpi_pico_mailbox_config rpi_pico_mailbox_config;
static struct rpi_pico_mailbox_data rpi_pico_mailbox_data;

static int rpi_pico_mailbox_send(const struct device *dev, int wait, uint32_t id, const void *data, int size)
{
	ARG_UNUSED(data);
	ARG_UNUSED(size);

    return 0;
}

static void rpi_pico_mailbox_register_callback(const struct device *dev, ipm_callback_t cb, void *user_data)
{
	struct rpi_pico_mailbox_data *data = dev->data;

	data->cb = cb;
	data->user_data = user_data;
}

static int rpi_pico_mailbox_max_data_size_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* FIFO mailbox allows a single 32 bit value to be sent - and we
	 * use that as the channel identifier. */
	return 0;
}

static unsigned int rpi_pico_mailbox_max_id_val_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* FIFO mailbox allows a single 32 bit value to be sent - and we
	 * use that as the channel identifier. */
	return UINT32_MAX;
}

static int rpi_pico_mailbox_set_enabled(const struct device *dev, int enable)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(enable);
	return 0;
}


static int rpi_pico_mailbox_init(const struct device *dev)
{
	return 0;
}

static DEVICE_API(ipm, rpi_pico_mailbox_driver_api) = {
	.send = rpi_pico_mailbox_send,
	.register_callback = rpi_pico_mailbox_register_callback,
	.max_data_size_get = rpi_pico_mailbox_max_data_size_get,
	.max_id_val_get = rpi_pico_mailbox_max_id_val_get,
	.set_enabled = rpi_pico_mailbox_set_enabled,
};

DEVICE_DT_INST_DEFINE(0,
	&rpi_pico_mailbox_init,
	NULL,
	&rpi_pico_mailbox_data, &rpi_pico_mailbox_config,
	POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
	&rpi_pico_mailbox_driver_api);
