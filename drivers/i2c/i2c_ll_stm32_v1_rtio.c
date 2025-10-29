/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>
#include <stm32_bitops.h>
#include <stm32_ll_i2c.h>
#include <stm32_ll_rcc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ll_stm32_v1_rtio);

#include "i2c_ll_stm32.h"
#include "i2c-priv.h"

#define I2C_REQUEST_WRITE       0x00
#define I2C_REQUEST_READ        0x01
#define HEADER                  0xF0

static void i2c_stm32_disable_transfer_interrupts(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_DisableIT_TX(i2c);
	LL_I2C_DisableIT_RX(i2c);
	LL_I2C_DisableIT_EVT(i2c);
	LL_I2C_DisableIT_BUF(i2c);
	LL_I2C_DisableIT_ERR(i2c);
}

static void i2c_stm32_enable_transfer_interrupts(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_EnableIT_ERR(i2c);
	LL_I2C_EnableIT_EVT(i2c);
	LL_I2C_EnableIT_BUF(i2c);
}

static void i2c_stm32_generate_start_condition(I2C_TypeDef *i2c)
{
	uint16_t cr1 = stm32_reg_read(&i2c->CR1);

	if ((cr1 & I2C_CR1_STOP) != 0) {
		LOG_DBG("%s: START while STOP active!", __func__);
		stm32_reg_write(&i2c->CR1, cr1 & ~I2C_CR1_STOP);
	}

	LL_I2C_GenerateStartCondition(i2c);
}

static void i2c_stm32_master_mode_end(const struct device *dev, int status)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	struct i2c_rtio *ctx = data->ctx;
	I2C_TypeDef *i2c = cfg->i2c;

	i2c_stm32_disable_transfer_interrupts(dev);

#if defined(CONFIG_I2C_TARGET)
	data->master_active = false;
	if (data->slave_attached) {
		i2c_stm32_enable_transfer_interrupts(dev);
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
		return;
	}
#endif

	LL_I2C_Disable(i2c);
	if ((data->xfer_len == 0U) && i2c_rtio_complete(ctx, status)) {
		i2c_stm32_start(dev);
	}
}

static void handle_sb(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	uint16_t saddr = data->slave_address;
	uint8_t slave;

	if ((data->xfer_flags & I2C_MSG_ADDR_10_BITS) != 0) {
		slave = ((saddr & 0x0300) >> 7) & 0xFF;
		slave |= HEADER;

		if (data->is_restart == 0U) {
			data->is_restart = 1U;
		} else {
			slave |= I2C_REQUEST_READ;
			data->is_restart = 0U;
		}
		LL_I2C_TransmitData8(i2c, slave);
	} else {
		slave = (saddr << 1) & 0xFF;
		if ((data->xfer_flags & I2C_MSG_READ) != 0) {
			LL_I2C_TransmitData8(i2c, slave | I2C_REQUEST_READ);
			if (data->xfer_len == 2) {
				LL_I2C_EnableBitPOS(i2c);
			}
		} else {
			LL_I2C_TransmitData8(i2c, slave | I2C_REQUEST_WRITE);
		}
	}
}

static void handle_addr(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if ((data->xfer_flags & I2C_MSG_ADDR_10_BITS) != 0) {
		if (((data->xfer_flags & I2C_MSG_READ) != 0) && (data->is_restart != 0)) {
			data->is_restart = 0U;
			LL_I2C_ClearFlag_ADDR(i2c);
			i2c_stm32_generate_start_condition(i2c);
			return;
		}
	}

	if ((data->xfer_flags & I2C_MSG_READ) == 0) {
		LL_I2C_ClearFlag_ADDR(i2c);
		return;
	}
	/* According to STM32F1 errata we need to handle these corner cases in
	 * a specific way.
	 * Please ref to STM32F10xxC/D/E I2C peripheral Errata sheet 2.14.1
	 */
	if ((data->xfer_len == 0U) && IS_ENABLED(CONFIG_SOC_SERIES_STM32F1X)) {
		LL_I2C_GenerateStopCondition(i2c);
	} else if (data->xfer_len == 1U) {
		/* Single byte reception: enable NACK and clear POS */
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
#ifdef CONFIG_SOC_SERIES_STM32F1X
		LL_I2C_ClearFlag_ADDR(i2c);
		LL_I2C_GenerateStopCondition(i2c);
#endif
	} else if (data->xfer_len == 2U) {
#ifdef CONFIG_SOC_SERIES_STM32F1X
		LL_I2C_ClearFlag_ADDR(i2c);
#endif
		/* 2-byte reception: enable NACK and set POS */
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
		LL_I2C_EnableBitPOS(i2c);
	}
	LL_I2C_ClearFlag_ADDR(i2c);
}

