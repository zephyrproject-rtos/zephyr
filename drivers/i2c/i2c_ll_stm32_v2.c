/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C Driver for: STM32F0, STM32F3, STM32F7, STM32L0, STM32L4, STM32WB and
 * STM32WL
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
LOG_MODULE_REGISTER(i2c_ll_stm32_v2);

#include "i2c-priv.h"

#define STM32_I2C_TRANSFER_TIMEOUT_MSEC  500

static inline void msg_init(const struct device *dev, struct i2c_msg *msg,
			    uint8_t *next_msg_flags, uint16_t slave,
			    uint32_t transfer)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
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

#if defined(CONFIG_I2C_TARGET)
		data->master_active = true;
#endif
		LL_I2C_Enable(i2c);

		LL_I2C_GenerateStartCondition(i2c);
	}
}

#ifdef CONFIG_I2C_STM32_INTERRUPT

static void stm32_i2c_disable_transfer_interrupts(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_DisableIT_TX(i2c);
	LL_I2C_DisableIT_RX(i2c);
	LL_I2C_DisableIT_STOP(i2c);
	LL_I2C_DisableIT_NACK(i2c);
	LL_I2C_DisableIT_TC(i2c);
	LL_I2C_DisableIT_ERR(i2c);
}

static void stm32_i2c_enable_transfer_interrupts(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_EnableIT_STOP(i2c);
	LL_I2C_EnableIT_NACK(i2c);
	LL_I2C_EnableIT_TC(i2c);
	LL_I2C_EnableIT_ERR(i2c);
}

static void stm32_i2c_master_mode_end(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	stm32_i2c_disable_transfer_interrupts(dev);

#if defined(CONFIG_I2C_TARGET)
	data->master_active = false;
	if (!data->slave_attached) {
		LL_I2C_Disable(i2c);
	}
#else
	LL_I2C_Disable(i2c);
#endif
	k_sem_give(&data->device_sync_sem);
}

