/*
 * Copyright (c) 2024, Croxel Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <nrfx_twi.h>
#include "i2c_nrfx_twi_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(i2c_nrfx_twi);

int i2c_nrfx_twi_init(const struct device *dev)
{
	const struct i2c_nrfx_twi_config *config = dev->config;
	nrfx_err_t result = nrfx_twi_init(&config->twi, &config->config,
					  config->event_handler, (void *)dev);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s",
			    dev->name);
		return -EBUSY;
	}

	return 0;
}

int i2c_nrfx_twi_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_nrfx_twi_config *config = dev->config;
	struct i2c_nrfx_twi_common_data *data = dev->data;
	nrfx_twi_t const *inst = &config->twi;

	if (I2C_ADDR_10_BITS & dev_config) {
		return -EINVAL;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		nrf_twi_frequency_set(inst->p_twi, NRF_TWI_FREQ_100K);
		break;
	case I2C_SPEED_FAST:
		nrf_twi_frequency_set(inst->p_twi, NRF_TWI_FREQ_400K);
		break;
	default:
		LOG_ERR("unsupported speed");
		return -EINVAL;
	}
	data->dev_config = dev_config;

	return 0;
}

int i2c_nrfx_twi_recover_bus(const struct device *dev)
{
	const struct i2c_nrfx_twi_config *config = dev->config;
	uint32_t scl_pin;
	uint32_t sda_pin;
	nrfx_err_t err;

	scl_pin = nrf_twi_scl_pin_get(config->twi.p_twi);
	sda_pin = nrf_twi_sda_pin_get(config->twi.p_twi);

	err = nrfx_twi_bus_recover(scl_pin, sda_pin);
	return (err == NRFX_SUCCESS ? 0 : -EBUSY);
}

int i2c_nrfx_twi_msg_transfer(const struct device *dev, uint8_t flags,
			      uint8_t *buf, size_t buf_len,
			      uint16_t i2c_addr, bool more_msgs)
{
	const struct i2c_nrfx_twi_config *config = dev->config;
	int ret = 0;
	uint32_t xfer_flags = 0;
	nrfx_err_t res;
	nrfx_twi_xfer_desc_t cur_xfer = {
		.p_primary_buf = buf,
		.primary_length = buf_len,
		.address = i2c_addr,
		.type = (flags & I2C_MSG_READ) ?
			 NRFX_TWI_XFER_RX : NRFX_TWI_XFER_TX,
	};

	if (flags & I2C_MSG_ADDR_10_BITS) {
		LOG_ERR("10-bit I2C Addr devices not supported");
		ret = -ENOTSUP;
	} else if (!(flags & I2C_MSG_STOP)) {
		/* - if the transfer consists of more messages
		 *   and the I2C repeated START is not requested
		 *   to appear before the next message, suspend
		 *   the transfer after the current message,
		 *   so that it can be resumed with the next one,
		 *   resulting in the two messages merged into
		 *   a continuous transfer on the bus
		 */
		if (more_msgs) {
			xfer_flags |= NRFX_TWI_FLAG_SUSPEND;
		/* - otherwise, just finish the transfer without
		 *   generating the STOP condition, unless the current
		 *   message is an RX request, for which such feature
		 *   is not supported
		 */
		} else if (flags & I2C_MSG_READ) {
			ret = -ENOTSUP;
		} else {
			xfer_flags |= NRFX_TWI_FLAG_TX_NO_STOP;
		}
	}

	if (!ret) {
		res = nrfx_twi_xfer(&config->twi, &cur_xfer, xfer_flags);
		switch (res) {
		case NRFX_SUCCESS:
			break;
		case NRFX_ERROR_BUSY:
			ret = -EBUSY;
			break;
		default:
			ret = -EIO;
			break;
		}
	}

	return ret;
}

#ifdef CONFIG_PM_DEVICE
int twi_nrfx_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct i2c_nrfx_twi_config *config = dev->config;
	struct i2c_nrfx_twi_common_data *data = dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
		i2c_nrfx_twi_init(dev);
		if (data->dev_config) {
			i2c_nrfx_twi_configure(dev, data->dev_config);
		}
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		nrfx_twi_uninit(&config->twi);

		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
		if (ret < 0) {
			return ret;
		}
		break;

	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */
