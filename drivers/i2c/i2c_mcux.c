/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_i2c

#include <errno.h>
#include <drivers/i2c.h>
#include <soc.h>
#include <fsl_i2c.h>
#include <fsl_clock.h>
#include <sys/util.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_mcux);

#include "i2c-priv.h"

#define DEV_CFG(dev) \
	((const struct i2c_mcux_config * const)(dev)->config)
#define DEV_DATA(dev) \
	((struct i2c_mcux_data * const)(dev)->data)
#define DEV_BASE(dev) \
	((I2C_Type *)(DEV_CFG(dev))->base)

struct i2c_mcux_config {
	I2C_Type *base;
	clock_name_t clock_source;
	void (*irq_config_func)(struct device *dev);
	uint32_t bitrate;
};

struct i2c_mcux_data {
	i2c_master_handle_t handle;
	struct k_sem device_sync_sem;
	status_t callback_status;
};

static int i2c_mcux_configure(struct device *dev, uint32_t dev_config_raw)
{
	I2C_Type *base = DEV_BASE(dev);
	const struct i2c_mcux_config *config = DEV_CFG(dev);
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

	clock_freq = CLOCK_GetFreq(config->clock_source);
	I2C_MasterSetBaudRate(base, baudrate, clock_freq);

	return 0;
}

static void i2c_mcux_master_transfer_callback(I2C_Type *base,
		i2c_master_handle_t *handle, status_t status, void *userData)
{
	struct device *dev = userData;
	struct i2c_mcux_data *data = DEV_DATA(dev);

	ARG_UNUSED(handle);
	ARG_UNUSED(base);

	data->callback_status = status;
	k_sem_give(&data->device_sync_sem);
}

static uint32_t i2c_mcux_convert_flags(int msg_flags)
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

static int i2c_mcux_transfer(struct device *dev, struct i2c_msg *msgs,
		uint8_t num_msgs, uint16_t addr)
{
	I2C_Type *base = DEV_BASE(dev);
	struct i2c_mcux_data *data = DEV_DATA(dev);
	i2c_master_transfer_t transfer;
	status_t status;

	/* Iterate over all the messages */
	for (int i = 0; i < num_msgs; i++) {
		if (I2C_MSG_ADDR_10_BITS & msgs->flags) {
			return -ENOTSUP;
		}

		/* Initialize the transfer descriptor */
		transfer.flags = i2c_mcux_convert_flags(msgs->flags);
		transfer.slaveAddress = addr;
		transfer.direction = (msgs->flags & I2C_MSG_READ)
			? kI2C_Read : kI2C_Write;
		transfer.subaddress = 0;
		transfer.subaddressSize = 0;
		transfer.data = msgs->buf;
		transfer.dataSize = msgs->len;

		/* Prevent the controller to send a start condition between
		 * messages, except if explicitly requested.
		 */
		if (i != 0 && !(msgs->flags & I2C_MSG_RESTART)) {
			transfer.flags |= kI2C_TransferNoStartFlag;
		}

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

static void i2c_mcux_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	I2C_Type *base = DEV_BASE(dev);
	struct i2c_mcux_data *data = DEV_DATA(dev);

	I2C_MasterTransferHandleIRQ(base, &data->handle);
}

static int i2c_mcux_init(struct device *dev)
{
	I2C_Type *base = DEV_BASE(dev);
	const struct i2c_mcux_config *config = DEV_CFG(dev);
	struct i2c_mcux_data *data = DEV_DATA(dev);
	uint32_t clock_freq, bitrate_cfg;
	i2c_master_config_t master_config;
	int error;

	k_sem_init(&data->device_sync_sem, 0, UINT_MAX);

	clock_freq = CLOCK_GetFreq(config->clock_source);
	I2C_MasterGetDefaultConfig(&master_config);
	I2C_MasterInit(base, &master_config, clock_freq);
	I2C_MasterTransferCreateHandle(base, &data->handle,
			i2c_mcux_master_transfer_callback, dev);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);

	error = i2c_mcux_configure(dev, I2C_MODE_MASTER | bitrate_cfg);
	if (error) {
		return error;
	}

	config->irq_config_func(dev);

	return 0;
}

static const struct i2c_driver_api i2c_mcux_driver_api = {
	.configure = i2c_mcux_configure,
	.transfer = i2c_mcux_transfer,
};

#define I2C_DEVICE_INIT_MCUX(n)			\
	static void i2c_mcux_config_func_ ## n(struct device *dev);	\
									\
	static const struct i2c_mcux_config i2c_mcux_config_ ## n = {	\
		.base = (I2C_Type *)DT_INST_REG_ADDR(n),\
		.clock_source = I2C ## n ## _CLK_SRC,			\
		.irq_config_func = i2c_mcux_config_func_ ## n,		\
		.bitrate = DT_INST_PROP(n, clock_frequency),		\
	};								\
									\
	static struct i2c_mcux_data i2c_mcux_data_ ## n;		\
									\
	DEVICE_AND_API_INIT(i2c_mcux_ ## n,				\
			DT_INST_LABEL(n),				\
			&i2c_mcux_init, &i2c_mcux_data_ ## n,		\
			&i2c_mcux_config_ ## n, POST_KERNEL,		\
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			&i2c_mcux_driver_api);				\
									\
	static void i2c_mcux_config_func_ ## n(struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			DT_INST_IRQ(n, priority),			\
			i2c_mcux_isr,					\
			DEVICE_GET(i2c_mcux_ ## n), 0);			\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_DEVICE_INIT_MCUX)
