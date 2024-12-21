/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_i2c

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <fsl_i2c.h>
#include <fsl_clock.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_mcux);

#include "i2c-priv.h"

#define DEV_BASE(dev) \
	((I2C_Type *)((const struct i2c_mcux_config * const)(dev)->config)->base)

struct i2c_mcux_config {
	I2C_Type *base;
	clock_name_t clock_source;
	void (*irq_config_func)(const struct device *dev);
	uint32_t bitrate;
	const struct pinctrl_dev_config *pincfg;
};

struct i2c_mcux_data {
	i2c_master_handle_t handle;
	struct k_sem lock;
	struct k_sem device_sync_sem;
	status_t callback_status;
#ifdef CONFIG_I2C_CALLBACK
	uint16_t addr;
	uint32_t msg;
	struct i2c_msg *msgs;
	uint32_t num_msgs;
	i2c_callback_t cb;
	void *userdata;
#endif /* CONFIG_I2C_CALLBACK */
};

static int i2c_mcux_configure(const struct device *dev,
			      uint32_t dev_config_raw)
{
	I2C_Type *base = DEV_BASE(dev);
	struct i2c_mcux_data *data = dev->data;
	const struct i2c_mcux_config *config = dev->config;
	uint32_t clock_freq;
	uint32_t baudrate;

	if (!(I2C_MODE_CONTROLLER & dev_config_raw)) {
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
	k_sem_take(&data->lock, K_FOREVER);
	I2C_MasterSetBaudRate(base, baudrate, clock_freq);
	k_sem_give(&data->lock);

	return 0;
}

#ifdef CONFIG_I2C_CALLBACK

static void i2c_mcux_async_done(const struct device *dev, struct i2c_mcux_data *data, int result);
static void i2c_mcux_async_iter(const struct device *dev);

#endif

static void i2c_mcux_master_transfer_callback(I2C_Type *base,
					      i2c_master_handle_t *handle,
					      status_t status, void *userdata)
{

	ARG_UNUSED(handle);
	ARG_UNUSED(base);

	struct device *dev = userdata;
	struct i2c_mcux_data *data = dev->data;

	#ifdef CONFIG_I2C_CALLBACK
	if (data->cb != NULL) {
		/* Async transfer */
		if (status != kStatus_Success) {
			I2C_MasterTransferAbort(base, &data->handle);
			i2c_mcux_async_done(dev, data, -EIO);
		} else if (data->msg == data->num_msgs - 1) {
			i2c_mcux_async_done(dev, data, 0);
		} else {
			data->msg++;
			i2c_mcux_async_iter(dev);
		}
		return;
	}
	#endif /* CONFIG_I2C_CALLBACK */

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

static int i2c_mcux_transfer(const struct device *dev, struct i2c_msg *msgs,
			     uint8_t num_msgs, uint16_t addr)
{
	I2C_Type *base = DEV_BASE(dev);
	struct i2c_mcux_data *data = dev->data;
	i2c_master_transfer_t transfer;
	status_t status;
	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);

	/* Iterate over all the messages */
	for (int i = 0; i < num_msgs; i++) {
		if (I2C_MSG_ADDR_10_BITS & msgs->flags) {
			ret = -ENOTSUP;
			break;
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
			ret = -EIO;
			break;
		}

		/* Wait for the transfer to complete */
		k_sem_take(&data->device_sync_sem, K_FOREVER);

		/* Return an error if the transfer didn't complete
		 * successfully. e.g., nak, timeout, lost arbitration
		 */
		if (data->callback_status != kStatus_Success) {
			I2C_MasterTransferAbort(base, &data->handle);
			ret = -EIO;
			break;
		}

		/* Move to the next message */
		msgs++;
	}

	k_sem_give(&data->lock);

	return ret;
}

#ifdef CONFIG_I2C_CALLBACK

static void i2c_mcux_async_done(const struct device *dev, struct i2c_mcux_data *data, int result)
{

	i2c_callback_t cb = data->cb;
	void *userdata = data->userdata;

	data->msg = 0;
	data->msgs = NULL;
	data->num_msgs = 0;
	data->cb = NULL;
	data->userdata = NULL;
	data->addr = 0;

	k_sem_give(&data->lock);

	/* Callback may wish to start another transfer */
	cb(dev, result, userdata);
}

/* Start a transfer asynchronously */
static void i2c_mcux_async_iter(const struct device *dev)
{
	I2C_Type *base = DEV_BASE(dev);
	struct i2c_mcux_data *data = dev->data;
	i2c_master_transfer_t transfer;
	status_t status;
	struct i2c_msg *msg = &data->msgs[data->msg];

	if (I2C_MSG_ADDR_10_BITS & msg->flags) {
		i2c_mcux_async_done(dev, data, -ENOTSUP);
		return;
	}

	/* Initialize the transfer descriptor */
	transfer.flags = i2c_mcux_convert_flags(msg->flags);
	transfer.slaveAddress = data->addr;
	transfer.direction = (msg->flags & I2C_MSG_READ) ? kI2C_Read : kI2C_Write;
	transfer.subaddress = 0;
	transfer.subaddressSize = 0;
	transfer.data = msg->buf;
	transfer.dataSize = msg->len;

	/* Prevent the controller to send a start condition between
	 * messages, except if explicitly requested.
	 */
	if (data->msg != 0 && !(msg->flags & I2C_MSG_RESTART)) {
		transfer.flags |= kI2C_TransferNoStartFlag;
	}

	/* Start the transfer */
	status = I2C_MasterTransferNonBlocking(base, &data->handle, &transfer);

	/* Return an error if the transfer didn't start successfully
	 * e.g., if the bus was busy
	 */
	if (status != kStatus_Success) {
		I2C_MasterTransferAbort(base, &data->handle);
		i2c_mcux_async_done(dev, data, -EIO);
	}
}

static int i2c_mcux_transfer_cb(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				   uint16_t addr, i2c_callback_t cb, void *userdata)
{
	struct i2c_mcux_data *data = dev->data;

	int res = k_sem_take(&data->lock, K_NO_WAIT);

	if (res != 0) {
		return -EWOULDBLOCK;
	}

	data->msg = 0;
	data->msgs = msgs;
	data->num_msgs = num_msgs;
	data->addr = addr;
	data->cb = cb;
	data->userdata = userdata;
	data->addr = addr;

	i2c_mcux_async_iter(dev);

	return 0;
}

#endif /* CONFIG_I2C_CALLBACK */

static void i2c_mcux_isr(const struct device *dev)
{
	I2C_Type *base = DEV_BASE(dev);
	struct i2c_mcux_data *data = dev->data;

	I2C_MasterTransferHandleIRQ(base, &data->handle);
}

static int i2c_mcux_init(const struct device *dev)
{
	I2C_Type *base = DEV_BASE(dev);
	const struct i2c_mcux_config *config = dev->config;
	struct i2c_mcux_data *data = dev->data;
	uint32_t clock_freq, bitrate_cfg;
	i2c_master_config_t master_config;
	int error;

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);

	clock_freq = CLOCK_GetFreq(config->clock_source);
	I2C_MasterGetDefaultConfig(&master_config);
	I2C_MasterInit(base, &master_config, clock_freq);
	I2C_MasterTransferCreateHandle(base, &data->handle,
				       i2c_mcux_master_transfer_callback, (void *)dev);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);

	error = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (error) {
		return error;
	}

	error = i2c_mcux_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (error) {
		return error;
	}

	config->irq_config_func(dev);

	return 0;
}

