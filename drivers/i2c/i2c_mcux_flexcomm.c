/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_i2c

#include <errno.h>
#include <drivers/i2c.h>
#include <drivers/clock_control.h>
#include <fsl_i2c.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(mcux_flexcomm);

#include "i2c-priv.h"

struct mcux_flexcomm_config {
	I2C_Type *base;
	void (*irq_config_func)(const struct device *dev);
	uint32_t bitrate;
};

struct mcux_flexcomm_data {
	i2c_master_handle_t handle;
	struct k_sem device_sync_sem;
	status_t callback_status;
};

static int mcux_flexcomm_configure(const struct device *dev,
				   uint32_t dev_config_raw)
{
	const struct mcux_flexcomm_config *config = dev->config;
	I2C_Type *base = config->base;
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

	clock_freq = MHZ(12);

	I2C_MasterSetBaudRate(base, baudrate, clock_freq);

	return 0;
}

static void mcux_flexcomm_master_transfer_callback(I2C_Type *base,
						   i2c_master_handle_t *handle,
						   status_t status,
						   void *userData)
{
	struct mcux_flexcomm_data *data = userData;

	ARG_UNUSED(handle);
	ARG_UNUSED(base);

	data->callback_status = status;
	k_sem_give(&data->device_sync_sem);
}

static uint32_t mcux_flexcomm_convert_flags(int msg_flags)
{
	uint32_t flags = 0U;

	if (!(msg_flags & I2C_MSG_STOP)) {
		flags |= kI2C_TransferNoStopFlag;
	}

	if (msg_flags & I2C_MSG_RESTART) {
		flags |= kI2C_TransferRepeatedStartFlag;
	}

	return flags;
}

static int mcux_flexcomm_transfer(const struct device *dev,
				  struct i2c_msg *msgs,
				  uint8_t num_msgs, uint16_t addr)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	I2C_Type *base = config->base;
	i2c_master_transfer_t transfer;
	status_t status;

	/* Iterate over all the messages */
	for (int i = 0; i < num_msgs; i++) {
		if (I2C_MSG_ADDR_10_BITS & msgs->flags) {
			return -ENOTSUP;
		}

		/* Initialize the transfer descriptor */
		transfer.flags = mcux_flexcomm_convert_flags(msgs->flags);

		/* Prevent the controller to send a start condition between
		 * messages, except if explicitly requested.
		 */
		if (i != 0 && !(msgs->flags & I2C_MSG_RESTART)) {
			transfer.flags |= kI2C_TransferNoStartFlag;
		}

		transfer.slaveAddress = addr;
		transfer.direction = (msgs->flags & I2C_MSG_READ)
			? kI2C_Read : kI2C_Write;
		transfer.subaddress = 0;
		transfer.subaddressSize = 0;
		transfer.data = msgs->buf;
		transfer.dataSize = msgs->len;

		/* Start the transfer */
		status = I2C_MasterTransferNonBlocking(base,
				&data->handle, &transfer);

		/* Return an error if the transfer didn't start successfully
		 * e.g., if the bus was busy
		 */
		if (status != kStatus_Success) {
			I2C_MasterTransferAbort(base, &data->handle);
			return -EIO;
		}

		/* Wait for the transfer to complete */
		k_sem_take(&data->device_sync_sem, K_FOREVER);

		/* Return an error if the transfer didn't complete
		 * successfully. e.g., nak, timeout, lost arbitration
		 */
		if (data->callback_status != kStatus_Success) {
			I2C_MasterTransferAbort(base, &data->handle);
			return -EIO;
		}

		/* Move to the next message */
		msgs++;
	}

	return 0;
}

static void mcux_flexcomm_isr(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	I2C_Type *base = config->base;

	I2C_MasterTransferHandleIRQ(base, &data->handle);
}

static int mcux_flexcomm_init(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	I2C_Type *base = config->base;
	uint32_t clock_freq, bitrate_cfg;
	i2c_master_config_t master_config;
	int error;

	k_sem_init(&data->device_sync_sem, 0, UINT_MAX);

	clock_freq = MHZ(12);

	I2C_MasterGetDefaultConfig(&master_config);
	I2C_MasterInit(base, &master_config, clock_freq);
	I2C_MasterTransferCreateHandle(base, &data->handle,
				       mcux_flexcomm_master_transfer_callback,
				       data);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);

	error = mcux_flexcomm_configure(dev, I2C_MODE_MASTER | bitrate_cfg);
	if (error) {
		return error;
	}

	config->irq_config_func(dev);

	return 0;
}

static const struct i2c_driver_api mcux_flexcomm_driver_api = {
	.configure = mcux_flexcomm_configure,
	.transfer = mcux_flexcomm_transfer,
};

#define I2C_MCUX_FLEXCOMM_DEVICE(id)					\
	static void mcux_flexcomm_config_func_##id(const struct device *dev); \
	static const struct mcux_flexcomm_config mcux_flexcomm_config_##id = {	\
		.base = (I2C_Type *) DT_INST_REG_ADDR(id),		\
		.irq_config_func = mcux_flexcomm_config_func_##id,	\
		.bitrate = DT_INST_PROP(id, clock_frequency),		\
	};								\
	static struct mcux_flexcomm_data mcux_flexcomm_data_##id;	\
	DEVICE_AND_API_INIT(mcux_flexcomm_##id,				\
			    DT_INST_LABEL(id),				\
			    &mcux_flexcomm_init,			\
			    &mcux_flexcomm_data_##id,			\
			    &mcux_flexcomm_config_##id,			\
			    POST_KERNEL,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &mcux_flexcomm_driver_api);			\
	static void mcux_flexcomm_config_func_##id(const struct device *dev) \
	{								\
		IRQ_CONNECT(DT_INST_IRQN(id),				\
			    DT_INST_IRQ(id, priority),			\
			    mcux_flexcomm_isr,				\
			    DEVICE_GET(mcux_flexcomm_##id),		\
			    0);						\
		irq_enable(DT_INST_IRQN(id));				\
	}								\

DT_INST_FOREACH_STATUS_OKAY(I2C_MCUX_FLEXCOMM_DEVICE)
