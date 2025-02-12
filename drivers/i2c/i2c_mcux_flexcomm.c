/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2019, 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_i2c

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control.h>
#include <fsl_i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(mcux_flexcomm);

#include "i2c-priv.h"

#define I2C_TRANSFER_TIMEOUT_MSEC                                                                  \
	COND_CODE_0(CONFIG_I2C_NXP_TRANSFER_TIMEOUT, (K_FOREVER),                                  \
		    (K_MSEC(CONFIG_I2C_NXP_TRANSFER_TIMEOUT)))

#define MCUX_FLEXCOMM_MAX_TARGETS 4

struct mcux_flexcomm_config {
	I2C_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	uint32_t bitrate;
	const struct pinctrl_dev_config *pincfg;
	const struct reset_dt_spec reset;
};

#ifdef CONFIG_I2C_TARGET
struct mcux_flexcomm_target_data {
	struct i2c_target_config *target_cfg;
	bool target_attached;
	bool first_read;
	bool first_write;
	bool is_write;
};
#endif

struct mcux_flexcomm_data {
	i2c_master_handle_t handle;
	struct k_sem device_sync_sem;
	struct k_sem lock;
	status_t callback_status;
#ifdef CONFIG_I2C_TARGET
	uint8_t nr_targets_attached;
	i2c_slave_config_t i2c_cfg;
	i2c_slave_handle_t target_handle;
	struct mcux_flexcomm_target_data target_data[MCUX_FLEXCOMM_MAX_TARGETS];
#endif
};

static int mcux_flexcomm_configure(const struct device *dev,
				   uint32_t dev_config_raw)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	I2C_Type *base = config->base;
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

	/* Get the clock frequency */
	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);
	I2C_MasterSetBaudRate(base, baudrate, clock_freq);
	k_sem_give(&data->lock);

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
	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);

	/* Iterate over all the messages */
	for (int i = 0; i < num_msgs; i++) {
		if (I2C_MSG_ADDR_10_BITS & msgs->flags) {
			ret = -ENOTSUP;
			break;
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
			ret = -EIO;
			break;
		}

		/* Wait for the transfer to complete */
		k_sem_take(&data->device_sync_sem, I2C_TRANSFER_TIMEOUT_MSEC);

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

#if defined(CONFIG_I2C_TARGET)

static struct mcux_flexcomm_target_data *mcux_flexcomm_find_free_target(
		struct mcux_flexcomm_data *data)
{
	struct mcux_flexcomm_target_data *target;
	int i;

	for (i = 0; i < ARRAY_SIZE(data->target_data); i++) {
		target = &data->target_data[i];
		if (!target->target_attached) {
			return target;
		}
	}
	return NULL;
}

static struct mcux_flexcomm_target_data *mcux_flexcomm_find_target_by_address(
		struct mcux_flexcomm_data *data, uint16_t address)
{
	struct mcux_flexcomm_target_data *target;
	int i;

	for (i = 0; i < ARRAY_SIZE(data->target_data); i++) {
		target = &data->target_data[i];
		if (target->target_attached && target->target_cfg->address == address) {
			return target;
		}
	}
	return NULL;
}

static int mcux_flexcomm_setup_i2c_config_address(struct mcux_flexcomm_data *data,
		struct mcux_flexcomm_target_data *target, bool disabled)
{
	i2c_slave_address_t *addr;
	int idx = -1;
	int i;

	for (i = 0; i < ARRAY_SIZE(data->target_data); i++) {
		if (data->target_data[i].target_attached && &data->target_data[i] == target) {
			idx = i;
			break;
		}
	}

	if (idx < 0) {
		return -ENODEV;
	}

	/* This could be just shifting a pointer in the i2c_cfg struct */
	/* However would be less readable and error prone if the struct changes */
	switch (idx) {
	case 0:
		addr = &data->i2c_cfg.address0;
		break;
	case 1:
		addr = &data->i2c_cfg.address1;
		break;
	case 2:
		addr = &data->i2c_cfg.address2;
		break;
	case 3:
		addr = &data->i2c_cfg.address3;
		break;
	default:
		return -1;
	}

	addr->address = target->target_cfg->address;
	addr->addressDisable = disabled;

	return 0;
}

static void i2c_target_transfer_callback(I2C_Type *base,
		volatile i2c_slave_transfer_t *transfer, void *userData)
{
	/* Convert 8-bit received address to 7-bit address */
	uint8_t address = transfer->receivedAddress >> 1;
	struct mcux_flexcomm_data *data = userData;
	struct mcux_flexcomm_target_data *target;
	const struct i2c_target_callbacks *target_cb;
	static uint8_t rxVal, txVal;

	ARG_UNUSED(base);

	target = mcux_flexcomm_find_target_by_address(data, address);
	if (!target) {
		LOG_ERR("No target found for address: 0x%x", address);
		return;
	}

	target_cb = target->target_cfg->callbacks;

	switch (transfer->event) {
	case kI2C_SlaveTransmitEvent:
		/* request to provide data to transmit */
		if (target->first_read && target_cb->read_requested) {
			target->first_read = false;
			target_cb->read_requested(target->target_cfg, &txVal);
		} else if (target_cb->read_processed) {
			target_cb->read_processed(target->target_cfg, &txVal);
		}

		transfer->txData = &txVal;
		transfer->txSize = 1;
		break;

	case kI2C_SlaveReceiveEvent:
		/* request to provide a buffer in which to place received data */
		if (target->first_write && target_cb->write_requested) {
			target_cb->write_requested(target->target_cfg);
			target->first_write = false;
		}

		transfer->rxData = &rxVal;
		transfer->rxSize = 1;
		target->is_write = true;
		break;

	case kI2C_SlaveCompletionEvent:
		/* called after every transferred byte */
		if (target->is_write && target_cb->write_received) {
			target_cb->write_received(target->target_cfg, rxVal);
			target->is_write = false;
		}
		break;

	case kI2C_SlaveDeselectedEvent:
		if (target_cb->stop) {
			target_cb->stop(target->target_cfg);
		}

		target->first_read = true;
		target->first_write = true;
		break;

	default:
		LOG_INF("Unhandled event: %d", transfer->event);
		break;
	}
}

static int mcux_flexcomm_setup_slave_config(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	I2C_Type *base = config->base;
	uint32_t clock_freq;

	/* Get the clock frequency */
	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	I2C_SlaveInit(base, &data->i2c_cfg, clock_freq);
	I2C_SlaveTransferCreateHandle(base, &data->target_handle,
			i2c_target_transfer_callback, data);
	I2C_SlaveTransferNonBlocking(base, &data->target_handle,
			kI2C_SlaveCompletionEvent | kI2C_SlaveTransmitEvent |
			kI2C_SlaveReceiveEvent | kI2C_SlaveDeselectedEvent);

	return 0;
}

int mcux_flexcomm_target_register(const struct device *dev,
			     struct i2c_target_config *target_config)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	struct mcux_flexcomm_target_data *target;
	I2C_Type *base = config->base;

	I2C_MasterDeinit(base);

	if (!target_config) {
		return -EINVAL;
	}

	target = mcux_flexcomm_find_free_target(data);
	if (!target) {
		return -EBUSY;
	}

	target->target_cfg = target_config;
	target->target_attached = true;
	target->first_read = true;
	target->first_write = true;

	if (data->nr_targets_attached == 0) {
		I2C_SlaveGetDefaultConfig(&data->i2c_cfg);
	}

	if (mcux_flexcomm_setup_i2c_config_address(data, target, false) < 0) {
		return -EINVAL;
	}

	if (mcux_flexcomm_setup_slave_config(dev) < 0) {
		return -EINVAL;
	}

	data->nr_targets_attached++;
	return 0;
}

