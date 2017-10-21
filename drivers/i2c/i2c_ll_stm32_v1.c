/*
 * Copyright (c) 2017, I-SENSE group of ICCS
 * Copyright (c) 2017 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C Driver for: STM32F1, STM32F2, STM32F4 and STM32L1
 *
 */

#include <clock_control/stm32_clock_control.h>
#include <clock_control.h>
#include <misc/util.h>
#include <kernel.h>
#include <board.h>
#include <errno.h>
#include <i2c.h>
#include "i2c_ll_stm32.h"

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_I2C_LEVEL
#include <logging/sys_log.h>

#define I2C_REQUEST_WRITE	0x00
#define I2C_REQUEST_READ	0x01
#define HEADER			0xF0

#ifdef CONFIG_I2C_STM32_INTERRUPT
static inline void handle_sb(I2C_TypeDef *i2c, struct i2c_stm32_data *data)
{
	u16_t saddr = data->slave_address;
	u8_t slave;

	if (I2C_ADDR_10_BITS & data->dev_config) {
		slave = (((saddr & 0x0300) >> 7) & 0xFF);
		u8_t header = slave | HEADER;

		if (data->current.is_restart == 0) {
			data->current.is_restart = 1;
		} else {
			header |= I2C_REQUEST_READ;
			data->current.is_restart = 0;
		}
		LL_I2C_TransmitData8(i2c, header);

		return;
	}
	slave = (saddr << 1) & 0xFF;
	if (data->current.is_write) {
		LL_I2C_TransmitData8(i2c, slave | I2C_REQUEST_WRITE);
	} else {
		LL_I2C_TransmitData8(i2c, slave | I2C_REQUEST_READ);
	}
}

static inline void handle_addr(I2C_TypeDef *i2c, struct i2c_stm32_data *data)
{
	if (I2C_ADDR_10_BITS & data->dev_config) {
		if (!data->current.is_write && data->current.is_restart) {
			data->current.is_restart = 0;
			LL_I2C_ClearFlag_ADDR(i2c);
			LL_I2C_GenerateStartCondition(i2c);

			return;
		}
	}
	if (!data->current.is_write) {
		if (data->current.len == 1) {
			/* Single byte reception: enable NACK and clear POS */
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
		} else if (data->current.len == 2) {
			/* 2-byte reception: enable NACK and set POS */
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
			LL_I2C_EnableBitPOS(i2c);
		}
	}
	LL_I2C_ClearFlag_ADDR(i2c);
}

static inline void handle_txe(I2C_TypeDef *i2c, struct i2c_stm32_data *data)
{
	if (data->current.len) {
		data->current.len--;
		if (data->current.len == 0) {
			/*
			 * This is the last byte to transmit disable Buffer
			 * interrupt and wait for a BTF interrupt
			 */
			LL_I2C_DisableIT_BUF(i2c);
		}
		LL_I2C_TransmitData8(i2c, *data->current.buf);
		data->current.buf++;
	} else {
		if (data->current.flags & I2C_MSG_STOP) {
			LL_I2C_GenerateStopCondition(i2c);
		}
		if (LL_I2C_IsActiveFlag_BTF(i2c)) {
			/* Read DR to clear BTF flag */
			LL_I2C_ReceiveData8(i2c);
		}
		k_sem_give(&data->device_sync_sem);
	}
}

static inline void handle_rxne(I2C_TypeDef *i2c, struct i2c_stm32_data *data)
{
	if (data->current.len > 0) {
		switch (data->current.len) {
		case 1:
			/* Single byte reception */
			if (data->current.flags & I2C_MSG_STOP) {
				LL_I2C_GenerateStopCondition(i2c);
			}
			LL_I2C_DisableIT_BUF(i2c);
			data->current.len--;
			*data->current.buf = LL_I2C_ReceiveData8(i2c);
			data->current.buf++;
			k_sem_give(&data->device_sync_sem);
			break;
		case 2:
		case 3:
			/*
			 * 2-byte, 3-byte reception and for N-2, N-1,
			 * N when N > 3
			 */
			LL_I2C_DisableIT_BUF(i2c);
			break;
		default:
			/* N byte reception when N > 3 */
			data->current.len--;
			*data->current.buf = LL_I2C_ReceiveData8(i2c);
			data->current.buf++;
		}
	}
}

static inline void handle_btf(I2C_TypeDef *i2c, struct i2c_stm32_data *data)
{
	if (data->current.is_write) {
		handle_txe(i2c, data);
	} else {
		u32_t counter = 0;

		switch (data->current.len) {
		case 2:
			/*
			 * Stop condition must be generated before reading the
			 * last two bytes.
			 */
			if (data->current.flags & I2C_MSG_STOP) {
				LL_I2C_GenerateStopCondition(i2c);
			}

			for (counter = 2; counter > 0; counter--) {
				data->current.len--;
				*data->current.buf = LL_I2C_ReceiveData8(i2c);
				data->current.buf++;
			}
			k_sem_give(&data->device_sync_sem);
			break;
		case 3:
			/* Set NACK before reading N-2 byte*/
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
			data->current.len--;
			*data->current.buf = LL_I2C_ReceiveData8(i2c);
			data->current.buf++;
			break;
		default:
			handle_rxne(i2c, data);
		}
	}
}

