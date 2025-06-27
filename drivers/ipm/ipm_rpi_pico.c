/*
 * Copyright (c) 2025 Dan Collins
 * Copyright (c) 2025 Dmitrii Sharshakov <d3dx12.xx@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_sio_fifo

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/ipm.h>

#include <zephyr/logging/log.h>
#include "hardware/structs/sio.h"

LOG_MODULE_REGISTER(ipm_rpi_pico, CONFIG_IPM_LOG_LEVEL);

struct rpi_pico_mailbox_config {
	sio_hw_t *const sio_regs;
};

struct rpi_pico_ipm_data {
	ipm_callback_t cb;
	void *user_data;
};

static struct rpi_pico_mailbox_config rpi_pico_sio_mailbox_config = {
	.sio_regs = (sio_hw_t *)DT_INST_REG_ADDR_BY_NAME(0, sio),
};
static struct rpi_pico_ipm_data rpi_pico_mailbox_data;

static int rpi_pico_mailbox_send(const struct device *dev, int wait, uint32_t id,
	const void *data, int size)
{
	ARG_UNUSED(data);

	if (size != 0) {
		return -EMSGSIZE;
	}

	if (id > UINT32_MAX) {
		return -EINVAL;
	}

	const struct rpi_pico_mailbox_config *config =
		(const struct rpi_pico_mailbox_config *)dev->config;

	if (!(config->sio_regs->fifo_st & SIO_FIFO_ST_RDY_BITS) && !wait) {
		LOG_ERR("Mailbox FIFO is full, cannot send message");

		return -EBUSY;
	}

	while (!(config->sio_regs->fifo_st & SIO_FIFO_ST_RDY_BITS)) {
	}

	config->sio_regs->fifo_wr = id;

	/* Inform other CPU about FIFO update. */
	__SEV();

	return 0;
}

static void rpi_pico_mailbox_register_callback(const struct device *dev, ipm_callback_t cb,
					       void *user_data)
{
	struct rpi_pico_ipm_data *data = dev->data;

	data->cb = cb;
	data->user_data = user_data;
}

static int rpi_pico_mailbox_max_data_size_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	/*
	 * FIFO mailbox allows a single 32 bit value to be sent - and we
	 * use that as the channel identifier.
	 */
	return 0;
}

static unsigned int rpi_pico_mailbox_max_id_val_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	/*
	 * FIFO mailbox allows a single 32 bit value to be sent - and we
	 * use that as the channel identifier.
	 */
	return UINT32_MAX;
}

static int rpi_pico_mailbox_set_enabled(const struct device *dev, int enable)
{
	ARG_UNUSED(dev);

	if (enable) {
		irq_enable(DT_INST_IRQN(0));
	} else {
		irq_disable(DT_INST_IRQN(0));
	}

	return 0;
}

static void rpi_pico_mailbox_isr(const struct device *dev)
{
	const struct rpi_pico_mailbox_config *config =
		(const struct rpi_pico_mailbox_config *)dev->config;

	/* Clear status */
	config->sio_regs->fifo_st = 0xff;

	while (config->sio_regs->fifo_st & SIO_FIFO_ST_VLD_BITS) {
		uint32_t msg = config->sio_regs->fifo_rd;
		struct rpi_pico_ipm_data *data = dev->data;

		if (data->cb) {
			/* Only send the channel ID to the callback, no data. */
			data->cb(dev, data->user_data, msg, 0);
		}
	}
}

static int rpi_pico_mailbox_init(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, sio_irq_fifo, irq),
		    DT_INST_IRQ_BY_NAME(0, sio_irq_fifo, priority), rpi_pico_mailbox_isr,
		    DEVICE_DT_INST_GET(0), 0);

	return 0;
}

static DEVICE_API(ipm, rpi_pico_mailbox_driver_api) = {
	.send = rpi_pico_mailbox_send,
	.register_callback = rpi_pico_mailbox_register_callback,
	.max_data_size_get = rpi_pico_mailbox_max_data_size_get,
	.max_id_val_get = rpi_pico_mailbox_max_id_val_get,
	.set_enabled = rpi_pico_mailbox_set_enabled,
};

DEVICE_DT_INST_DEFINE(0, &rpi_pico_mailbox_init, NULL, &rpi_pico_mailbox_data,
		      &rpi_pico_sio_mailbox_config, POST_KERNEL,
			  CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &rpi_pico_mailbox_driver_api);
