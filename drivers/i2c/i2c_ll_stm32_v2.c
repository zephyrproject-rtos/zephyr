/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C Driver for: STM32F0, STM32F3, STM32F7, STM32L0 and STM32L4
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

static inline void msg_init(struct device *dev, struct i2c_msg *msg,
			    u8_t *next_msg_flags, u16_t slave,
			    uint32_t transfer)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	struct i2c_stm32_data *data = DEV_DATA(dev);
	I2C_TypeDef *i2c = cfg->i2c;

	if (LL_I2C_IsEnabledReloadMode(i2c)) {
		LL_I2C_SetTransferSize(i2c, msg->len);
	} else {
		if (I2C_ADDR_10_BITS & data->dev_config) {
			LL_I2C_SetMasterAddressingMode(i2c,
					LL_I2C_ADDRESSING_MODE_10BIT);
			LL_I2C_SetSlaveAddr(i2c, (uint32_t) slave);
		} else {
			LL_I2C_SetMasterAddressingMode(i2c,
				LL_I2C_ADDRESSING_MODE_7BIT);
			LL_I2C_SetSlaveAddr(i2c, (uint32_t) slave << 1);
		}

		if (!(msg->flags & I2C_MSG_STOP) && next_msg_flags &&
		    !(*next_msg_flags & I2C_MSG_RESTART)) {
			LL_I2C_EnableReloadMode(i2c);
		} else {
			LL_I2C_DisableReloadMode(i2c);
		}
		LL_I2C_DisableAutoEndMode(i2c);
		LL_I2C_SetTransferRequest(i2c, transfer);
		LL_I2C_SetTransferSize(i2c, msg->len);
		LL_I2C_GenerateStartCondition(i2c);
	}
}

#ifdef CONFIG_I2C_STM32_INTERRUPT
void stm32_i2c_event_isr(void *arg)
{
	const struct i2c_stm32_config *cfg = DEV_CFG((struct device *)arg);
	struct i2c_stm32_data *data = DEV_DATA((struct device *)arg);
	I2C_TypeDef *i2c = cfg->i2c;

	if (data->current.len) {
		/* Interrupts for sending/receiving next byte */

		if (data->current.is_write && LL_I2C_IsActiveFlag_TXIS(i2c)) {
			/* Send next byte */
			LL_I2C_TransmitData8(i2c, *data->current.buf);
		} else if (!(data->current.is_write) &&
			   LL_I2C_IsActiveFlag_RXNE(i2c)) {
			/* Receive next byte */
			*data->current.buf = LL_I2C_ReceiveData8(i2c);
		} else {
			goto error;
		}

		data->current.buf++;
		data->current.len--;
		if (!data->current.len) {
			LL_I2C_EnableIT_TC(i2c);
		}

	} else {
		/* Interrupts to process end of message */

		if (LL_I2C_IsActiveFlag_TC(i2c) ||
		    LL_I2C_IsActiveFlag_TCR(i2c)) {
			/* Interrupt after last byte of message
			 * was transferred
			 */
			LL_I2C_DisableIT_TC(i2c);

			/* Issue stop condition if necessary */
			if (data->current.msg->flags & I2C_MSG_STOP) {
				LL_I2C_GenerateStopCondition(i2c);
				LL_I2C_EnableIT_STOP(i2c);
			} else {
				k_sem_give(&data->device_sync_sem);
			}

		} else if (LL_I2C_IsActiveFlag_STOP(i2c)) {
			/* Interrupt after stop condition was
			 * successfully issued
			 */
			LL_I2C_DisableIT_STOP(i2c);
			LL_I2C_ClearFlag_STOP(i2c);
			LL_I2C_DisableReloadMode(i2c);
			k_sem_give(&data->device_sync_sem);
		} else {
			goto error;
		}
	}

	return;
error:
	LL_I2C_DisableIT_TX(i2c);
	LL_I2C_DisableIT_RX(i2c);
	data->current.is_err = 1;
	k_sem_give(&data->device_sync_sem);
}

