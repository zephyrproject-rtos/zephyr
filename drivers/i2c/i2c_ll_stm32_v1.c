/*
 * Copyright (c) 2017, I-SENSE group of ICCS
 * Copyright (c) 2017 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C Driver for: STM32F1, STM32F2, STM32F4 and STM32L1
 *
 */

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <stm32_ll_i2c.h>
#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include "i2c_ll_stm32.h"

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ll_stm32_v1);

#include "i2c-priv.h"

#define STM32_I2C_TRANSFER_TIMEOUT_MSEC  500

#define STM32_I2C_TIMEOUT_USEC  1000
#define I2C_REQUEST_WRITE       0x00
#define I2C_REQUEST_READ        0x01
#define HEADER                  0xF0

static void stm32_i2c_generate_start_condition(I2C_TypeDef *i2c)
{
	uint16_t cr1 = LL_I2C_ReadReg(i2c, CR1);

	if (cr1 & I2C_CR1_STOP) {
		LOG_DBG("%s: START while STOP active!", __func__);
		LL_I2C_WriteReg(i2c, CR1, cr1 & ~I2C_CR1_STOP);
	}

	LL_I2C_GenerateStartCondition(i2c);
}

#ifdef CONFIG_I2C_STM32_INTERRUPT

static void stm32_i2c_disable_transfer_interrupts(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_DisableIT_TX(i2c);
	LL_I2C_DisableIT_RX(i2c);
	LL_I2C_DisableIT_EVT(i2c);
	LL_I2C_DisableIT_BUF(i2c);

	if (!data->smbalert_active) {
		LL_I2C_DisableIT_ERR(i2c);
	}
}

static void stm32_i2c_enable_transfer_interrupts(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_EnableIT_ERR(i2c);
	LL_I2C_EnableIT_EVT(i2c);
	LL_I2C_EnableIT_BUF(i2c);
}

#endif /* CONFIG_I2C_STM32_INTERRUPT */

static void stm32_i2c_reset(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;
	uint16_t cr1, cr2, oar1, oar2, trise, ccr;
#if defined(I2C_FLTR_ANOFF) && defined(I2C_FLTR_DNF)
	uint16_t fltr;
#endif

	/* disable i2c and disable IRQ's */
	LL_I2C_Disable(i2c);
#ifdef CONFIG_I2C_STM32_INTERRUPT
	stm32_i2c_disable_transfer_interrupts(dev);
#endif

	/* save all important registers before reset */
	cr1 = LL_I2C_ReadReg(i2c, CR1);
	cr2 = LL_I2C_ReadReg(i2c, CR2);
	oar1 = LL_I2C_ReadReg(i2c, OAR1);
	oar2 = LL_I2C_ReadReg(i2c, OAR2);
	ccr = LL_I2C_ReadReg(i2c, CCR);
	trise = LL_I2C_ReadReg(i2c, TRISE);
#if defined(I2C_FLTR_ANOFF) && defined(I2C_FLTR_DNF)
	fltr = LL_I2C_ReadReg(i2c, FLTR);
#endif

	/* reset i2c hardware */
	LL_I2C_EnableReset(i2c);
	LL_I2C_DisableReset(i2c);

	/* restore all important registers after reset */
	LL_I2C_WriteReg(i2c, CR1, cr1);
	LL_I2C_WriteReg(i2c, CR2, cr2);

	/* bit 14 of OAR1 must always be 1 */
	oar1 |= (1 << 14);
	LL_I2C_WriteReg(i2c, OAR1, oar1);
	LL_I2C_WriteReg(i2c, OAR2, oar2);
	LL_I2C_WriteReg(i2c, CCR, ccr);
	LL_I2C_WriteReg(i2c, TRISE, trise);
#if defined(I2C_FLTR_ANOFF) && defined(I2C_FLTR_DNF)
	LL_I2C_WriteReg(i2c, FLTR, fltr);
#endif
}


static void stm32_i2c_master_finish(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

#ifdef CONFIG_I2C_STM32_INTERRUPT
	stm32_i2c_disable_transfer_interrupts(dev);
#endif

#if defined(CONFIG_I2C_TARGET)
	data->master_active = false;
	if (!data->slave_attached && !data->smbalert_active) {
		LL_I2C_Disable(i2c);
	} else {
		stm32_i2c_enable_transfer_interrupts(dev);
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
	}
#else
	if (!data->smbalert_active) {
		LL_I2C_Disable(i2c);
	}
#endif
}