static void handle_txe(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if (data->xfer_len != 0) {
		data->xfer_len--;
		if (data->xfer_len == 0U) {
			/*
			 * This is the last byte to transmit disable Buffer
			 * interrupt and wait for a BTF interrupt
			 */
			LL_I2C_DisableIT_BUF(i2c);
		}
		LL_I2C_TransmitData8(i2c, *data->xfer_buf);
		data->xfer_buf++;
	} else {
		if ((data->xfer_flags & I2C_MSG_STOP) != 0) {
			LL_I2C_GenerateStopCondition(i2c);
		}
		if (LL_I2C_IsActiveFlag_BTF(i2c)) {
			/* Read DR to clear BTF flag */
			LL_I2C_ReceiveData8(i2c);
		}
		i2c_stm32_master_mode_end(dev, 0);
	}
}

static void handle_rxne(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	switch (data->xfer_len) {
	case 0:
		if ((data->xfer_flags & I2C_MSG_STOP) != 0) {
			LL_I2C_GenerateStopCondition(i2c);
		}
		i2c_stm32_master_mode_end(dev, 0);
		break;
	case 1:
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
		LL_I2C_DisableBitPOS(i2c);
		/* Single byte reception */
		if ((data->xfer_flags & I2C_MSG_STOP) != 0) {
			LL_I2C_GenerateStopCondition(i2c);
		}
		LL_I2C_DisableIT_BUF(i2c);
		data->xfer_len--;
		*data->xfer_buf = LL_I2C_ReceiveData8(i2c);
		data->xfer_buf++;
		i2c_stm32_master_mode_end(dev, 0);
		break;
	case 2:
		/*
		 * 2-byte reception for N > 3 has already set the NACK
		 * bit, and must not set the POS bit. See pg. 854 in
		 * the F4 reference manual (RM0090).
		 */
		if (data->msg_len > 2) {
			break;
		}
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
		LL_I2C_EnableBitPOS(i2c);
		__fallthrough;
	case 3:
		/*
		 * 2-byte, 3-byte reception and for N-2, N-1,
		 * N when N > 3
		 */
		LL_I2C_DisableIT_BUF(i2c);
		break;
	default:
		/* N byte reception when N > 3 */
		data->xfer_len--;
		*data->xfer_buf = LL_I2C_ReceiveData8(i2c);
		data->xfer_buf++;
	}
}

static void handle_btf(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if ((data->xfer_flags & I2C_MSG_READ) == 0) {
		handle_txe(dev);
	} else {
		uint32_t counter = 0U;

		switch (data->xfer_len) {
		case 2:
			/*
			 * Stop condition must be generated before reading the
			 * last two bytes.
			 */
			if (data->xfer_flags & I2C_MSG_STOP) {
				LL_I2C_GenerateStopCondition(i2c);
			}

			for (counter = 2U; counter > 0; counter--) {
				data->xfer_len--;
				*data->xfer_buf = LL_I2C_ReceiveData8(i2c);
				data->xfer_buf++;
			}
			i2c_stm32_master_mode_end(dev, 0);
			break;
		case 3:
			/* Set NACK before reading N-2 byte*/
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
			data->xfer_len--;
			*data->xfer_buf = LL_I2C_ReceiveData8(i2c);
			data->xfer_buf++;
			break;
		default:
			handle_rxne(dev);
		}
	}
}

