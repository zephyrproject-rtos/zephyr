/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
	u32_t bitrate;
};

struct mcux_lpi2c_data {
	lpi2c_master_handle_t handle;
	struct k_sem device_sync_sem;
	status_t callback_status;
};

static int mcux_lpi2c_configure(struct device *dev, u32_t dev_config_raw)
{
	const struct mcux_lpi2c_config *config = dev->config->config_info;
	LPI2C_Type *base = config->base;
	struct device *clock_dev;
	u32_t clock_freq;
	u32_t baudrate;

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
	struct mcux_lpi2c_data *data = dev->driver_data;

	ARG_UNUSED(handle);
	ARG_UNUSED(base);

	data->callback_status = status;
	k_sem_give(&data->device_sync_sem);
}

static u32_t mcux_lpi2c_convert_flags(int msg_flags)
{
	u32_t flags = 0U;

	if (!(msg_flags & I2C_MSG_STOP)) {
		flags |= kLPI2C_TransferNoStopFlag;
	}

	if (msg_flags & I2C_MSG_RESTART) {
		flags |= kLPI2C_TransferRepeatedStartFlag;
	}

	return flags;
}

static int mcux_lpi2c_transfer(struct device *dev, struct i2c_msg *msgs,
		u8_t num_msgs, u16_t addr)
{
	const struct mcux_lpi2c_config *config = dev->config->config_info;
	struct mcux_lpi2c_data *data = dev->driver_data;
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
			return -EIO;
		}

		/* Wait for the transfer to complete */
		k_sem_take(&data->device_sync_sem, K_FOREVER);

		/* Return an error if the transfer didn't complete
		 * successfully. e.g., nak, timeout, lost arbitration
		 */
		if (data->callback_status != kStatus_Success) {
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
	const struct mcux_lpi2c_config *config = dev->config->config_info;
	struct mcux_lpi2c_data *data = dev->driver_data;
	LPI2C_Type *base = config->base;

	LPI2C_MasterTransferHandleIRQ(base, &data->handle);
}