static const struct i2c_driver_api i2c_mcux_driver_api = {
	.configure = i2c_mcux_configure,
	.transfer = i2c_mcux_transfer,
#ifdef CONFIG_I2C_CALLBACK
	.transfer_cb = i2c_mcux_transfer_cb,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#define I2C_DEVICE_INIT_MCUX(n)			\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static void i2c_mcux_config_func_ ## n(const struct device *dev); \
									\
	static const struct i2c_mcux_config i2c_mcux_config_ ## n = {	\
		.base = (I2C_Type *)DT_INST_REG_ADDR(n),\
		.clock_source = I2C ## n ## _CLK_SRC,			\
		.irq_config_func = i2c_mcux_config_func_ ## n,		\
		.bitrate = DT_INST_PROP(n, clock_frequency),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
									\
	static struct i2c_mcux_data i2c_mcux_data_ ## n;		\
									\
	I2C_DEVICE_DT_INST_DEFINE(n,					\
			i2c_mcux_init, NULL,				\
			&i2c_mcux_data_ ## n,				\
			&i2c_mcux_config_ ## n, POST_KERNEL,		\
			CONFIG_I2C_INIT_PRIORITY,			\
			&i2c_mcux_driver_api);				\
									\
	static void i2c_mcux_config_func_ ## n(const struct device *dev) \
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			DT_INST_IRQ(n, priority),			\
			i2c_mcux_isr,					\
			DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_DEVICE_INIT_MCUX)