void stm32_i2c_event_isr(void *arg)
{
	const struct i2c_stm32_config *cfg = DEV_CFG((struct device *)arg);
	struct i2c_stm32_data *data = DEV_DATA((struct device *)arg);
	I2C_TypeDef *i2c = cfg->i2c;

	if (LL_I2C_IsActiveFlag_SB(i2c)) {
		handle_sb(i2c, data);
	} else if (LL_I2C_IsActiveFlag_ADD10(i2c)) {
		LL_I2C_TransmitData8(i2c, data->slave_address);
	} else if (LL_I2C_IsActiveFlag_ADDR(i2c)) {
		handle_addr(i2c, data);
	} else if (LL_I2C_IsActiveFlag_BTF(i2c)) {
		handle_btf(i2c, data);
	} else if (LL_I2C_IsActiveFlag_TXE(i2c) && data->current.is_write) {
		handle_txe(i2c, data);
	} else if (LL_I2C_IsActiveFlag_RXNE(i2c) && !data->current.is_write) {
		handle_rxne(i2c, data);
	}
}

void stm32_i2c_error_isr(void *arg)
{
	const struct i2c_stm32_config *cfg = DEV_CFG((struct device *)arg);
	struct i2c_stm32_data *data = DEV_DATA((struct device *)arg);
	I2C_TypeDef *i2c = cfg->i2c;

	if (LL_I2C_IsActiveFlag_AF(i2c)) {
		LL_I2C_ClearFlag_AF(i2c);
		data->current.is_nack = 1;
		k_sem_give(&data->device_sync_sem);

		return;
	}
	data->current.is_err = 1;
	k_sem_give(&data->device_sync_sem);
}

s32_t stm32_i2c_msg_write(struct device *dev, struct i2c_msg *msg,
			  u8_t *next_msg_flags, u16_t saddr)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	struct i2c_stm32_data *data = DEV_DATA(dev);
	I2C_TypeDef *i2c = cfg->i2c;
	s32_t ret = 0;

	ARG_UNUSED(next_msg_flags);

	data->current.len = msg->len;
	data->current.buf = msg->buf;
	data->current.flags = msg->flags;
	data->current.is_restart = 0;
	data->current.is_write = 1;
	data->current.is_nack = 0;
	data->current.is_err = 0;
	data->slave_address = saddr;

	LL_I2C_EnableIT_EVT(i2c);
	LL_I2C_EnableIT_ERR(i2c);
	LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
	if (msg->flags & I2C_MSG_RESTART) {
		LL_I2C_GenerateStartCondition(i2c);
	}
	LL_I2C_EnableIT_BUF(i2c);

	k_sem_take(&data->device_sync_sem, K_FOREVER);

	LL_I2C_DisableIT_BUF(i2c);
	if (data->current.is_nack || data->current.is_err) {

		if (data->current.is_nack)
			SYS_LOG_DBG("%s: NACK", __func__);

		if (data->current.is_err)
			SYS_LOG_DBG("%s: ERR %d", __func__,
				    data->current.is_err);

		data->current.is_nack = 0;
		data->current.is_err = 0;
		ret = -EIO;
	}

	LL_I2C_DisableIT_EVT(i2c);
	LL_I2C_DisableIT_ERR(i2c);

	return ret;
}

s32_t stm32_i2c_msg_read(struct device *dev, struct i2c_msg *msg,
			 u8_t *next_msg_flags, u16_t saddr)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	struct i2c_stm32_data *data = DEV_DATA(dev);
	I2C_TypeDef *i2c = cfg->i2c;
	s32_t ret = 0;

	ARG_UNUSED(next_msg_flags);

	data->current.len = msg->len;
	data->current.buf = msg->buf;
	data->current.flags = msg->flags;
	data->current.is_restart = 0;
	data->current.is_write = 0;
	data->current.is_err = 0;
	data->slave_address = saddr;

	LL_I2C_EnableIT_EVT(i2c);
	LL_I2C_EnableIT_ERR(i2c);
	LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
	LL_I2C_GenerateStartCondition(i2c);
	LL_I2C_EnableIT_BUF(i2c);

	k_sem_take(&data->device_sync_sem, K_FOREVER);

	LL_I2C_DisableIT_BUF(i2c);
	if (data->current.is_err) {
		SYS_LOG_DBG("%s: ERR %d", __func__, data->current.is_err);
		data->current.is_err = 0;
		ret = -EIO;
	}

	LL_I2C_DisableIT_EVT(i2c);
	LL_I2C_DisableIT_ERR(i2c);

	return ret;
}

#else