static int mcux_lpi2c_init(struct device *dev)
{
	const struct mcux_lpi2c_config *config = dev->config->config_info;
	struct mcux_lpi2c_data *data = dev->driver_data;
	LPI2C_Type *base = config->base;
	struct device *clock_dev;
	u32_t clock_freq, bitrate_cfg;
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

#ifdef CONFIG_I2C_0
static void mcux_lpi2c_config_func_0(struct device *dev);

static const struct mcux_lpi2c_config mcux_lpi2c_config_0 = {
	.base = (LPI2C_Type *)DT_I2C_MCUX_LPI2C_0_BASE_ADDRESS,
	.clock_name = DT_I2C_MCUX_LPI2C_0_CLOCK_NAME,
	.clock_subsys =
		(clock_control_subsys_t) DT_I2C_MCUX_LPI2C_0_CLOCK_SUBSYS,
	.irq_config_func = mcux_lpi2c_config_func_0,
	.bitrate = DT_I2C_MCUX_LPI2C_0_BITRATE,
};

static struct mcux_lpi2c_data mcux_lpi2c_data_0;

DEVICE_AND_API_INIT(mcux_lpi2c_0, DT_I2C_0_NAME, &mcux_lpi2c_init,
		    &mcux_lpi2c_data_0, &mcux_lpi2c_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_lpi2c_driver_api);

static void mcux_lpi2c_config_func_0(struct device *dev)
{
	IRQ_CONNECT(DT_I2C_MCUX_LPI2C_0_IRQ, DT_I2C_MCUX_LPI2C_0_IRQ_PRI,
		    mcux_lpi2c_isr, DEVICE_GET(mcux_lpi2c_0), 0);

	irq_enable(DT_I2C_MCUX_LPI2C_0_IRQ);
}
#endif /* CONFIG_I2C_0 */

#ifdef CONFIG_I2C_1
static void mcux_lpi2c_config_func_1(struct device *dev);

static const struct mcux_lpi2c_config mcux_lpi2c_config_1 = {
	.base = (LPI2C_Type *)DT_I2C_MCUX_LPI2C_1_BASE_ADDRESS,
	.clock_name = DT_I2C_MCUX_LPI2C_1_CLOCK_NAME,
	.clock_subsys =
		(clock_control_subsys_t) DT_I2C_MCUX_LPI2C_1_CLOCK_SUBSYS,
	.irq_config_func = mcux_lpi2c_config_func_1,
	.bitrate = DT_I2C_MCUX_LPI2C_1_BITRATE,
};

static struct mcux_lpi2c_data mcux_lpi2c_data_1;

DEVICE_AND_API_INIT(mcux_lpi2c_1, DT_I2C_1_NAME, &mcux_lpi2c_init,
		    &mcux_lpi2c_data_1, &mcux_lpi2c_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_lpi2c_driver_api);

static void mcux_lpi2c_config_func_1(struct device *dev)
{
	IRQ_CONNECT(DT_I2C_MCUX_LPI2C_1_IRQ, DT_I2C_MCUX_LPI2C_1_IRQ_PRI,
		    mcux_lpi2c_isr, DEVICE_GET(mcux_lpi2c_1), 0);

	irq_enable(DT_I2C_MCUX_LPI2C_1_IRQ);
}
#endif /* CONFIG_I2C_1 */

#ifdef CONFIG_I2C_2
static void mcux_lpi2c_config_func_2(struct device *dev);

static const struct mcux_lpi2c_config mcux_lpi2c_config_2 = {
	.base = (LPI2C_Type *)DT_I2C_MCUX_LPI2C_2_BASE_ADDRESS,
	.clock_name = DT_I2C_MCUX_LPI2C_2_CLOCK_NAME,
	.clock_subsys =
		(clock_control_subsys_t) DT_I2C_MCUX_LPI2C_2_CLOCK_SUBSYS,
	.irq_config_func = mcux_lpi2c_config_func_2,
	.bitrate = DT_I2C_MCUX_LPI2C_2_BITRATE,
};

static struct mcux_lpi2c_data mcux_lpi2c_data_2;

DEVICE_AND_API_INIT(mcux_lpi2c_2, DT_I2C_2_NAME, &mcux_lpi2c_init,
		    &mcux_lpi2c_data_2, &mcux_lpi2c_config_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_lpi2c_driver_api);

static void mcux_lpi2c_config_func_2(struct device *dev)
{
	IRQ_CONNECT(DT_I2C_MCUX_LPI2C_2_IRQ, DT_I2C_MCUX_LPI2C_2_IRQ_PRI,
		    mcux_lpi2c_isr, DEVICE_GET(mcux_lpi2c_2), 0);

	irq_enable(DT_I2C_MCUX_LPI2C_2_IRQ);
}
#endif /* CONFIG_I2C_2 */

#ifdef CONFIG_I2C_3
static void mcux_lpi2c_config_func_3(struct device *dev);

static const struct mcux_lpi2c_config mcux_lpi2c_config_3 = {
	.base = (LPI2C_Type *)DT_I2C_MCUX_LPI2C_3_BASE_ADDRESS,
	.clock_name = DT_I2C_MCUX_LPI2C_3_CLOCK_NAME,
	.clock_subsys =
		(clock_control_subsys_t) DT_I2C_MCUX_LPI2C_3_CLOCK_SUBSYS,
	.irq_config_func = mcux_lpi2c_config_func_3,
	.bitrate = DT_I2C_MCUX_LPI2C_3_BITRATE,
};

static struct mcux_lpi2c_data mcux_lpi2c_data_3;

DEVICE_AND_API_INIT(mcux_lpi2c_3, DT_I2C_3_NAME, &mcux_lpi2c_init,
		    &mcux_lpi2c_data_3, &mcux_lpi2c_config_3,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_lpi2c_driver_api);

static void mcux_lpi2c_config_func_3(struct device *dev)
{
	IRQ_CONNECT(DT_I2C_MCUX_LPI2C_3_IRQ, DT_I2C_MCUX_LPI2C_3_IRQ_PRI,
		    mcux_lpi2c_isr, DEVICE_GET(mcux_lpi2c_3), 0);

	irq_enable(DT_I2C_MCUX_LPI2C_3_IRQ);
}
#endif /* CONFIG_I2C_3 */

#ifdef CONFIG_I2C_4
static void mcux_lpi2c_config_func_4(struct device *dev);

static const struct mcux_lpi2c_config mcux_lpi2c_config_4 = {
	.base = (LPI2C_Type *)DT_I2C_MCUX_LPI2C_4_BASE_ADDRESS,
	.clock_name = DT_I2C_MCUX_LPI2C_4_CLOCK_NAME,
	.clock_subsys =
		(clock_control_subsys_t) DT_I2C_MCUX_LPI2C_4_CLOCK_SUBSYS,
	.irq_config_func = mcux_lpi2c_config_func_4,
	.bitrate = DT_I2C_MCUX_LPI2C_4_BITRATE,
};

static struct mcux_lpi2c_data mcux_lpi2c_data_4;

DEVICE_AND_API_INIT(mcux_lpi2c_4, DT_I2C_4_NAME, &mcux_lpi2c_init,
		    &mcux_lpi2c_data_4, &mcux_lpi2c_config_4,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_lpi2c_driver_api);

static void mcux_lpi2c_config_func_4(struct device *dev)
{
	IRQ_CONNECT(DT_I2C_MCUX_LPI2C_4_IRQ, DT_I2C_MCUX_LPI2C_4_IRQ_PRI,
		    mcux_lpi2c_isr, DEVICE_GET(mcux_lpi2c_4), 0);

	irq_enable(DT_I2C_MCUX_LPI2C_4_IRQ);
}
#endif /* CONFIG_I2C_4 */