static inline void msg_init(const struct device *dev, struct i2c_msg *msg,
			    uint8_t *next_msg_flags, uint16_t slave,
			    uint32_t transfer)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	ARG_UNUSED(next_msg_flags);

#ifdef CONFIG_I2C_STM32_INTERRUPT
	k_sem_reset(&data->device_sync_sem);
#endif

	data->current.len = msg->len;
	data->current.buf = msg->buf;
	data->current.flags = msg->flags;
	data->current.is_restart = 0U;
	data->current.is_write = (transfer == I2C_REQUEST_WRITE);
	data->current.is_arlo = 0U;
	data->current.is_err = 0U;
	data->current.is_nack = 0U;
	data->current.msg = msg;
#if defined(CONFIG_I2C_TARGET)
	data->master_active = true;
#endif
	data->slave_address = slave;

	LL_I2C_Enable(i2c);

	LL_I2C_DisableBitPOS(i2c);
	LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
	if (msg->flags & I2C_MSG_RESTART) {
		stm32_i2c_generate_start_condition(i2c);
	}
}

static int32_t msg_end(const struct device *dev, uint8_t *next_msg_flags,
		       const char *funcname)
{
	struct i2c_stm32_data *data = dev->data;

	if (data->current.is_nack || data->current.is_err ||
	    data->current.is_arlo) {
		goto error;
	}

	if (!next_msg_flags) {
		stm32_i2c_master_finish(dev);
	}

	return 0;

error:
	if (data->current.is_arlo) {
		LOG_DBG("%s: ARLO %d", funcname,
			data->current.is_arlo);
		data->current.is_arlo = 0U;
	}

	if (data->current.is_nack) {
		LOG_DBG("%s: NACK", funcname);
		data->current.is_nack = 0U;
	}

	if (data->current.is_err) {
		LOG_DBG("%s: ERR %d", funcname,
			data->current.is_err);
		data->current.is_err = 0U;
	}
	stm32_i2c_master_finish(dev);

	return -EIO;
}

#ifdef CONFIG_I2C_STM32_INTERRUPT

static void stm32_i2c_master_mode_end(const struct device *dev)
{
	struct i2c_stm32_data *data = dev->data;

	k_sem_give(&data->device_sync_sem);
}

static inline void handle_sb(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	uint16_t saddr = data->slave_address;
	uint8_t slave;

	if (I2C_ADDR_10_BITS & data->dev_config) {
		slave = (((saddr & 0x0300) >> 7) & 0xFF);
		uint8_t header = slave | HEADER;

		if (data->current.is_restart == 0U) {
			data->current.is_restart = 1U;
		} else {
			header |= I2C_REQUEST_READ;
			data->current.is_restart = 0U;
		}
		LL_I2C_TransmitData8(i2c, header);

		return;
	}
	slave = (saddr << 1) & 0xFF;
	if (data->current.is_write) {
		LL_I2C_TransmitData8(i2c, slave | I2C_REQUEST_WRITE);
	} else {
		LL_I2C_TransmitData8(i2c, slave | I2C_REQUEST_READ);
		if (data->current.len == 2) {
			LL_I2C_EnableBitPOS(i2c);
		}
	}
}

static inline void handle_addr(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if (I2C_ADDR_10_BITS & data->dev_config) {
		if (!data->current.is_write && data->current.is_restart) {
			data->current.is_restart = 0U;
			LL_I2C_ClearFlag_ADDR(i2c);
			stm32_i2c_generate_start_condition(i2c);

			return;
		}
	}

	if (data->current.is_write) {
		LL_I2C_ClearFlag_ADDR(i2c);
		return;
	}
	/* according to STM32F1 errata we need to handle these corner cases in
	 * specific way.
	 * Please ref to STM32F10xxC/D/E I2C peripheral Errata sheet 2.14.1
	 */
	if (data->current.len == 0U && IS_ENABLED(CONFIG_SOC_SERIES_STM32F1X)) {
		LL_I2C_GenerateStopCondition(i2c);
	} else if (data->current.len == 1U) {
		/* Single byte reception: enable NACK and clear POS */
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
#ifdef CONFIG_SOC_SERIES_STM32F1X
		LL_I2C_ClearFlag_ADDR(i2c);
		LL_I2C_GenerateStopCondition(i2c);
#endif
	} else if (data->current.len == 2U) {
#ifdef CONFIG_SOC_SERIES_STM32F1X
		LL_I2C_ClearFlag_ADDR(i2c);
#endif
		/* 2-byte reception: enable NACK and set POS */
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
		LL_I2C_EnableBitPOS(i2c);
	}
	LL_I2C_ClearFlag_ADDR(i2c);
}

