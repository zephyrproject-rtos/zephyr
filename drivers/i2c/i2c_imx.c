/*
 * Copyright (c) 2018 Diego Sueiro, <diego.sueiro@gmail.com>
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fsl_imx21_i2c

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <soc.h>
#include <i2c_imx.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_imx);

#include "i2c-priv.h"

#define DEV_BASE(dev) \
	((I2C_Type *)((const struct i2c_imx_config * const)(dev)->config)->base)

struct i2c_imx_config {
	I2C_Type *base;
	void (*irq_config_func)(const struct device *dev);
	uint32_t bitrate;
	const struct pinctrl_dev_config *pincfg;
};

struct i2c_master_transfer {
	const uint8_t     *txBuff;
	volatile uint8_t  *rxBuff;
	volatile uint32_t	cmdSize;
	volatile uint32_t	txSize;
	volatile uint32_t	rxSize;
	volatile bool	isBusy;
	volatile uint32_t	currentDir;
	volatile uint32_t	currentMode;
	volatile bool	ack;
};

struct i2c_imx_data {
	struct i2c_master_transfer transfer;
	struct k_sem device_sync_sem;
};

static bool i2c_imx_write(const struct device *dev, uint8_t *txBuffer,
			  uint32_t txSize)
{
	I2C_Type *base = DEV_BASE(dev);
	struct i2c_imx_data *data = dev->data;
	struct i2c_master_transfer *transfer = &data->transfer;

	transfer->isBusy = true;

	/* Clear I2C interrupt flag to avoid spurious interrupt */
	I2C_ClearStatusFlag(base, i2cStatusInterrupt);

	/* Set I2C work under Tx mode */
	I2C_SetDirMode(base, i2cDirectionTransmit);
	transfer->currentDir = i2cDirectionTransmit;

	transfer->txBuff = txBuffer;
	transfer->txSize = txSize;

	I2C_WriteByte(base, *transfer->txBuff);
	transfer->txBuff++;
	transfer->txSize--;

	/* Enable I2C interrupt, subsequent data transfer will be handled
	 * in ISR.
	 */
	I2C_SetIntCmd(base, true);

	/* Wait for the transfer to complete */
	k_sem_take(&data->device_sync_sem, K_FOREVER);

	return transfer->ack;
}

static void i2c_imx_read(const struct device *dev, uint8_t *rxBuffer,
			 uint32_t rxSize)
{
	I2C_Type *base = DEV_BASE(dev);
	struct i2c_imx_data *data = dev->data;
	struct i2c_master_transfer *transfer = &data->transfer;

	transfer->isBusy = true;

	/* Clear I2C interrupt flag to avoid spurious interrupt */
	I2C_ClearStatusFlag(base, i2cStatusInterrupt);

	/* Change to receive state. */
	I2C_SetDirMode(base, i2cDirectionReceive);
	transfer->currentDir = i2cDirectionReceive;

	transfer->rxBuff = rxBuffer;
	transfer->rxSize = rxSize;

	if (transfer->rxSize == 1U) {
		/* Send Nack */
		I2C_SetAckBit(base, false);
	} else {
		/* Send Ack */
		I2C_SetAckBit(base, true);
	}

	/* dummy read to clock in 1st byte */
	I2C_ReadByte(base);

	/* Enable I2C interrupt, subsequent data transfer will be handled
	 * in ISR.
	 */
	I2C_SetIntCmd(base, true);

	/* Wait for the transfer to complete */
	k_sem_take(&data->device_sync_sem, K_FOREVER);

}

static int i2c_imx_configure(const struct device *dev,
			     uint32_t dev_config_raw)
{
	I2C_Type *base = DEV_BASE(dev);
	struct i2c_imx_data *data = dev->data;
	struct i2c_master_transfer *transfer = &data->transfer;
	uint32_t baudrate;

	if (!(I2C_MODE_CONTROLLER & dev_config_raw)) {
		return -EINVAL;
	}

	if (I2C_ADDR_10_BITS & dev_config_raw) {
		return -EINVAL;
	}

	/* Initialize I2C state structure content. */
	transfer->txBuff = 0;
	transfer->rxBuff = 0;
	transfer->cmdSize = 0U;
	transfer->txSize = 0U;
	transfer->rxSize = 0U;
	transfer->isBusy = false;
	transfer->currentDir = i2cDirectionReceive;
	transfer->currentMode = i2cModeSlave;

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

	/* Setup I2C init structure. */
	i2c_init_config_t i2cInitConfig = {
		.baudRate	  = baudrate,
		.slaveAddress = 0x00
	};

	i2cInitConfig.clockRate = get_i2c_clock_freq(base);

	I2C_Init(base, &i2cInitConfig);

	I2C_Enable(base);

	return 0;
}

