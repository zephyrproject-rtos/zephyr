/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>
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
LOG_MODULE_REGISTER(i2c_ll_stm32_v2_rtio);

#include "i2c_ll_stm32.h"
#include "i2c-priv.h"

static void i2c_stm32_disable_transfer_interrupts(const struct device *dev)
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

static void i2c_stm32_enable_transfer_interrupts(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_EnableIT_STOP(i2c);
	LL_I2C_EnableIT_NACK(i2c);
	LL_I2C_EnableIT_TC(i2c);
	LL_I2C_EnableIT_ERR(i2c);
}

static void i2c_stm32_master_mode_end(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	i2c_stm32_disable_transfer_interrupts(dev);

	if (LL_I2C_IsEnabledReloadMode(i2c)) {
		LL_I2C_DisableReloadMode(i2c);
	}

#if defined(CONFIG_I2C_TARGET)
	struct i2c_stm32_data *data = dev->data;

	data->master_active = false;
	if (!data->slave_attached) {
		LL_I2C_Disable(i2c);
	}
#else
	LL_I2C_Disable(i2c);
#endif

}

#if defined(CONFIG_I2C_TARGET)
static void i2c_stm32_target_event(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	const struct i2c_target_callbacks *target_cb;
	struct i2c_target_config *target_cfg;

	if (data->slave_cfg->flags != I2C_TARGET_FLAGS_ADDR_10_BITS) {
		uint8_t target_address;

		/* Choose the right target from the address match code */
		target_address = LL_I2C_GetAddressMatchCode(i2c) >> 1;
		if (data->slave_cfg != NULL &&
		    target_address == data->slave_cfg->address) {
			target_cfg = data->slave_cfg;
		} else if (data->slave2_cfg != NULL &&
			   target_address == data->slave2_cfg->address) {
			target_cfg = data->slave2_cfg;
		} else {
			__ASSERT_NO_MSG(0);
			return;
		}
	} else {
		/* On STM32 the LL_I2C_GetAddressMatchCode & (ISR register) returns
		 * only 7bits of address match so 10 bit dual addressing is broken.
		 * Revert to assuming single address match.
		 */
		if (data->slave_cfg != NULL) {
			target_cfg = data->slave_cfg;
		} else {
			__ASSERT_NO_MSG(0);
			return;
		}
	}

	target_cb = target_cfg->callbacks;

	if (LL_I2C_IsActiveFlag_TXIS(i2c)) {
		uint8_t val;

		if (target_cb->read_processed(target_cfg, &val) < 0) {
			LOG_ERR("Error continuing reading");
		} else {
			LL_I2C_TransmitData8(i2c, val);
		}
		return;
	}

	if (LL_I2C_IsActiveFlag_RXNE(i2c)) {
		uint8_t val = LL_I2C_ReceiveData8(i2c);

		if (target_cb->write_received(target_cfg, val)) {
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
		}
		return;
	}

	if (LL_I2C_IsActiveFlag_NACK(i2c)) {
		LL_I2C_ClearFlag_NACK(i2c);
	}

	if (LL_I2C_IsActiveFlag_STOP(i2c)) {
		i2c_stm32_disable_transfer_interrupts(dev);

		/* Flush remaining TX byte before clearing Stop Flag */
		LL_I2C_ClearFlag_TXE(i2c);

		LL_I2C_ClearFlag_STOP(i2c);

		target_cb->stop(target_cfg);

		/* Prepare to ACK next transmissions address byte */
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
	}

	if (LL_I2C_IsActiveFlag_ADDR(i2c)) {
		uint32_t dir;

		LL_I2C_ClearFlag_ADDR(i2c);

		dir = LL_I2C_GetTransferDirection(i2c);
		if (dir == LL_I2C_DIRECTION_WRITE) {
			if (target_cb->write_requested(target_cfg) < 0) {
				LOG_ERR("Error initiating writing");
			} else {
				LL_I2C_EnableIT_RX(i2c);
			}
		} else {
			uint8_t val;

			if (target_cb->read_requested(target_cfg, &val) < 0) {
				LOG_ERR("Error initiating reading");
			} else {
				LL_I2C_TransmitData8(i2c, val);
				LL_I2C_EnableIT_TX(i2c);
			}
		}

		i2c_stm32_enable_transfer_interrupts(dev);
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

	if (config == NULL) {
		return -EINVAL;
	}

	if (data->slave_cfg && data->slave2_cfg) {
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

	/* Mark device as active */
	(void)pm_device_runtime_get(dev);

#if !defined(CONFIG_SOC_SERIES_STM32F7X)
	if (pm_device_wakeup_is_capable(dev)) {
		/* Enable wake-up from stop */
		LOG_DBG("i2c: enabling wakeup from stop");
		LL_I2C_EnableWakeUpFromStop(cfg->i2c);
	}
#endif /* !CONFIG_SOC_SERIES_STM32F7X */

	LL_I2C_Enable(i2c);

	if (!data->slave_cfg) {
		data->slave_cfg = config;
		if (data->slave_cfg->flags == I2C_TARGET_FLAGS_ADDR_10_BITS)	{
			LL_I2C_SetOwnAddress1(i2c, config->address, LL_I2C_OWNADDRESS1_10BIT);
			LOG_DBG("i2c: target #1 registered with 10-bit address");
		} else {
			LL_I2C_SetOwnAddress1(i2c, config->address << 1U, LL_I2C_OWNADDRESS1_7BIT);
			LOG_DBG("i2c: target #1 registered with 7-bit address");
		}

		LL_I2C_EnableOwnAddress1(i2c);

		LOG_DBG("i2c: target #1 registered");
	} else {
		data->slave2_cfg = config;

		if (data->slave2_cfg->flags == I2C_TARGET_FLAGS_ADDR_10_BITS)	{
			return -EINVAL;
		}
		LL_I2C_SetOwnAddress2(i2c, config->address << 1U,
				      LL_I2C_OWNADDRESS2_NOMASK);
		LL_I2C_EnableOwnAddress2(i2c);
		LOG_DBG("i2c: target #2 registered");
	}

	data->slave_attached = true;

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

	if (config == data->slave_cfg) {
		LL_I2C_DisableOwnAddress1(i2c);
		data->slave_cfg = NULL;

		LOG_DBG("i2c: target #1 unregistered");
	} else if (config == data->slave2_cfg) {
		LL_I2C_DisableOwnAddress2(i2c);
		data->slave2_cfg = NULL;

		LOG_DBG("i2c: target #2 unregistered");
	} else {
		return -EINVAL;
	}

	/* Return if there is a target remaining */
	if (data->slave_cfg || data->slave2_cfg) {
		LOG_DBG("i2c: target#%c still registered", data->slave_cfg?'1':'2');
		return 0;
	}

	/* Otherwise disable I2C */
	LL_I2C_DisableIT_ADDR(i2c);
	i2c_stm32_disable_transfer_interrupts(dev);

	LL_I2C_ClearFlag_NACK(i2c);
	LL_I2C_ClearFlag_STOP(i2c);
	LL_I2C_ClearFlag_ADDR(i2c);

	LL_I2C_Disable(i2c);

#if !defined(CONFIG_SOC_SERIES_STM32F7X)
	if (pm_device_wakeup_is_capable(dev)) {
		/* Disable wake-up from STOP */
		LOG_DBG("i2c: disabling wakeup from stop");
		LL_I2C_DisableWakeUpFromStop(i2c);
	}
#endif /* !CONFIG_SOC_SERIES_STM32F7X */

	/* Release the device */
	(void)pm_device_runtime_put(dev);

	data->slave_attached = false;

	return 0;
}

#endif /* defined(CONFIG_I2C_TARGET) */

static void i2c_stm32_reload_burst(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	__ASSERT_NO_MSG(LL_I2C_IsEnabledReloadMode(i2c));

	if (data->xfer_len > UINT8_MAX) {
		/* Not the last burst for this message, don't consider STOP and RESTART flags */
		data->burst_len = UINT8_MAX;
		data->burst_flags = data->xfer_flags & ~(I2C_MSG_STOP | I2C_MSG_RESTART);
	} else {
		/* Preserve possible STOP flag */
		data->burst_len = data->xfer_len;
		data->burst_flags = data->xfer_flags;
	}

	LL_I2C_SetTransferSize(i2c, data->burst_len);

	/* If last burst in reload mode, disable the mode now that transfer size is loaded */
	if ((data->burst_len == data->xfer_len) &&
	    (data->burst_flags & I2C_MSG_STM32_USE_RELOAD_MODE) == 0) {
		LL_I2C_DisableReloadMode(i2c);
	}
}

void i2c_stm32_event(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	struct i2c_rtio *ctx = data->ctx;
	I2C_TypeDef *i2c = cfg->i2c;
	int ret = 0;

#if defined(CONFIG_I2C_TARGET)
	if (data->slave_attached && !data->master_active) {
		i2c_stm32_target_event(dev);
		return;
	}
#endif

	if (data->burst_len != 0U) {
		/* Send next byte */
		if (LL_I2C_IsActiveFlag_TXIS(i2c)) {
			LL_I2C_TransmitData8(i2c, *data->xfer_buf);
		}

		/* Receive next byte */
		if (LL_I2C_IsActiveFlag_RXNE(i2c)) {
			*data->xfer_buf = LL_I2C_ReceiveData8(i2c);
		}

		data->xfer_buf++;
		data->xfer_len--;
		data->burst_len--;
	}

	/* NACK received */
	if (LL_I2C_IsActiveFlag_NACK(i2c)) {
		LL_I2C_ClearFlag_NACK(i2c);
		/*
		 * AutoEndMode is always disabled in master mode,
		 * so send a stop condition manually
		 */
		LL_I2C_GenerateStopCondition(i2c);
		ret = -EIO;
	}

	/* STOP received */
	if (LL_I2C_IsActiveFlag_STOP(i2c)) {
		LL_I2C_ClearFlag_STOP(i2c);
		LL_I2C_DisableReloadMode(i2c);
		i2c_stm32_master_mode_end(dev);

		if (i2c_rtio_complete(ctx, ret)) {
			i2c_stm32_start(dev);
			return;
		}
	}

	if (LL_I2C_IsActiveFlag_TC(i2c) ||
	    LL_I2C_IsActiveFlag_TCR(i2c)) {
		__ASSERT_NO_MSG(data->burst_len == 0U);

		if (data->xfer_len != 0U) {
			i2c_stm32_reload_burst(dev);
			return;
		}

		/* Issue stop condition if necessary */
		if ((data->burst_flags & I2C_MSG_STOP) != 0) {
			LL_I2C_GenerateStopCondition(i2c);
		} else {
			i2c_stm32_disable_transfer_interrupts(dev);
			if (i2c_rtio_complete(ctx, ret)) {
				i2c_stm32_start(dev);
			}
		}
	}
}

int i2c_stm32_error(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	struct i2c_rtio *ctx = data->ctx;
	I2C_TypeDef *i2c = cfg->i2c;
	int ret = 0;

#if defined(CONFIG_I2C_TARGET)
	if (data->slave_attached && !data->master_active) {
		/* No need for a target error function right now. */
		return 0;
	}
#endif

	if (LL_I2C_IsActiveFlag_ARLO(i2c)) {
		LL_I2C_ClearFlag_ARLO(i2c);
		ret = -EIO;
	}

	if (ret) {
		i2c_stm32_master_mode_end(dev);
		if (i2c_rtio_complete(ctx, ret)) {
			i2c_stm32_start(dev);
		}
	}

	return ret;
}

int i2c_stm32_msg_start(const struct device *dev, uint8_t flags,
	uint8_t *buf, size_t buf_len, uint16_t i2c_addr)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t transfer;

	data->xfer_buf = buf;
	data->xfer_len = buf_len;
	data->xfer_flags = flags;

	if (LL_I2C_IsEnabledReloadMode(i2c)) {
		__ASSERT_NO_MSG((flags & I2C_MSG_RESTART) == 0U);
		i2c_stm32_reload_burst(dev);
		goto out;
	}

	if ((flags & I2C_MSG_READ) != 0) {
		transfer = LL_I2C_REQUEST_READ;
	} else {
		transfer = LL_I2C_REQUEST_WRITE;
	}

	if ((I2C_MSG_ADDR_10_BITS & flags) != 0) {
		LL_I2C_SetMasterAddressingMode(i2c,
				LL_I2C_ADDRESSING_MODE_10BIT);
		LL_I2C_SetSlaveAddr(i2c, (uint32_t) i2c_addr);
	} else {
		LL_I2C_SetMasterAddressingMode(i2c,
			LL_I2C_ADDRESSING_MODE_7BIT);
		LL_I2C_SetSlaveAddr(i2c, (uint32_t) i2c_addr << 1);
	}

	if (buf_len > UINT8_MAX) {
		data->burst_flags = flags & ~I2C_MSG_STOP;
		data->burst_len = UINT8_MAX;
		LL_I2C_EnableReloadMode(i2c);
	} else {
		data->burst_flags = flags;
		data->burst_len = buf_len;
		if ((flags & I2C_MSG_STM32_USE_RELOAD_MODE) != 0) {
			LL_I2C_EnableReloadMode(i2c);
		} else {
			LL_I2C_DisableReloadMode(i2c);
		}
	}

	LL_I2C_DisableAutoEndMode(i2c);
	LL_I2C_SetTransferRequest(i2c, transfer);
	LL_I2C_SetTransferSize(i2c, data->burst_len);

#if defined(CONFIG_I2C_TARGET)
	data->master_active = true;
#endif

	LL_I2C_Enable(i2c);

	LL_I2C_GenerateStartCondition(i2c);

out:
	i2c_stm32_enable_transfer_interrupts(dev);
	if ((flags & I2C_MSG_READ) != 0) {
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
		LOG_ERR("i2c: speed above \"fast\" requires manual timing configuration, "
				"see \"timings\" property of st,stm32-i2c-v2 devicetree binding");
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
