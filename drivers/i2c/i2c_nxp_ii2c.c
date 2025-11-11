/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_ii2c

#include <errno.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <fsl_i2c.h>
#include <fsl_clock.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_nxp_ii2c);

#include "i2c-priv.h"

#define DEV_CFG(_dev) ((const struct nxp_ii2c_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct nxp_ii2c_data *)(_dev)->data)

struct nxp_ii2c_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	uint32_t bitrate;
	const struct pinctrl_dev_config *pincfg;
};

struct nxp_ii2c_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
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

static I2C_Type *get_base(const struct device *dev)
{
	return (I2C_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
}

static int nxp_ii2c_configure(const struct device *dev,
			      uint32_t dev_config_raw)
{
	I2C_Type *base = get_base(dev);
	struct nxp_ii2c_data *data = dev->data;
	const struct nxp_ii2c_config *config = dev->config;
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

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);
	I2C_MasterSetBaudRate(base, baudrate, clock_freq);
	k_sem_give(&data->lock);

	return 0;
}

#ifdef CONFIG_I2C_CALLBACK

static void nxp_ii2c_async_done(const struct device *dev, struct nxp_ii2c_data *data, int result);
static void nxp_ii2c_async_iter(const struct device *dev);

#endif

static void nxp_ii2c_master_transfer_callback(I2C_Type *base,
					      i2c_master_handle_t *handle,
					      status_t status, void *userdata)
{

	ARG_UNUSED(handle);
	ARG_UNUSED(base);

	struct device *dev = userdata;
	struct nxp_ii2c_data *data = dev->data;

#ifdef CONFIG_I2C_CALLBACK
	if (data->cb != NULL) {
		/* Async transfer */
		if (status != kStatus_Success) {
			I2C_MasterTransferAbort(base, &data->handle);
			nxp_ii2c_async_done(dev, data, -EIO);
		} else if (data->msg == data->num_msgs - 1) {
			nxp_ii2c_async_done(dev, data, 0);
		} else {
			data->msg++;
			nxp_ii2c_async_iter(dev);
		}
		return;
	}
#endif /* CONFIG_I2C_CALLBACK */

	data->callback_status = status;

	k_sem_give(&data->device_sync_sem);
}

static uint32_t nxp_ii2c_convert_flags(int msg_flags)
{
	uint32_t flags = kI2C_TransferDefaultFlag;

	if (!(msg_flags & I2C_MSG_STOP)) {
		flags |= kI2C_TransferNoStopFlag;
	}

	if (msg_flags & I2C_MSG_RESTART) {
		flags |= kI2C_TransferRepeatedStartFlag;
	}

	return flags;
}

static int nxp_ii2c_transfer(const struct device *dev, struct i2c_msg *msgs,
			     uint8_t num_msgs, uint16_t addr)
{
	I2C_Type *base = get_base(dev);
	struct nxp_ii2c_data *data = dev->data;
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
		transfer.flags = nxp_ii2c_convert_flags(msgs->flags);
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

static void nxp_ii2c_async_done(const struct device *dev, struct nxp_ii2c_data *data, int result)
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
static void nxp_ii2c_async_iter(const struct device *dev)
{
	I2C_Type *base = get_base(dev);
	struct nxp_ii2c_data *data = dev->data;
	i2c_master_transfer_t transfer;
	status_t status;
	struct i2c_msg *msg = &data->msgs[data->msg];

	if (I2C_MSG_ADDR_10_BITS & msg->flags) {
		nxp_ii2c_async_done(dev, data, -ENOTSUP);
		return;
	}

	/* Initialize the transfer descriptor */
	transfer.flags = nxp_ii2c_convert_flags(msg->flags);
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
		nxp_ii2c_async_done(dev, data, -EIO);
	}
}

static int nxp_ii2c_transfer_cb(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				   uint16_t addr, i2c_callback_t cb, void *userdata)
{
	struct nxp_ii2c_data *data = dev->data;

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

	nxp_ii2c_async_iter(dev);

	return 0;
}

#endif /* CONFIG_I2C_CALLBACK */

static void nxp_ii2c_isr(const struct device *dev)
{
	I2C_Type *base = get_base(dev);
	struct nxp_ii2c_data *data = dev->data;

	I2C_MasterTransferHandleIRQ(base, &data->handle);
}

static int nxp_ii2c_init(const struct device *dev)
{
	I2C_Type *base;
	const struct nxp_ii2c_config *config = dev->config;
	struct nxp_ii2c_data *data = dev->data;
	uint32_t clock_freq, bitrate_cfg;
	i2c_master_config_t master_config;
	int error;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	error = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (error) {
		return error;
	}

	base = get_base(dev);

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);

	I2C_MasterGetDefaultConfig(&master_config);
	I2C_MasterInit(base, &master_config, clock_freq);
	I2C_MasterTransferCreateHandle(base, &data->handle,
				       nxp_ii2c_master_transfer_callback, (void *)dev);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);

	error = nxp_ii2c_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (error) {
		return error;
	}

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(i2c, nxp_ii2c_driver_api) = {
	.configure = nxp_ii2c_configure,
	.transfer = nxp_ii2c_transfer,
#ifdef CONFIG_I2C_CALLBACK
	.transfer_cb = nxp_ii2c_transfer_cb,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#define I2C_DEVICE_INIT_MCUX(n)			\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static void nxp_ii2c_config_func_ ## n(const struct device *dev); \
									\
	static const struct nxp_ii2c_config nxp_ii2c_config_ ## n = {	\
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),	\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys =						\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
		.irq_config_func = nxp_ii2c_config_func_ ## n,		\
		.bitrate = DT_INST_PROP(n, clock_frequency),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
									\
	static struct nxp_ii2c_data nxp_ii2c_data_ ## n;		\
									\
	I2C_DEVICE_DT_INST_DEFINE(n,					\
			nxp_ii2c_init, NULL,				\
			&nxp_ii2c_data_ ## n,				\
			&nxp_ii2c_config_ ## n, POST_KERNEL,		\
			CONFIG_I2C_INIT_PRIORITY,			\
			&nxp_ii2c_driver_api);				\
									\
	static void nxp_ii2c_config_func_ ## n(const struct device *dev) \
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			DT_INST_IRQ(n, priority),			\
			nxp_ii2c_isr,					\
			DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_DEVICE_INIT_MCUX)