s32_t stm32_i2c_msg_write(struct device *dev, struct i2c_msg *msg,
			  u8_t *next_msg_flags, u16_t saddr)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	struct i2c_stm32_data *data = DEV_DATA(dev);
	I2C_TypeDef *i2c = cfg->i2c;
	u32_t len = msg->len;
	u8_t *buf = msg->buf;

	ARG_UNUSED(next_msg_flags);

	LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);

	if (msg->flags & I2C_MSG_RESTART) {
		LL_I2C_GenerateStartCondition(i2c);
		while (!LL_I2C_IsActiveFlag_SB(i2c)) {
			;
		}

		if (I2C_ADDR_10_BITS & data->dev_config) {
			u8_t slave = (((saddr & 0x0300) >> 7) & 0xFF);
			u8_t header = slave | HEADER;

			LL_I2C_TransmitData8(i2c, header);
			while (!LL_I2C_IsActiveFlag_ADD10(i2c)) {
				;
			}
			slave = data->slave_address & 0xFF;
			LL_I2C_TransmitData8(i2c, slave);
		} else {
			u8_t slave = (saddr << 1) & 0xFF;

			LL_I2C_TransmitData8(i2c, slave | I2C_REQUEST_WRITE);
		}
		while (!LL_I2C_IsActiveFlag_ADDR(i2c)) {
			;
		}
		LL_I2C_ClearFlag_ADDR(i2c);
	}

	while (len) {
		while (1) {
			if (LL_I2C_IsActiveFlag_TXE(i2c)) {
				break;
			}
			if (LL_I2C_IsActiveFlag_AF(i2c)) {
				LL_I2C_ClearFlag_AF(i2c);
				SYS_LOG_DBG("%s: NACK", __func__);

				return -EIO;
			}
		};
		LL_I2C_TransmitData8(i2c, *buf);
		buf++;
		len--;
	}

	while (!LL_I2C_IsActiveFlag_BTF(i2c)) {
		;
	}

	if (msg->flags & I2C_MSG_STOP) {
		LL_I2C_GenerateStopCondition(i2c);
	}

	return 0;
}

s32_t stm32_i2c_msg_read(struct device *dev, struct i2c_msg *msg,
			 u8_t *next_msg_flags, u16_t saddr)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	struct i2c_stm32_data *data = DEV_DATA(dev);
	I2C_TypeDef *i2c = cfg->i2c;
	u32_t len = msg->len;
	u8_t *buf = msg->buf;

	ARG_UNUSED(next_msg_flags);

	LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);

	if (msg->flags & I2C_MSG_RESTART) {
		LL_I2C_GenerateStartCondition(i2c);
		while (!LL_I2C_IsActiveFlag_SB(i2c)) {
			;
		}

		if (I2C_ADDR_10_BITS & data->dev_config) {
			u8_t slave = (((saddr &	0x0300) >> 7) & 0xFF);
			u8_t header = slave | HEADER;

			LL_I2C_TransmitData8(i2c, header);
			while (!LL_I2C_IsActiveFlag_ADD10(i2c)) {
				;
			}
			slave = saddr & 0xFF;
			LL_I2C_TransmitData8(i2c, slave);
			while (!LL_I2C_IsActiveFlag_ADDR(i2c)) {
				;
			}
			LL_I2C_ClearFlag_ADDR(i2c);
			LL_I2C_GenerateStartCondition(i2c);
			while (!LL_I2C_IsActiveFlag_SB(i2c)) {
				;
			}
			header |= I2C_REQUEST_READ;
			LL_I2C_TransmitData8(i2c, header);
		} else {
			u8_t slave = ((saddr) << 1) & 0xFF;

			LL_I2C_TransmitData8(i2c, slave | I2C_REQUEST_READ);
		}

		while (!LL_I2C_IsActiveFlag_ADDR(i2c)) {
			;
		}

		if (len == 1) {
			/* Single byte reception: enable NACK and set STOP */
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
		} else if (len == 2) {
			/* 2-byte reception: enable NACK and set POS */
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
			LL_I2C_EnableBitPOS(i2c);
		}

		LL_I2C_ClearFlag_ADDR(i2c);
	}

	while (len) {
		while (!LL_I2C_IsActiveFlag_RXNE(i2c)) {
			;
		}
		switch (len) {
		case 1:
			if (msg->flags & I2C_MSG_STOP) {
				LL_I2C_GenerateStopCondition(i2c);
			}
			len--;
			*buf = LL_I2C_ReceiveData8(i2c);
			buf++;
			break;
		case 2:
			while (!LL_I2C_IsActiveFlag_BTF(i2c)) {
				;
			}
			/*
			 * Stop condition must be generated before reading the
			 * last two bytes.
			 */
			if (msg->flags & I2C_MSG_STOP) {
				LL_I2C_GenerateStopCondition(i2c);
			}

			for (u32_t counter = 2; counter > 0; counter--) {
				len--;
				*data->current.buf = LL_I2C_ReceiveData8(i2c);
				buf++;
			}

			break;
		case 3:
			/* Set NACK before reading N-2 byte*/
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
			/* Fall through */
		default:
			len--;
			*buf = LL_I2C_ReceiveData8(i2c);
			buf++;
		}
	}

	return 0;
}
#endif

s32_t stm32_i2c_configure_timing(struct device *dev, u32_t clock)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	struct i2c_stm32_data *data = DEV_DATA(dev);
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