int mcux_flexcomm_target_unregister(const struct device *dev,
			       struct i2c_target_config *target_config)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	struct mcux_flexcomm_target_data *target;
	I2C_Type *base = config->base;

	target = mcux_flexcomm_find_target_by_address(data, target_config->address);
	if (!target || !target->target_attached) {
		return -EINVAL;
	}

	if (mcux_flexcomm_setup_i2c_config_address(data, target, true) < 0) {
		return -EINVAL;
	}

	target->target_cfg = NULL;
	target->target_attached = false;

	data->nr_targets_attached--;

	if (data->nr_targets_attached > 0) {
		/* still slaves attached, reconfigure the I2C peripheral after address removal */
		if (mcux_flexcomm_setup_slave_config(dev) < 0) {
			return -EINVAL;
		}

	} else {
		I2C_SlaveDeinit(base);
	}

	return 0;
}
#endif

static void mcux_flexcomm_isr(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	I2C_Type *base = config->base;

#if defined(CONFIG_I2C_TARGET)
	if (data->nr_targets_attached > 0) {
		I2C_SlaveTransferHandleIRQ(base, &data->target_handle);
		return;
	}
#endif

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

	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("Reset device not ready");
		return -ENODEV;
	}

	error = reset_line_toggle(config->reset.dev, config->reset.id);
	if (error) {
		return error;
	}

	error = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (error) {
		return error;
	}

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Get the clock frequency */
	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	I2C_MasterGetDefaultConfig(&master_config);
	I2C_MasterInit(base, &master_config, clock_freq);
	I2C_MasterTransferCreateHandle(base, &data->handle,
				       mcux_flexcomm_master_transfer_callback,
				       data);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);

	error = mcux_flexcomm_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (error) {
		return error;
	}

	config->irq_config_func(dev);

	return 0;
}

static const struct i2c_driver_api mcux_flexcomm_driver_api = {
	.configure = mcux_flexcomm_configure,
	.transfer = mcux_flexcomm_transfer,
#if defined(CONFIG_I2C_TARGET)
	.target_register = mcux_flexcomm_target_register,
	.target_unregister = mcux_flexcomm_target_unregister,
#endif
};

#define I2C_MCUX_FLEXCOMM_DEVICE(id)					\
	PINCTRL_DT_INST_DEFINE(id);					\
	static void mcux_flexcomm_config_func_##id(const struct device *dev); \
	static const struct mcux_flexcomm_config mcux_flexcomm_config_##id = {	\
		.base = (I2C_Type *) DT_INST_REG_ADDR(id),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),	\
		.clock_subsys =				\
		(clock_control_subsys_t)DT_INST_CLOCKS_CELL(id, name),\
		.irq_config_func = mcux_flexcomm_config_func_##id,	\
		.bitrate = DT_INST_PROP(id, clock_frequency),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),		\
		.reset = RESET_DT_SPEC_INST_GET(id),			\
	};								\
	static struct mcux_flexcomm_data mcux_flexcomm_data_##id;	\
	I2C_DEVICE_DT_INST_DEFINE(id,					\
			    mcux_flexcomm_init,				\
			    NULL,					\
			    &mcux_flexcomm_data_##id,			\
			    &mcux_flexcomm_config_##id,			\
			    POST_KERNEL,				\
			    CONFIG_I2C_INIT_PRIORITY,			\
			    &mcux_flexcomm_driver_api);			\
	static void mcux_flexcomm_config_func_##id(const struct device *dev) \
	{								\
		IRQ_CONNECT(DT_INST_IRQN(id),				\
			    DT_INST_IRQ(id, priority),			\
			    mcux_flexcomm_isr,				\
			    DEVICE_DT_INST_GET(id),			\
			    0);						\
		irq_enable(DT_INST_IRQN(id));				\
	}								\

DT_INST_FOREACH_STATUS_OKAY(I2C_MCUX_FLEXCOMM_DEVICE)