#if defined(CONFIG_I2C_TARGET)
static void i2c_stm32_target_event(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	const struct i2c_target_callbacks *target_cb =
		data->slave_cfg->callbacks;

	if (LL_I2C_IsActiveFlag_TXE(i2c) && LL_I2C_IsActiveFlag_BTF(i2c)) {
		uint8_t val;

		target_cb->read_processed(data->slave_cfg, &val);
		LL_I2C_TransmitData8(i2c, val);
		return;
	}

	if (LL_I2C_IsActiveFlag_RXNE(i2c)) {
		uint8_t val = LL_I2C_ReceiveData8(i2c);

		if (target_cb->write_received(data->slave_cfg, val)) {
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
		}
		return;
	}

	if (LL_I2C_IsActiveFlag_AF(i2c)) {
		LL_I2C_ClearFlag_AF(i2c);
	}

	if (LL_I2C_IsActiveFlag_STOP(i2c)) {
		LL_I2C_ClearFlag_STOP(i2c);
		target_cb->stop(data->slave_cfg);
		/* Prepare to ACK next transmissions address byte */
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
	}

	if (LL_I2C_IsActiveFlag_ADDR(i2c)) {
		uint32_t dir = LL_I2C_GetTransferDirection(i2c);

		if (dir == LL_I2C_DIRECTION_READ) {
			target_cb->write_requested(data->slave_cfg);
			LL_I2C_EnableIT_RX(i2c);
		} else {
			uint8_t val;

			target_cb->read_requested(data->slave_cfg, &val);
			LL_I2C_TransmitData8(i2c, val);
			LL_I2C_EnableIT_TX(i2c);
		}

		i2c_stm32_enable_transfer_interrupts(dev);
	}
}

/* Attach and start I2C as target */
int i2c_stm32_target_register(const struct device *dev, struct i2c_target_config *config)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t bitrate_cfg;
	int ret;

	if (config == NULL) {
		return -EINVAL;
	}

	if (data->slave_attached) {
		return -EBUSY;
	}

	if (data->master_active) {
		return -EBUSY;
	}

	bitrate_cfg = i2c_map_dt_bitrate(cfg->bitrate);

	ret = i2c_stm32_runtime_configure(dev, bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("i2c: failure initializing");
		return ret;
	}

	data->slave_cfg = config;

	LL_I2C_Enable(i2c);

	if (data->slave_cfg->flags == I2C_TARGET_FLAGS_ADDR_10_BITS)	{
		return -ENOTSUP;
	}
	LL_I2C_SetOwnAddress1(i2c, config->address << 1U, LL_I2C_OWNADDRESS1_7BIT);
	data->slave_attached = true;

	LOG_DBG("i2c: target registered");

	i2c_stm32_enable_transfer_interrupts(dev);
	LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);

	return 0;
}

int i2c_stm32_target_unregister(const struct device *dev, struct i2c_target_config *config)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if (!data->slave_attached) {
		return -EINVAL;
	}

	if (data->master_active) {
		return -EBUSY;
	}

	i2c_stm32_disable_transfer_interrupts(dev);

	LL_I2C_ClearFlag_AF(i2c);
	LL_I2C_ClearFlag_STOP(i2c);
	LL_I2C_ClearFlag_ADDR(i2c);
	LL_I2C_Disable(i2c);

	data->slave_attached = false;

	LOG_DBG("i2c: target unregistered");

	return 0;
}
#endif /* defined(CONFIG_I2C_TARGET) */