#if defined(CONFIG_I2C_TARGET)
static void stm32_i2c_slave_event(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	const struct i2c_target_callbacks *slave_cb =
		data->slave_cfg->callbacks;

	if (LL_I2C_IsActiveFlag_TXIS(i2c)) {
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

	if (LL_I2C_IsActiveFlag_NACK(i2c)) {
		LL_I2C_ClearFlag_NACK(i2c);
	}

	if (LL_I2C_IsActiveFlag_STOP(i2c)) {
		stm32_i2c_disable_transfer_interrupts(dev);

		/* Flush remaining TX byte before clearing Stop Flag */
		LL_I2C_ClearFlag_TXE(i2c);

		LL_I2C_ClearFlag_STOP(i2c);

		slave_cb->stop(data->slave_cfg);

		/* Prepare to ACK next transmissions address byte */
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
	}

	if (LL_I2C_IsActiveFlag_ADDR(i2c)) {
		uint32_t dir;

		LL_I2C_ClearFlag_ADDR(i2c);

		dir = LL_I2C_GetTransferDirection(i2c);
		if (dir == LL_I2C_DIRECTION_WRITE) {
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

/* Attach and start I2C as target */
int i2c_stm32_target_register(const struct device *dev,
			     struct i2c_target_config *config)
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

	LL_I2C_SetOwnAddress1(i2c, config->address << 1,
			      LL_I2C_OWNADDRESS1_7BIT);
	LL_I2C_EnableOwnAddress1(i2c);

	data->slave_attached = true;

	LOG_DBG("i2c: slave registered");

	LL_I2C_EnableIT_ADDR(i2c);

	return 0;
}

int i2c_stm32_target_unregister(const struct device *dev,
			       struct i2c_target_config *config)
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

	LL_I2C_DisableOwnAddress1(i2c);

	LL_I2C_DisableIT_ADDR(i2c);
	stm32_i2c_disable_transfer_interrupts(dev);

	LL_I2C_ClearFlag_NACK(i2c);
	LL_I2C_ClearFlag_STOP(i2c);
	LL_I2C_ClearFlag_ADDR(i2c);

	LL_I2C_Disable(i2c);

	data->slave_attached = false;

	LOG_DBG("i2c: slave unregistered");

	return 0;
}

#endif /* defined(CONFIG_I2C_TARGET) */

static void stm32_i2c_event(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

#if defined(CONFIG_I2C_TARGET)
	if (data->slave_attached && !data->master_active) {
		stm32_i2c_slave_event(dev);
		return;
	}
#endif
	if (data->current.len) {
		/* Send next byte */
		if (LL_I2C_IsActiveFlag_TXIS(i2c)) {
			LL_I2C_TransmitData8(i2c, *data->current.buf);
		}

		/* Receive next byte */
		if (LL_I2C_IsActiveFlag_RXNE(i2c)) {
			*data->current.buf = LL_I2C_ReceiveData8(i2c);
		}

		data->current.buf++;
		data->current.len--;
	}

	/* NACK received */
	if (LL_I2C_IsActiveFlag_NACK(i2c)) {
		LL_I2C_ClearFlag_NACK(i2c);
		data->current.is_nack = 1U;
		/*
		 * AutoEndMode is always disabled in master mode,
		 * so send a stop condition manually
		 */
		LL_I2C_GenerateStopCondition(i2c);
		return;
	}

	/* STOP received */
	if (LL_I2C_IsActiveFlag_STOP(i2c)) {
		LL_I2C_ClearFlag_STOP(i2c);
		LL_I2C_DisableReloadMode(i2c);
		goto end;
	}

	/* Transfer Complete or Transfer Complete Reload */
	if (LL_I2C_IsActiveFlag_TC(i2c) ||
	    LL_I2C_IsActiveFlag_TCR(i2c)) {
		/* Issue stop condition if necessary */
		if (data->current.msg->flags & I2C_MSG_STOP) {
			LL_I2C_GenerateStopCondition(i2c);
		} else {
			stm32_i2c_disable_transfer_interrupts(dev);
			k_sem_give(&data->device_sync_sem);
		}
	}

	return;
end:
	stm32_i2c_master_mode_end(dev);
}

static int stm32_i2c_error(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

#if defined(CONFIG_I2C_TARGET)
	if (data->slave_attached && !data->master_active) {
		/* No need for a slave error function right now. */
		return 0;
	}
#endif

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

	return 0;
end:
	stm32_i2c_master_mode_end(dev);
	return -EIO;
}

#ifdef CONFIG_I2C_STM32_COMBINED_INTERRUPT
void stm32_i2c_combined_isr(void *arg)
{
	const struct device *dev = (const struct device *) arg;

	if (stm32_i2c_error(dev)) {
		return;
	}
	stm32_i2c_event(dev);
}
#else

void stm32_i2c_event_isr(void *arg)
{
	const struct device *dev = (const struct device *) arg;

	stm32_i2c_event(dev);
}

void stm32_i2c_error_isr(void *arg)
{
	const struct device *dev = (const struct device *) arg;

	stm32_i2c_error(dev);
}
#endif

int stm32_i2c_msg_write(const struct device *dev, struct i2c_msg *msg,
			uint8_t *next_msg_flags, uint16_t slave)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	bool is_timeout = false;

	data->current.len = msg->len;
	data->current.buf = msg->buf;
	data->current.is_write = 1U;
	data->current.is_nack = 0U;
	data->current.is_err = 0U;
	data->current.msg = msg;

	msg_init(dev, msg, next_msg_flags, slave, LL_I2C_REQUEST_WRITE);

	stm32_i2c_enable_transfer_interrupts(dev);
	LL_I2C_EnableIT_TX(i2c);

	if (k_sem_take(&data->device_sync_sem,
		       K_MSEC(STM32_I2C_TRANSFER_TIMEOUT_MSEC)) != 0) {
		stm32_i2c_master_mode_end(dev);
		k_sem_take(&data->device_sync_sem, K_FOREVER);
		is_timeout = true;
	}

	if (data->current.is_nack || data->current.is_err ||
	    data->current.is_arlo || is_timeout) {
		goto error;
	}

	return 0;
error:
	if (data->current.is_arlo) {
		LOG_DBG("%s: ARLO %d", __func__,
				    data->current.is_arlo);
		data->current.is_arlo = 0U;
	}

	if (data->current.is_nack) {
		LOG_DBG("%s: NACK", __func__);
		data->current.is_nack = 0U;
	}

	if (data->current.is_err) {
		LOG_DBG("%s: ERR %d", __func__,
				    data->current.is_err);
		data->current.is_err = 0U;
	}

	if (is_timeout) {
		LOG_DBG("%s: TIMEOUT", __func__);
	}

	return -EIO;
}

int stm32_i2c_msg_read(const struct device *dev, struct i2c_msg *msg,
		       uint8_t *next_msg_flags, uint16_t slave)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	bool is_timeout = false;

	data->current.len = msg->len;
	data->current.buf = msg->buf;
	data->current.is_write = 0U;
	data->current.is_arlo = 0U;
	data->current.is_err = 0U;
	data->current.is_nack = 0U;
	data->current.msg = msg;

	msg_init(dev, msg, next_msg_flags, slave, LL_I2C_REQUEST_READ);

	stm32_i2c_enable_transfer_interrupts(dev);
	LL_I2C_EnableIT_RX(i2c);

	if (k_sem_take(&data->device_sync_sem,
		       K_MSEC(STM32_I2C_TRANSFER_TIMEOUT_MSEC)) != 0) {
		stm32_i2c_master_mode_end(dev);
		k_sem_take(&data->device_sync_sem, K_FOREVER);
		is_timeout = true;
	}

	if (data->current.is_nack || data->current.is_err ||
	    data->current.is_arlo || is_timeout) {
		goto error;
	}

	return 0;
error:
	if (data->current.is_arlo) {
		LOG_DBG("%s: ARLO %d", __func__,
				    data->current.is_arlo);
		data->current.is_arlo = 0U;
	}

	if (data->current.is_nack) {
		LOG_DBG("%s: NACK", __func__);
		data->current.is_nack = 0U;
	}

	if (data->current.is_err) {
		LOG_DBG("%s: ERR %d", __func__,
				    data->current.is_err);
		data->current.is_err = 0U;
	}

	if (is_timeout) {
		LOG_DBG("%s: TIMEOUT", __func__);
	}

	return -EIO;
}