static inline void handle_txe(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if (data->current.len) {
		data->current.len--;
		if (data->current.len == 0U) {
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

static inline void handle_rxne(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if (data->current.len > 0) {
		switch (data->current.len) {
		case 1:
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
			LL_I2C_DisableBitPOS(i2c);
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
			/*
			 * 2-byte reception for N > 3 has already set the NACK
			 * bit, and must not set the POS bit. See pg. 854 in
			 * the F4 reference manual (RM0090).
			 */
			if (data->current.msg->len > 2) {
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
			data->current.len--;
			*data->current.buf = LL_I2C_ReceiveData8(i2c);
			data->current.buf++;
		}
	} else {

		if (data->current.flags & I2C_MSG_STOP) {
			LL_I2C_GenerateStopCondition(i2c);
		}
		k_sem_give(&data->device_sync_sem);
	}
}

static inline void handle_btf(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if (data->current.is_write) {
		handle_txe(dev);
	} else {
		uint32_t counter = 0U;

		switch (data->current.len) {
		case 2:
			/*
			 * Stop condition must be generated before reading the
			 * last two bytes.
			 */
			if (data->current.flags & I2C_MSG_STOP) {
				LL_I2C_GenerateStopCondition(i2c);
			}

			for (counter = 2U; counter > 0; counter--) {
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
			handle_rxne(dev);
		}
	}
}


#if defined(CONFIG_I2C_TARGET)
static void stm32_i2c_slave_event(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	const struct i2c_target_callbacks *slave_cb =
		data->slave_cfg->callbacks;

	if (LL_I2C_IsActiveFlag_TXE(i2c) && LL_I2C_IsActiveFlag_BTF(i2c)) {
		uint8_t val;
		slave_cb->read_processed(data->slave_cfg, &val);
		LL_I2C_TransmitData8(i2c, val);
		return;
	}

	if (LL_I2C_IsActiveFlag_RXNE(i2c)) {
		uint8_t val = LL_I2C_ReceiveData8(i2c);
		if (slave_cb->write_received(data->slave_cfg, val)) {
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
		}
		return;
	}

	if (LL_I2C_IsActiveFlag_AF(i2c)) {
		LL_I2C_ClearFlag_AF(i2c);
	}

	if (LL_I2C_IsActiveFlag_STOP(i2c)) {
		LL_I2C_ClearFlag_STOP(i2c);
		slave_cb->stop(data->slave_cfg);
		/* Prepare to ACK next transmissions address byte */
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
	}

	if (LL_I2C_IsActiveFlag_ADDR(i2c)) {
		uint32_t dir = LL_I2C_GetTransferDirection(i2c);
		if (dir == LL_I2C_DIRECTION_READ) {
			slave_cb->write_requested(data->slave_cfg);
			LL_I2C_EnableIT_RX(i2c);
		} else {
			uint8_t val;
			slave_cb->read_requested(data->slave_cfg, &val);
			LL_I2C_TransmitData8(i2c, val);
			LL_I2C_EnableIT_TX(i2c);
		}

		stm32_i2c_enable_transfer_interrupts(dev);
	}
}

/* Attach and start I2C as slave */
int i2c_stm32_target_register(const struct device *dev, struct i2c_target_config *config)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t bitrate_cfg;
	int ret;

	if (!config) {
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

	stm32_i2c_enable_transfer_interrupts(dev);
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

	stm32_i2c_disable_transfer_interrupts(dev);

	LL_I2C_ClearFlag_AF(i2c);
	LL_I2C_ClearFlag_STOP(i2c);
	LL_I2C_ClearFlag_ADDR(i2c);

	if (!data->smbalert_active) {
		LL_I2C_Disable(i2c);
	}

	data->slave_attached = false;

	LOG_DBG("i2c: slave unregistered");

	return 0;
}
#endif /* defined(CONFIG_I2C_TARGET) */

void stm32_i2c_event_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

#if defined(CONFIG_I2C_TARGET)
	if (data->slave_attached && !data->master_active) {
		stm32_i2c_slave_event(dev);
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
	} else if (LL_I2C_IsActiveFlag_TXE(i2c) && data->current.is_write) {
		handle_txe(dev);
	} else if (LL_I2C_IsActiveFlag_RXNE(i2c) && !data->current.is_write) {
		handle_rxne(dev);
	}
}

void stm32_i2c_error_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

#if defined(CONFIG_I2C_TARGET)
	if (data->slave_attached && !data->master_active) {
		/* No need for a slave error function right now. */
		return;
	}
#endif

	if (LL_I2C_IsActiveFlag_AF(i2c)) {
		LL_I2C_ClearFlag_AF(i2c);
		LL_I2C_GenerateStopCondition(i2c);
		data->current.is_nack = 1U;
		goto end;
	}
	if (LL_I2C_IsActiveFlag_ARLO(i2c)) {
		LL_I2C_ClearFlag_ARLO(i2c);
		data->current.is_arlo = 1U;
		goto end;
	}

	if (LL_I2C_IsActiveFlag_BERR(i2c)) {
		LL_I2C_ClearFlag_BERR(i2c);
		data->current.is_err = 1U;
		goto end;
	}

#if defined(CONFIG_SMBUS_STM32_SMBALERT)
	if (LL_I2C_IsActiveSMBusFlag_ALERT(i2c)) {
		LL_I2C_ClearSMBusFlag_ALERT(i2c);
		if (data->smbalert_cb_func != NULL) {
			data->smbalert_cb_func(data->smbalert_cb_dev);
		}
		goto end;
	}
#endif
	return;
end:
	stm32_i2c_master_mode_end(dev);
}

static int32_t stm32_i2c_msg_write(const struct device *dev, struct i2c_msg *msg,
			    uint8_t *next_msg_flags, uint16_t saddr)
{
	struct i2c_stm32_data *data = dev->data;

	msg_init(dev, msg, next_msg_flags, saddr, I2C_REQUEST_WRITE);

	stm32_i2c_enable_transfer_interrupts(dev);

	if (k_sem_take(&data->device_sync_sem,
			K_MSEC(STM32_I2C_TRANSFER_TIMEOUT_MSEC)) != 0) {
		LOG_DBG("%s: WRITE timeout", __func__);
		stm32_i2c_reset(dev);
		return -EIO;
	}

	return msg_end(dev, next_msg_flags, __func__);
}

static int32_t stm32_i2c_msg_read(const struct device *dev, struct i2c_msg *msg,
			   uint8_t *next_msg_flags, uint16_t saddr)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	msg_init(dev, msg, next_msg_flags, saddr, I2C_REQUEST_READ);

	stm32_i2c_enable_transfer_interrupts(dev);
	LL_I2C_EnableIT_RX(i2c);

	if (k_sem_take(&data->device_sync_sem,
			K_MSEC(STM32_I2C_TRANSFER_TIMEOUT_MSEC)) != 0) {
		LOG_DBG("%s: READ timeout", __func__);
		stm32_i2c_reset(dev);
		return -EIO;
	}

	return msg_end(dev, next_msg_flags, __func__);
}

#else /* CONFIG_I2C_STM32_INTERRUPT */

static inline int check_errors(const struct device *dev, const char *funcname)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if (LL_I2C_IsActiveFlag_AF(i2c)) {
		LL_I2C_ClearFlag_AF(i2c);
		LOG_DBG("%s: NACK", funcname);
		data->current.is_nack = 1U;
		goto error;
	}

	if (LL_I2C_IsActiveFlag_ARLO(i2c)) {
		LL_I2C_ClearFlag_ARLO(i2c);
		LOG_DBG("%s: ARLO", funcname);
		data->current.is_arlo = 1U;
		goto error;
	}

	if (LL_I2C_IsActiveFlag_OVR(i2c)) {
		LL_I2C_ClearFlag_OVR(i2c);
		LOG_DBG("%s: OVR", funcname);
		data->current.is_err = 1U;
		goto error;
	}

	if (LL_I2C_IsActiveFlag_BERR(i2c)) {
		LL_I2C_ClearFlag_BERR(i2c);
		LOG_DBG("%s: BERR", funcname);
		data->current.is_err = 1U;
		goto error;
	}

	return 0;
error:
	return -EIO;
}

static int stm32_i2c_wait_timeout(uint16_t *timeout)
{
	if (*timeout == 0) {
		return 1;
	} else {
		k_busy_wait(1);
		(*timeout)--;
		return 0;
	}
}

static int32_t stm32_i2c_msg_write(const struct device *dev, struct i2c_msg *msg,
			    uint8_t *next_msg_flags, uint16_t saddr)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t len = msg->len;
	uint16_t timeout;
	uint8_t *buf = msg->buf;
	int32_t res;

	msg_init(dev, msg, next_msg_flags, saddr, I2C_REQUEST_WRITE);

	if (msg->flags & I2C_MSG_RESTART) {
		timeout = STM32_I2C_TIMEOUT_USEC;
		while (!LL_I2C_IsActiveFlag_SB(i2c)) {
			if (stm32_i2c_wait_timeout(&timeout)) {
				LL_I2C_GenerateStopCondition(i2c);
				data->current.is_err = 1U;
				goto end;
			}
		}

		if (I2C_ADDR_10_BITS & data->dev_config) {
			uint8_t slave = (((saddr & 0x0300) >> 7) & 0xFF);
			uint8_t header = slave | HEADER;

			LL_I2C_TransmitData8(i2c, header);
			timeout = STM32_I2C_TIMEOUT_USEC;
			while (!LL_I2C_IsActiveFlag_ADD10(i2c)) {
				if (stm32_i2c_wait_timeout(&timeout)) {
					LL_I2C_GenerateStopCondition(i2c);
					data->current.is_err = 1U;
					goto end;
				}
			}

			slave = data->slave_address & 0xFF;
			LL_I2C_TransmitData8(i2c, slave);
		} else {
			uint8_t slave = (saddr << 1) & 0xFF;

			LL_I2C_TransmitData8(i2c, slave | I2C_REQUEST_WRITE);
		}

		timeout = STM32_I2C_TIMEOUT_USEC;
		while (!LL_I2C_IsActiveFlag_ADDR(i2c)) {
			if (LL_I2C_IsActiveFlag_AF(i2c) || stm32_i2c_wait_timeout(&timeout)) {
				LL_I2C_ClearFlag_AF(i2c);
				LL_I2C_GenerateStopCondition(i2c);
				data->current.is_nack = 1U;
				goto end;
			}
		}
		LL_I2C_ClearFlag_ADDR(i2c);
	}

	while (len) {
		timeout = STM32_I2C_TIMEOUT_USEC;
		while (1) {
			if (LL_I2C_IsActiveFlag_TXE(i2c)) {
				break;
			}
			if (LL_I2C_IsActiveFlag_AF(i2c) || stm32_i2c_wait_timeout(&timeout)) {
				LL_I2C_ClearFlag_AF(i2c);
				LL_I2C_GenerateStopCondition(i2c);
				data->current.is_nack = 1U;
				goto end;
			}
		}
		LL_I2C_TransmitData8(i2c, *buf);
		buf++;
		len--;
	}

	timeout = STM32_I2C_TIMEOUT_USEC;
	while (!LL_I2C_IsActiveFlag_BTF(i2c)) {
		if (stm32_i2c_wait_timeout(&timeout)) {
			LL_I2C_GenerateStopCondition(i2c);
			data->current.is_err = 1U;
			goto end;
		}
	}

	if (msg->flags & I2C_MSG_STOP) {
		LL_I2C_GenerateStopCondition(i2c);
	}

end:
	check_errors(dev, __func__);
	res = msg_end(dev, next_msg_flags, __func__);
	if (res < 0) {
		stm32_i2c_reset(dev);
	}

	return res;
}