void i2c_stm32_event(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

#if defined(CONFIG_I2C_TARGET)
	if (data->slave_attached && !data->master_active) {
		i2c_stm32_target_event(dev);
		return;
	}
#endif

	if (LL_I2C_IsActiveFlag_SB(i2c)) {
		handle_sb(dev);
	} else if (LL_I2C_IsActiveFlag_ADD10(i2c)) {
		LL_I2C_TransmitData8(i2c, data->slave_address);
	} else if (LL_I2C_IsActiveFlag_ADDR(i2c)) {
		handle_addr(dev);
	} else if (LL_I2C_IsActiveFlag_BTF(i2c)) {
		handle_btf(dev);
	} else if (LL_I2C_IsActiveFlag_TXE(i2c) && ((data->xfer_flags & I2C_MSG_READ) == 0)) {
		handle_txe(dev);
	} else if (LL_I2C_IsActiveFlag_RXNE(i2c) && ((data->xfer_flags & I2C_MSG_READ) != 0)) {
		handle_rxne(dev);
	}
}

int i2c_stm32_error(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

#if defined(CONFIG_I2C_TARGET)
	struct i2c_stm32_data *data = dev->data;
	i2c_target_error_cb_t error_cb = NULL;

	if (data->slave_attached && !data->master_active &&
	    data->slave_cfg != NULL && data->slave_cfg->callbacks != NULL) {
		error_cb = data->slave_cfg->callbacks->error;
	}
#endif

	if (LL_I2C_IsActiveFlag_AF(i2c)) {
		LL_I2C_ClearFlag_AF(i2c);
		LL_I2C_GenerateStopCondition(i2c);
#if defined(CONFIG_I2C_TARGET)
		if (error_cb != NULL) {
			error_cb(data->slave_cfg, I2C_ERROR_GENERIC);
		}
#endif
		goto error;
	}
	if (LL_I2C_IsActiveFlag_ARLO(i2c)) {
		LL_I2C_ClearFlag_ARLO(i2c);
#if defined(CONFIG_I2C_TARGET)
		if (error_cb != NULL) {
			error_cb(data->slave_cfg, I2C_ERROR_ARBITRATION);
		}
#endif
		goto error;
	}

	if (LL_I2C_IsActiveFlag_BERR(i2c)) {
		LL_I2C_ClearFlag_BERR(i2c);
#if defined(CONFIG_I2C_TARGET)
		if (error_cb != NULL) {
			error_cb(data->slave_cfg, I2C_ERROR_GENERIC);
		}
#endif
		goto error;
	}

	return 0;
error:
#if defined(CONFIG_I2C_TARGET)
	if (!data->slave_attached || data->master_active) {
		i2c_stm32_master_mode_end(dev, -EIO);
	}
#else
	i2c_stm32_master_mode_end(dev, -EIO);
#endif
	return -EIO;

}

int i2c_stm32_msg_start(const struct device *dev, uint8_t flags,
			uint8_t *buf, size_t buf_len, uint16_t i2c_addr)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	data->xfer_buf = buf;
	data->xfer_len = buf_len;
	data->xfer_flags = flags;
	data->msg_len = buf_len;
	data->is_restart = 0;
	data->slave_address = i2c_addr;
#if defined(CONFIG_I2C_TARGET)
	data->master_active = true;
#endif

	LL_I2C_Enable(i2c);

	LL_I2C_DisableBitPOS(i2c);
	LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
	if ((flags & I2C_MSG_RESTART) != 0U) {
		i2c_stm32_generate_start_condition(i2c);
	}

	i2c_stm32_enable_transfer_interrupts(dev);
	if (flags & I2C_MSG_READ) {
		LL_I2C_EnableIT_RX(i2c);
	} else {
		LL_I2C_EnableIT_TX(i2c);
	}

	return 0;
}

int i2c_stm32_configure_timing(const struct device *dev, uint32_t clock)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	switch (I2C_SPEED_GET(data->dev_config)) {
	case I2C_SPEED_STANDARD:
		LL_I2C_ConfigSpeed(i2c, clock, 100000, LL_I2C_DUTYCYCLE_2);
		break;
	case I2C_SPEED_FAST:
		LL_I2C_ConfigSpeed(i2c, clock, 400000, LL_I2C_DUTYCYCLE_2);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