#ifndef CONFIG_I2C_STM32_COMBINED_INTERRUPT
void stm32_i2c_error_isr(void *arg)
{
	const struct i2c_stm32_config *cfg = DEV_CFG((struct device *)arg);
	struct i2c_stm32_data *data = DEV_DATA((struct device *)arg);
	I2C_TypeDef *i2c = cfg->i2c;

	if (LL_I2C_IsActiveFlag_NACK(i2c)) {
		LL_I2C_ClearFlag_NACK(i2c);
		data->current.is_nack = 1;
	} else {
		data->current.is_err = 1;
	}

	k_sem_give(&data->device_sync_sem);
}
#endif

#ifdef CONFIG_I2C_STM32_COMBINED_INTERRUPT
void stm32_i2c_combined_isr(void *arg)
{
	const struct i2c_stm32_config *cfg = DEV_CFG((struct device *)arg);
	struct i2c_stm32_data *data = DEV_DATA((struct device *)arg);
	I2C_TypeDef *i2c = cfg->i2c;

	if (LL_I2C_IsActiveFlag_NACK(i2c)) {
		LL_I2C_ClearFlag_NACK(i2c);
		data->current.is_nack = 1;

		k_sem_give(&data->device_sync_sem);
	} else {
		stm32_i2c_event_isr(arg);
	}
}
#endif

int stm32_i2c_msg_write(struct device *dev, struct i2c_msg *msg,
			u8_t *next_msg_flags, uint16_t slave)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	struct i2c_stm32_data *data = DEV_DATA(dev);
	I2C_TypeDef *i2c = cfg->i2c;

	data->current.len = msg->len;
	data->current.buf = msg->buf;
	data->current.is_write = 1;
	data->current.is_nack = 0;
	data->current.is_err = 0;
	data->current.msg = msg;

	msg_init(dev, msg, next_msg_flags, slave, LL_I2C_REQUEST_WRITE);
	LL_I2C_EnableIT_TX(i2c);
	LL_I2C_EnableIT_NACK(i2c);

	k_sem_take(&data->device_sync_sem, K_FOREVER);
	LL_I2C_DisableIT_TX(i2c);
	LL_I2C_DisableIT_NACK(i2c);

	if (data->current.is_nack || data->current.is_err) {
		goto error;
	}

	return 0;
error:
	if (data->current.is_nack) {
		SYS_LOG_DBG("%s: NACK", __func__);
		data->current.is_nack = 0;
	}

	if (data->current.is_err) {
		SYS_LOG_DBG("%s: ERR %d", __func__,
				    data->current.is_err);
		data->current.is_err = 0;
	}

	return -EIO;
}

int stm32_i2c_msg_read(struct device *dev, struct i2c_msg *msg,
		       u8_t *next_msg_flags, uint16_t slave)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	struct i2c_stm32_data *data = DEV_DATA(dev);
	I2C_TypeDef *i2c = cfg->i2c;

	data->current.len = msg->len;
	data->current.buf = msg->buf;
	data->current.is_write = 0;
	data->current.is_err = 0;
	data->current.msg = msg;

	msg_init(dev, msg, next_msg_flags, slave, LL_I2C_REQUEST_READ);
	LL_I2C_EnableIT_RX(i2c);

	k_sem_take(&data->device_sync_sem, K_FOREVER);
	LL_I2C_DisableIT_RX(i2c);

	if (data->current.is_err) {
		goto error;
	}

	return 0;
error:
	SYS_LOG_DBG("%s: ERR %d", __func__, data->current.is_err);
	data->current.is_err = 0;

	return -EIO;
}

#else /* !CONFIG_I2C_STM32_INTERRUPT */
static inline void msg_done(struct device *dev, unsigned int current_msg_flags)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	I2C_TypeDef *i2c = cfg->i2c;

	/* Wait for transfer to complete */
	while (!LL_I2C_IsActiveFlag_TC(i2c) && !LL_I2C_IsActiveFlag_TCR(i2c)) {
		;
	}
	/* Issue stop condition if necessary */
	if (current_msg_flags & I2C_MSG_STOP) {
		LL_I2C_GenerateStopCondition(i2c);
		while (!LL_I2C_IsActiveFlag_STOP(i2c)) {
			;
		}
		LL_I2C_ClearFlag_STOP(i2c);
		LL_I2C_DisableReloadMode(i2c);
	}
}