static int32_t stm32_i2c_msg_read(const struct device *dev, struct i2c_msg *msg,
			   uint8_t *next_msg_flags, uint16_t saddr)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t len = msg->len;
	uint16_t timeout;
	uint8_t *buf = msg->buf;
	int32_t res;

	msg_init(dev, msg, next_msg_flags, saddr, I2C_REQUEST_READ);

	if (msg->flags & I2C_MSG_RESTART) {
		timeout = STM32_I2C_TIMEOUT_USEC;
		while (!LL_I2C_IsActiveFlag_SB(i2c)) {
			if (stm32_i2c_wait_timeout(&timeout)) {
				LL_I2C_GenerateStopCondition(i2c);
				data->current.is_err = 1U;
				goto end;
			}
		}

		if (I2C_ADDR_10_BITS & data->dev_config) {
			uint8_t slave = (((saddr & 0x0300) >> 7) & 0xFF);
			uint8_t header = slave | HEADER;

			LL_I2C_TransmitData8(i2c, header);
			timeout = STM32_I2C_TIMEOUT_USEC;
			while (!LL_I2C_IsActiveFlag_ADD10(i2c)) {
				if (stm32_i2c_wait_timeout(&timeout)) {
					LL_I2C_GenerateStopCondition(i2c);
					data->current.is_err = 1U;
					goto end;
				}
			}

			slave = saddr & 0xFF;
			LL_I2C_TransmitData8(i2c, slave);
			timeout = STM32_I2C_TIMEOUT_USEC;
			while (!LL_I2C_IsActiveFlag_ADDR(i2c)) {
				if (stm32_i2c_wait_timeout(&timeout)) {
					LL_I2C_GenerateStopCondition(i2c);
					data->current.is_err = 1U;
					goto end;
				}
			}

			LL_I2C_ClearFlag_ADDR(i2c);
			stm32_i2c_generate_start_condition(i2c);
			timeout = STM32_I2C_TIMEOUT_USEC;
			while (!LL_I2C_IsActiveFlag_SB(i2c)) {
				if (stm32_i2c_wait_timeout(&timeout)) {
					LL_I2C_GenerateStopCondition(i2c);
					data->current.is_err = 1U;
					goto end;
				}
			}

			header |= I2C_REQUEST_READ;
			LL_I2C_TransmitData8(i2c, header);
		} else {
			uint8_t slave = ((saddr) << 1) & 0xFF;

			LL_I2C_TransmitData8(i2c, slave | I2C_REQUEST_READ);
		}

		timeout = STM32_I2C_TIMEOUT_USEC;
		while (!LL_I2C_IsActiveFlag_ADDR(i2c)) {
			if (LL_I2C_IsActiveFlag_AF(i2c) || stm32_i2c_wait_timeout(&timeout)) {
				LL_I2C_ClearFlag_AF(i2c);
				LL_I2C_GenerateStopCondition(i2c);
				data->current.is_nack = 1U;
				goto end;
			}
		}
		/* ADDR must be cleared before NACK generation. Either in 2 byte reception
		 * byte 1 will be NACK'ed and slave won't sent the last byte
		 */
		LL_I2C_ClearFlag_ADDR(i2c);
		if (len == 1U) {
			/* Single byte reception: enable NACK and set STOP */
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
		} else if (len == 2U) {
			/* 2-byte reception: enable NACK and set POS */
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
			LL_I2C_EnableBitPOS(i2c);
		}
	}

	while (len) {
		timeout = STM32_I2C_TIMEOUT_USEC;
		while (!LL_I2C_IsActiveFlag_RXNE(i2c)) {
			if (stm32_i2c_wait_timeout(&timeout)) {
				LL_I2C_GenerateStopCondition(i2c);
				data->current.is_err = 1U;
				goto end;
			}
		}

		timeout = STM32_I2C_TIMEOUT_USEC;
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
				if (stm32_i2c_wait_timeout(&timeout)) {
					LL_I2C_GenerateStopCondition(i2c);
					data->current.is_err = 1U;
					goto end;
				}
			}

			/*
			 * Stop condition must be generated before reading the
			 * last two bytes.
			 */
			if (msg->flags & I2C_MSG_STOP) {
				LL_I2C_GenerateStopCondition(i2c);
			}

			for (uint32_t counter = 2; counter > 0; counter--) {
				len--;
				*buf = LL_I2C_ReceiveData8(i2c);
				buf++;
			}

			break;
		case 3:
			while (!LL_I2C_IsActiveFlag_BTF(i2c)) {
				if (stm32_i2c_wait_timeout(&timeout)) {
					LL_I2C_GenerateStopCondition(i2c);
					data->current.is_err = 1U;
					goto end;
				}
			}

			/* Set NACK before reading N-2 byte*/
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
			__fallthrough;
		default:
			len--;
			*buf = LL_I2C_ReceiveData8(i2c);
			buf++;
		}
	}
end:
	check_errors(dev, __func__);
	res = msg_end(dev, next_msg_flags, __func__);
	if (res < 0) {
		stm32_i2c_reset(dev);
	}

	return res;
}
#endif /* CONFIG_I2C_STM32_INTERRUPT */

int stm32_i2c_configure_timing(const struct device *dev, uint32_t clock)
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

int stm32_i2c_transaction(const struct device *dev,
						  struct i2c_msg msg, uint8_t *next_msg_flags,
						  uint16_t periph)
{
	int ret;

	if ((msg.flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
		ret = stm32_i2c_msg_write(dev, &msg, next_msg_flags, periph);
	} else {
		ret = stm32_i2c_msg_read(dev, &msg, next_msg_flags, periph);
	}
	return ret;
}