#else /* !CONFIG_I2C_STM32_INTERRUPT */
static inline int check_errors(const struct device *dev, const char *funcname)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	if (LL_I2C_IsActiveFlag_NACK(i2c)) {
		LL_I2C_ClearFlag_NACK(i2c);
		LOG_DBG("%s: NACK", funcname);
		goto error;
	}

	if (LL_I2C_IsActiveFlag_ARLO(i2c)) {
		LL_I2C_ClearFlag_ARLO(i2c);
		LOG_DBG("%s: ARLO", funcname);
		goto error;
	}

	if (LL_I2C_IsActiveFlag_OVR(i2c)) {
		LL_I2C_ClearFlag_OVR(i2c);
		LOG_DBG("%s: OVR", funcname);
		goto error;
	}

	if (LL_I2C_IsActiveFlag_BERR(i2c)) {
		LL_I2C_ClearFlag_BERR(i2c);
		LOG_DBG("%s: BERR", funcname);
		goto error;
	}

	return 0;
error:
	if (LL_I2C_IsEnabledReloadMode(i2c)) {
		LL_I2C_DisableReloadMode(i2c);
	}
	return -EIO;
}

static inline int msg_done(const struct device *dev,
			   unsigned int current_msg_flags)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	/* Wait for transfer to complete */
	while (!LL_I2C_IsActiveFlag_TC(i2c) && !LL_I2C_IsActiveFlag_TCR(i2c)) {
		if (check_errors(dev, __func__)) {
			return -EIO;
		}
	}
	/* Issue stop condition if necessary */
	if (current_msg_flags & I2C_MSG_STOP) {
		LL_I2C_GenerateStopCondition(i2c);
		while (!LL_I2C_IsActiveFlag_STOP(i2c)) {
		}

		LL_I2C_ClearFlag_STOP(i2c);
		LL_I2C_DisableReloadMode(i2c);
	}

	return 0;
}