int stm32_i2c_msg_write(struct device *dev, struct i2c_msg *msg,
			u8_t *next_msg_flags, uint16_t slave)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	I2C_TypeDef *i2c = cfg->i2c;
	unsigned int len = 0;
	u8_t *buf = msg->buf;

	msg_init(dev, msg, next_msg_flags, slave, LL_I2C_REQUEST_WRITE);

	len = msg->len;
	while (len) {
		while (1) {
			if (LL_I2C_IsActiveFlag_TXIS(i2c)) {
				break;
			}

			if (LL_I2C_IsActiveFlag_NACK(i2c)) {
				goto error;
			}
		}

		LL_I2C_TransmitData8(i2c, *buf);
		buf++;
		len--;
	}

	msg_done(dev, msg->flags);

	return 0;
error:
	LL_I2C_ClearFlag_NACK(i2c);
	SYS_LOG_DBG("%s: NACK", __func__);

	return -EIO;
}

int stm32_i2c_msg_read(struct device *dev, struct i2c_msg *msg,
		       u8_t *next_msg_flags, uint16_t slave)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	I2C_TypeDef *i2c = cfg->i2c;
	unsigned int len = 0;
	u8_t *buf = msg->buf;

	msg_init(dev, msg, next_msg_flags, slave, LL_I2C_REQUEST_READ);

	len = msg->len;
	while (len) {
		while (!LL_I2C_IsActiveFlag_RXNE(i2c)) {
			;
		}

		*buf = LL_I2C_ReceiveData8(i2c);
		buf++;
		len--;
	}

	msg_done(dev, msg->flags);

	return 0;
}
#endif

int stm32_i2c_configure_timing(struct device *dev, u32_t clock)
{
	const struct i2c_stm32_config *cfg = DEV_CFG(dev);
	struct i2c_stm32_data *data = DEV_DATA(dev);
	I2C_TypeDef *i2c = cfg->i2c;
	u32_t i2c_hold_time_min, i2c_setup_time_min;
	u32_t i2c_h_min_time, i2c_l_min_time;
	u32_t presc = 1;
	u32_t timing = 0;

	switch (I2C_SPEED_GET(data->dev_config)) {
	case I2C_SPEED_STANDARD:
		i2c_h_min_time = 4000;
		i2c_l_min_time = 4700;
		i2c_hold_time_min = 500;
		i2c_setup_time_min = 1250;
		break;
	case I2C_SPEED_FAST:
		i2c_h_min_time = 600;
		i2c_l_min_time = 1300;
		i2c_hold_time_min = 375;
		i2c_setup_time_min = 500;
		break;
	default:
		return -EINVAL;
	}

	/* Calculate period until prescaler matches */
	do {
		u32_t t_presc = clock / presc;
		u32_t ns_presc = NSEC_PER_SEC / t_presc;
		u32_t sclh = i2c_h_min_time / ns_presc;
		u32_t scll = i2c_l_min_time / ns_presc;
		u32_t sdadel = i2c_hold_time_min / ns_presc;
		u32_t scldel = i2c_setup_time_min / ns_presc;

		if ((sclh - 1) > 255 ||  (scll - 1) > 255) {
			++presc;
			continue;
		}

		if (sdadel > 15 || (scldel - 1) > 15) {
			++presc;
			continue;
		}

		timing = __LL_I2C_CONVERT_TIMINGS(presc - 1,
					scldel - 1, sdadel, sclh - 1, scll - 1);
		break;
	} while (presc < 16);

	if (presc >= 16) {
		SYS_LOG_DBG("I2C:failed to find prescaler value");
		return -EINVAL;
	}

	LL_I2C_SetTiming(i2c, timing);

	return 0;
}
