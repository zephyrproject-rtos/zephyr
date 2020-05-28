/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_lpi2c

#include <errno.h>
#include <drivers/i2c.h>
#include <drivers/clock_control.h>
#include <fsl_lpi2c.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(mcux_lpi2c);

#include "i2c-priv.h"

struct mcux_lpi2c_config {
	LPI2C_Type *base;
	char *clock_name;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(struct device *dev);
	uint32_t bitrate;
	uint32_t bus_idle_timeout_ns;
};

struct mcux_lpi2c_data {
	lpi2c_master_handle_t handle;
	struct k_sem device_sync_sem;
	status_t callback_status;
};

static int mcux_lpi2c_configure(struct device *dev, uint32_t dev_config_raw)
{
	const struct mcux_lpi2c_config *config = dev->config;
	LPI2C_Type *base = config->base;
	struct device *clock_dev;
	uint32_t clock_freq;
	uint32_t baudrate;

	if (!(I2C_MODE_MASTER & dev_config_raw)) {
		return -EINVAL;
	}

	if (I2C_ADDR_10_BITS & dev_config_raw) {
		return -EINVAL;
	}

	switch (I2C_SPEED_GET(dev_config_raw)) {
	case I2C_SPEED_STANDARD:
		baudrate = KHZ(100);
		break;
	case I2C_SPEED_FAST:
		baudrate = KHZ(400);
		break;
	case I2C_SPEED_FAST_PLUS:
		baudrate = MHZ(1);
		break;
	default:
		return -EINVAL;
	}

	clock_dev = device_get_binding(config->clock_name);
	if (clock_dev == NULL) {
		return -EINVAL;
	}

	if (clock_control_get_rate(clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	LPI2C_MasterSetBaudRate(base, clock_freq, baudrate);

	return 0;
}

static void mcux_lpi2c_master_transfer_callback(LPI2C_Type *base,
		lpi2c_master_handle_t *handle, status_t status, void *userData)
{
	struct device *dev = userData;
	struct mcux_lpi2c_data *data = dev->data;

	ARG_UNUSED(handle);
	ARG_UNUSED(base);

	data->callback_status = status;
	k_sem_give(&data->device_sync_sem);
}

static uint32_t mcux_lpi2c_convert_flags(int msg_flags)
{
	uint32_t flags = 0U;

	if (!(msg_flags & I2C_MSG_STOP)) {
		flags |= kLPI2C_TransferNoStopFlag;
	}

	if (msg_flags & I2C_MSG_RESTART) {
		flags |= kLPI2C_TransferRepeatedStartFlag;
	}

	return flags;
}

static int mcux_lpi2c_transfer(struct device *dev, struct i2c_msg *msgs,
		uint8_t num_msgs, uint16_t addr)
{
	const struct mcux_lpi2c_config *config = dev->config;
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = config->base;
	lpi2c_master_transfer_t transfer;
	status_t status;

	/* Iterate over all the messages */
	for (int i = 0; i < num_msgs; i++) {
		if (I2C_MSG_ADDR_10_BITS & msgs->flags) {
			return -ENOTSUP;
		}

		/* Initialize the transfer descriptor */
		transfer.flags = mcux_lpi2c_convert_flags(msgs->flags);

		/* Prevent the controller to send a start condition between
		 * messages, except if explicitly requested.
		 */
		if (i != 0 && !(msgs->flags & I2C_MSG_RESTART)) {
			transfer.flags |= kLPI2C_TransferNoStartFlag;
		}

		transfer.slaveAddress = addr;
		transfer.direction = (msgs->flags & I2C_MSG_READ)
			? kLPI2C_Read : kLPI2C_Write;
		transfer.subaddress = 0;
		transfer.subaddressSize = 0;
		transfer.data = msgs->buf;
		transfer.dataSize = msgs->len;

		/* Start the transfer */
		status = LPI2C_MasterTransferNonBlocking(base,
				&data->handle, &transfer);

		/* Return an error if the transfer didn't start successfully
		 * e.g., if the bus was busy
		 */
		if (status != kStatus_Success) {
			LPI2C_MasterTransferAbort(base, &data->handle);
			return -EIO;
		}

		/* Wait for the transfer to complete */
		k_sem_take(&data->device_sync_sem, K_FOREVER);

		/* Return an error if the transfer didn't complete
		 * successfully. e.g., nak, timeout, lost arbitration
		 */
		if (data->callback_status != kStatus_Success) {
			LPI2C_MasterTransferAbort(base, &data->handle);
			return -EIO;
		}

		/* Move to the next message */
		msgs++;
	}

	return 0;
}

static void mcux_lpi2c_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct mcux_lpi2c_config *config = dev->config;
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = config->base;

	LPI2C_MasterTransferHandleIRQ(base, &data->handle);
}

static int mcux_lpi2c_init(struct device *dev)
{
	const struct mcux_lpi2c_config *config = dev->config;
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = config->base;
	struct device *clock_dev;
	uint32_t clock_freq, bitrate_cfg;
	lpi2c_master_config_t master_config;
	int error;

	k_sem_init(&data->device_sync_sem, 0, UINT_MAX);

	clock_dev = device_get_binding(config->clock_name);
	if (clock_dev == NULL) {
		return -EINVAL;
	}

	if (clock_control_get_rate(clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	LPI2C_MasterGetDefaultConfig(&master_config);
	master_config.busIdleTimeout_ns = config->bus_idle_timeout_ns;
	LPI2C_MasterInit(base, &master_config, clock_freq);
	LPI2C_MasterTransferCreateHandle(base, &data->handle,
			mcux_lpi2c_master_transfer_callback, dev);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);

	error = mcux_lpi2c_configure(dev, I2C_MODE_MASTER | bitrate_cfg);
	if (error) {
		return error;
	}

	config->irq_config_func(dev);

	return 0;
}

static const struct i2c_driver_api mcux_lpi2c_driver_api = {
	.configure = mcux_lpi2c_configure,
	.transfer = mcux_lpi2c_transfer,
};

#define I2C_MCUX_LPI2C_INIT(n)						\
	static void mcux_lpi2c_config_func_##n(struct device *dev);	\
									\
	static const struct mcux_lpi2c_config mcux_lpi2c_config_##n = {	\
		.base = (LPI2C_Type *)DT_INST_REG_ADDR(n),		\
		.clock_name = DT_INST_CLOCKS_LABEL(n),			\
		.clock_subsys =						\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
		.irq_config_func = mcux_lpi2c_config_func_##n,		\
		.bitrate = DT_INST_PROP(n, clock_frequency),		\
		.bus_idle_timeout_ns =					\
			UTIL_AND(DT_INST_NODE_HAS_PROP(n, bus_idle_timeout),\
				 DT_INST_PROP(n, bus_idle_timeout)),	\
	};								\
									\
	static struct mcux_lpi2c_data mcux_lpi2c_data_##n;		\
									\
	DEVICE_AND_API_INIT(mcux_lpi2c_##n, DT_INST_LABEL(n),		\
			    &mcux_lpi2c_init, &mcux_lpi2c_data_##n,	\
			    &mcux_lpi2c_config_##n, POST_KERNEL,	\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &mcux_lpi2c_driver_api);			\
									\
	static void mcux_lpi2c_config_func_##n(struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    mcux_lpi2c_isr,				\
			    DEVICE_GET(mcux_lpi2c_##n), 0);		\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_MCUX_LPI2C_INIT)