static int i2c_imx_send_addr(const struct device *dev, uint16_t addr,
			     uint8_t flags)
{
	uint8_t byte0 = addr << 1;

	byte0 |= (flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
	return i2c_imx_write(dev, &byte0, 1);
}

static int i2c_imx_transfer(const struct device *dev, struct i2c_msg *msgs,
			    uint8_t num_msgs, uint16_t addr)
{
	I2C_Type *base = DEV_BASE(dev);
	struct i2c_imx_data *data = dev->data;
	struct i2c_master_transfer *transfer = &data->transfer;
	uint16_t timeout = UINT16_MAX;
	int result = -EIO;

	if (!num_msgs) {
		return 0;
	}

	/* Wait until bus not busy */
	while ((I2C_I2SR_REG(base) & i2cStatusBusBusy) && (--timeout)) {
	}

	if (timeout == 0U) {
		return result;
	}

	/* Make sure we're in a good state so slave recognises the Start */
	I2C_SetWorkMode(base, i2cModeSlave);
	transfer->currentMode = i2cModeSlave;
	/* Switch back to Rx direction. */
	I2C_SetDirMode(base, i2cDirectionReceive);
	transfer->currentDir = i2cDirectionReceive;
	/* Start condition */
	I2C_SetDirMode(base, i2cDirectionTransmit);
	transfer->currentDir = i2cDirectionTransmit;
	I2C_SetWorkMode(base, i2cModeMaster);
	transfer->currentMode = i2cModeMaster;

	/* Send address after any Start condition */
	if (!i2c_imx_send_addr(dev, addr, msgs->flags)) {
		goto finish; /* No ACK received */
	}

	do {
		if (msgs->flags & I2C_MSG_RESTART) {
			I2C_SendRepeatStart(base);
			if (!i2c_imx_send_addr(dev, addr, msgs->flags)) {
				goto finish; /* No ACK received */
			}
		}

		/* Transfer data */
		if (msgs->len) {
			if ((msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
				i2c_imx_read(dev, msgs->buf, msgs->len);
			} else {
				if (!i2c_imx_write(dev, msgs->buf, msgs->len)) {
					goto finish; /* No ACK received */
				}
			}
		}

		if (msgs->flags & I2C_MSG_STOP) {
			I2C_SetWorkMode(base, i2cModeSlave);
			transfer->currentMode = i2cModeSlave;
			I2C_SetDirMode(base, i2cDirectionReceive);
			transfer->currentDir = i2cDirectionReceive;
		}

		/* Next message */
		msgs++;
		num_msgs--;
	} while (num_msgs);

	/* Complete without error */
	result = 0;
	return result;

finish:
	I2C_SetWorkMode(base, i2cModeSlave);
	transfer->currentMode = i2cModeSlave;
	I2C_SetDirMode(base, i2cDirectionReceive);
	transfer->currentDir = i2cDirectionReceive;

	return result;
}


static void i2c_imx_isr(const struct device *dev)
{
	I2C_Type *base = DEV_BASE(dev);
	struct i2c_imx_data *data = dev->data;
	struct i2c_master_transfer *transfer = &data->transfer;

	/* Clear interrupt flag. */
	I2C_ClearStatusFlag(base, i2cStatusInterrupt);


	/* Exit the ISR if no transfer is happening for this instance. */
	if (!transfer->isBusy) {
		return;
	}

	if (i2cModeMaster == transfer->currentMode) {
		if (i2cDirectionTransmit == transfer->currentDir) {
			/* Normal write operation. */
			transfer->ack =
			!(I2C_GetStatusFlag(base, i2cStatusReceivedAck));

			if (transfer->txSize == 0U) {
				/* Close I2C interrupt. */
				I2C_SetIntCmd(base, false);
				/* Release I2C Bus. */
				transfer->isBusy = false;
				k_sem_give(&data->device_sync_sem);
			} else {
				I2C_WriteByte(base, *transfer->txBuff);
				transfer->txBuff++;
				transfer->txSize--;
			}
		} else {
			/* Normal read operation. */
			if (transfer->rxSize == 2U) {
				/* Send Nack */
				I2C_SetAckBit(base, false);
			} else {
				/* Send Ack */
				I2C_SetAckBit(base, true);
			}

			if (transfer->rxSize == 1U) {
				/* Switch back to Tx direction to avoid
				 * additional I2C bus read.
				 */
				I2C_SetDirMode(base, i2cDirectionTransmit);
				transfer->currentDir = i2cDirectionTransmit;
			}

			*transfer->rxBuff = I2C_ReadByte(base);
			transfer->rxBuff++;
			transfer->rxSize--;

			/* receive finished. */
			if (transfer->rxSize == 0U) {
				/* Close I2C interrupt. */
				I2C_SetIntCmd(base, false);
				/* Release I2C Bus. */
				transfer->isBusy = false;
				k_sem_give(&data->device_sync_sem);
			}
		}
	}
}

static int i2c_imx_init(const struct device *dev)
{
	const struct i2c_imx_config *config = dev->config;
	struct i2c_imx_data *data = dev->data;
	uint32_t bitrate_cfg;
	int error;

	k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);

	error = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (error) {
		return error;
	}

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);

	error = i2c_imx_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (error) {
		return error;
	}

	config->irq_config_func(dev);

	return 0;
}

static const struct i2c_driver_api i2c_imx_driver_api = {
	.configure = i2c_imx_configure,
	.transfer = i2c_imx_transfer,
};

#define I2C_IMX_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);					\
	static void i2c_imx_config_func_##n(const struct device *dev);	\
									\
	static const struct i2c_imx_config i2c_imx_config_##n = {	\
		.base = (I2C_Type *)DT_INST_REG_ADDR(n),		\
		.irq_config_func = i2c_imx_config_func_##n,		\
		.bitrate = DT_INST_PROP(n, clock_frequency),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
									\
	static struct i2c_imx_data i2c_imx_data_##n;			\
									\
	I2C_DEVICE_DT_INST_DEFINE(n,					\
				i2c_imx_init,				\
				NULL,					\
				&i2c_imx_data_##n, &i2c_imx_config_##n,	\
				POST_KERNEL,				\
				CONFIG_I2C_INIT_PRIORITY,		\
				&i2c_imx_driver_api);			\
									\
	static void i2c_imx_config_func_##n(const struct device *dev)	\
	{								\
		ARG_UNUSED(dev);					\
									\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    i2c_imx_isr, DEVICE_DT_INST_GET(n), 0);	\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_IMX_INIT)
