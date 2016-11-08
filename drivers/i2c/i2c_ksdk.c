/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <i2c.h>
#include <soc.h>
#include <fsl_i2c.h>
#include <fsl_clock.h>
#include <misc/util.h>

#define DEV_CFG(dev) \
	((const struct i2c_ksdk_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct i2c_ksdk_data * const)(dev)->driver_data)
#define DEV_BASE(dev) \
	((I2C_Type *)(DEV_CFG(dev))->base)

struct i2c_ksdk_config {
	I2C_Type *base;
	clock_name_t clock_source;
	void (*irq_config_func)(struct device *dev);
	union dev_config default_cfg;
};

struct i2c_ksdk_data {
	i2c_master_handle_t handle;
	device_sync_call_t sync;
	status_t callback_status;
};

static int i2c_ksdk_configure(struct device *dev, uint32_t dev_config_raw)
{
	I2C_Type *base = DEV_BASE(dev);
	const struct i2c_ksdk_config *config = DEV_CFG(dev);
	union dev_config dev_config = (union dev_config)dev_config_raw;
	uint32_t clock_freq;
	uint32_t baudrate;

	if (!dev_config.bits.is_master_device) {
		return -EINVAL;
	}

	if (dev_config.bits.is_slave_read) {
		return -EINVAL;
	}

	if (dev_config.bits.use_10_bit_addr) {
		return -EINVAL;
	}

	switch (dev_config.bits.speed) {
	case I2C_SPEED_STANDARD:
		baudrate = KHZ(100);
		break;
	case I2C_SPEED_FAST:
		baudrate = MHZ(1);
		break;
	default:
		return -EINVAL;
	}

	clock_freq = CLOCK_GetFreq(config->clock_source);
	I2C_MasterSetBaudRate(base, baudrate, clock_freq);

	return 0;
}

static void i2c_ksdk_master_transfer_callback(I2C_Type *base,
		i2c_master_handle_t *handle, status_t status, void *userData)
{
	struct device *dev = userData;
	struct i2c_ksdk_data *data = DEV_DATA(dev);

	data->callback_status = status;
	device_sync_call_complete(&data->sync);
}

static uint32_t i2c_ksdk_convert_flags(int msg_flags)
{
	uint32_t flags = 0;

	if (!(msg_flags & I2C_MSG_STOP)) {
		flags |= kI2C_TransferNoStopFlag;
	}

	if (msg_flags & I2C_MSG_RESTART) {
		flags |= kI2C_TransferRepeatedStartFlag;
	}

	return flags;
}

static int i2c_ksdk_transfer(struct device *dev, struct i2c_msg *msgs,
		uint8_t num_msgs, uint16_t addr)
{
	I2C_Type *base = DEV_BASE(dev);
	struct i2c_ksdk_data *data = DEV_DATA(dev);
	i2c_master_transfer_t transfer;
	status_t status;

	/* Iterate over all the messages */
	for (int i = 0; i < num_msgs; i++) {

		/* Initialize the transfer descriptor */
		transfer.flags = i2c_ksdk_convert_flags(msgs->flags);
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
			return -EIO;
		}

		/* Wait for the transfer to complete */
		device_sync_call_wait(&data->sync);

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

static void i2c_ksdk_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	I2C_Type *base = DEV_BASE(dev);
	struct i2c_ksdk_data *data = DEV_DATA(dev);

	I2C_MasterTransferHandleIRQ(base, &data->handle);
}

static int i2c_ksdk_init(struct device *dev)
{
	I2C_Type *base = DEV_BASE(dev);
	const struct i2c_ksdk_config *config = DEV_CFG(dev);
	struct i2c_ksdk_data *data = DEV_DATA(dev);
	uint32_t clock_freq;
	i2c_master_config_t master_config;
	int error;

	device_sync_call_init(&data->sync);

	clock_freq = CLOCK_GetFreq(config->clock_source);
	I2C_MasterGetDefaultConfig(&master_config);
	I2C_MasterInit(base, &master_config, clock_freq);
	I2C_MasterTransferCreateHandle(base, &data->handle,
			i2c_ksdk_master_transfer_callback, dev);

	error = i2c_ksdk_configure(dev, config->default_cfg.raw);
	if (error) {
		return error;
	}

	config->irq_config_func(dev);

	return 0;
}

static const struct i2c_driver_api i2c_ksdk_driver_api = {
	.configure = i2c_ksdk_configure,
	.transfer = i2c_ksdk_transfer,
};

#ifdef CONFIG_I2C_0
static void i2c_ksdk_config_func_0(struct device *dev);

static const struct i2c_ksdk_config i2c_ksdk_config_0 = {
	.base = I2C0,
	.clock_source = I2C0_CLK_SRC,
	.irq_config_func = i2c_ksdk_config_func_0,
	.default_cfg.raw = CONFIG_I2C_0_DEFAULT_CFG,
};

static struct i2c_ksdk_data i2c_ksdk_data_0;

DEVICE_AND_API_INIT(i2c_ksdk_0, CONFIG_I2C_0_NAME, &i2c_ksdk_init,
		    &i2c_ksdk_data_0, &i2c_ksdk_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &i2c_ksdk_driver_api);

static void i2c_ksdk_config_func_0(struct device *dev)
{
	IRQ_CONNECT(IRQ_I2C0, CONFIG_I2C_0_IRQ_PRI,
		    i2c_ksdk_isr, DEVICE_GET(i2c_ksdk_0), 0);

	irq_enable(I2C0_IRQn);
}
#endif /* CONFIG_I2C_0 */

#ifdef CONFIG_I2C_1
static void i2c_ksdk_config_func_1(struct device *dev);

static const struct i2c_ksdk_config i2c_ksdk_config_1 = {
	.base = I2C1,
	.clock_source = I2C1_CLK_SRC,
	.irq_config_func = i2c_ksdk_config_func_1,
	.default_cfg.raw = CONFIG_I2C_1_DEFAULT_CFG,
};

static struct i2c_ksdk_data i2c_ksdk_data_1;

DEVICE_AND_API_INIT(i2c_ksdk_1, CONFIG_I2C_1_NAME, &i2c_ksdk_init,
		    &i2c_ksdk_data_1, &i2c_ksdk_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &i2c_ksdk_driver_api);

static void i2c_ksdk_config_func_1(struct device *dev)
{
	IRQ_CONNECT(IRQ_I2C1, CONFIG_I2C_1_IRQ_PRI,
		    i2c_ksdk_isr, DEVICE_GET(i2c_ksdk_1), 0);

	irq_enable(I2C1_IRQn);
}
#endif /* CONFIG_I2C_1 */