int stm32_i2c_msg_write(const struct device *dev, struct i2c_msg *msg,
			uint8_t *next_msg_flags, uint16_t slave)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;
	unsigned int len = 0U;
	uint8_t *buf = msg->buf;

	msg_init(dev, msg, next_msg_flags, slave, LL_I2C_REQUEST_WRITE);

	len = msg->len;
	while (len) {
		while (1) {
			if (LL_I2C_IsActiveFlag_TXIS(i2c)) {
				break;
			}

			if (check_errors(dev, __func__)) {
				return -EIO;
			}
		}

		LL_I2C_TransmitData8(i2c, *buf);
		buf++;
		len--;
	}

	return msg_done(dev, msg->flags);
}

int stm32_i2c_msg_read(const struct device *dev, struct i2c_msg *msg,
		       uint8_t *next_msg_flags, uint16_t slave)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;
	unsigned int len = 0U;
	uint8_t *buf = msg->buf;

	msg_init(dev, msg, next_msg_flags, slave, LL_I2C_REQUEST_READ);

	len = msg->len;
	while (len) {
		while (!LL_I2C_IsActiveFlag_RXNE(i2c)) {
			if (check_errors(dev, __func__)) {
				return -EIO;
			}
		}

		*buf = LL_I2C_ReceiveData8(i2c);
		buf++;
		len--;
	}

	return msg_done(dev, msg->flags);
}
#endif

int stm32_i2c_configure_timing(const struct device *dev, uint32_t clock)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t i2c_hold_time_min, i2c_setup_time_min;
	uint32_t i2c_h_min_time, i2c_l_min_time;
	uint32_t presc = 1U;
	uint32_t timing = 0U;

	/*  Look for an adequate preset timing value */
	for (uint32_t i = 0; i < cfg->n_timings; i++) {
		const struct i2c_config_timing *preset = &cfg->timings[i];
		uint32_t speed = i2c_map_dt_bitrate(preset->i2c_speed);

		if ((I2C_SPEED_GET(speed) == I2C_SPEED_GET(data->dev_config))
		   && (preset->periph_clock == clock)) {
			/*  Found a matching periph clock and i2c speed */
			LL_I2C_SetTiming(i2c, preset->timing_setting);
			return 0;
		}
	}

	/* No preset timing was provided, let's dynamically configure */
	switch (I2C_SPEED_GET(data->dev_config)) {
	case I2C_SPEED_STANDARD:
		i2c_h_min_time = 4000U;
		i2c_l_min_time = 4700U;
		i2c_hold_time_min = 500U;
		i2c_setup_time_min = 1250U;
		break;
	case I2C_SPEED_FAST:
		i2c_h_min_time = 600U;
		i2c_l_min_time = 1300U;
		i2c_hold_time_min = 375U;
		i2c_setup_time_min = 500U;
		break;
	default:
		return -EINVAL;
	}

	/* Calculate period until prescaler matches */
	do {
		uint32_t t_presc = clock / presc;
		uint32_t ns_presc = NSEC_PER_SEC / t_presc;
		uint32_t sclh = i2c_h_min_time / ns_presc;
		uint32_t scll = i2c_l_min_time / ns_presc;
		uint32_t sdadel = i2c_hold_time_min / ns_presc;
		uint32_t scldel = i2c_setup_time_min / ns_presc;

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

	if (presc >= 16U) {
		LOG_DBG("I2C:failed to find prescaler value");
		return -EINVAL;
	}

	LL_I2C_SetTiming(i2c, timing);

	return 0;
}
