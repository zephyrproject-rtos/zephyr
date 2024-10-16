/********************************************************************
 *   @file   LTC268X.c
 *   @brief  Implementation of LTC2686/8 Driver.
 *   @author Mircea Caprioru (mircea.caprioru@analog.com)
 *   @author Meta Platforms, Inc. (adaptations/modifications for Zephyr OS)
 ********************************************************************
 * Copyright 2021(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************/

/*******************************************************************/
/***************************** Include Files ***********************/
/*******************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include "ltc268x.h"

#if DT_HAS_COMPAT_STATUS_OKAY(lltc_ltc2688) || DT_HAS_COMPAT_STATUS_OKAY(lltc_ltc2686)

/* LTC268X definitions. */
LOG_MODULE_REGISTER(dac_ltc268x, LOG_LEVEL_INF);

static const struct ltc268x_span_tbl ltc268x_span_tbl[] = {
	[LTC268X_VOLTAGE_RANGE_0V_5V] = {0, 5},
	[LTC268X_VOLTAGE_RANGE_0V_10V] = {0, 10},
	[LTC268X_VOLTAGE_RANGE_M5V_5V] = {-5, 5},
	[LTC268X_VOLTAGE_RANGE_M10V_10V] = {-10, 10},
	[LTC268X_VOLTAGE_RANGE_M15V_15V] = {-15, 15}};

/* LTC268x runtime data defaults  */
static const struct ltc268x_data data_defaults = {
	.pwd_dac_setting = 0,
	.dither_toggle_en = 0,
	.dither_mode = {[0 ... 15] = false},
	.dac_code = {[0 ... 15] = 0},
	.crt_range = {[0 ... 15] = LTC268X_VOLTAGE_RANGE_0V_5V},
	.dither_phase = {[0 ... 15] = LTC268X_DITH_PHASE_0},
	.dither_period = {[0 ... 15] = LTC268X_DITH_PERIOD_4},
	.clk_input = {[0 ... 15] = LTC268X_SOFT_TGL},
	.reg_select = {[0 ... 15] = LTC268X_SELECT_A_REG},
};

/*******************************************************************/
/************************ Functions Definitions ********************/
/*******************************************************************/
/**
 * SPI command write to device.
 * @param dev - The device structure.
 * @param cmd - The command.
 * @param data - The data.
 * @return 0 in case of success, negative error code otherwise.
 */
static int32_t _ltc268x_spi_write(const struct device *dev, uint8_t cmd, uint16_t data)
{
	if (dev == NULL) {
		return -ENODEV;
	}

	const struct ltc268x_config *config = dev->config;
	uint8_t buffer_tx[3];
	uint8_t buffer_rx[ARRAY_SIZE(buffer_tx)];
	int32_t ret;

	buffer_tx[0] = cmd;
	buffer_tx[1] = (data & 0xFF00) >> 8;
	buffer_tx[2] = data & 0x00FF;

	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	const struct spi_buf rx_buf[] = {{
		.buf = buffer_rx,
		.len = ARRAY_SIZE(buffer_rx),
	}};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	ret = spi_transceive_dt(&config->bus, &tx, &rx);

	if (ret != 0) {
		LOG_ERR("%s: spi_transceive failed with error %i", __func__, ret);
		return ret;
	}
	return ret;
}

/**
 * SPI read from device.
 * @param dev - The device structure.
 * @param reg - The register address.
 * @param data - The register data.
 * @return 0 in case of success, negative error code otherwise.
 */
static int32_t _ltc268x_spi_read(const struct device *dev, uint8_t reg, uint16_t *data)
{
	if (dev == NULL) {
		return -ENODEV;
	}

	_ltc268x_spi_write(dev, reg | LTC268X_READ_OPERATION, 0x0000);

	const struct ltc268x_config *config = dev->config;
	uint8_t buffer_tx[3] = {0, 0, 0};
	uint8_t buffer_rx[ARRAY_SIZE(buffer_tx)];
	int32_t ret;

	buffer_tx[0] = LTC268X_CMD_NOOP;

	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	const struct spi_buf rx_buf[] = {{
		.buf = buffer_rx,
		.len = ARRAY_SIZE(buffer_rx),
	}};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	ret = spi_transceive_dt(&config->bus, &tx, &rx);

	*data = (buffer_rx[1] << 8) | buffer_rx[2];

	LOG_DBG("%s read from register 0x%02X value 0x%02X", __func__, reg, *data);

	return ret;
}

/**
 * SPI readback register from device.
 * @param dev - The device structure.
 * @param reg - The register address.
 * @param mask - The register mask.
 * @param val - The updated value.
 * @return 0 in case of success, negative error code otherwise.
 */
static int32_t _ltc268x_spi_update_bits(const struct device *dev, uint8_t reg, uint16_t mask,
					uint16_t val)
{
	uint16_t regval;
	int32_t ret;

	ret = _ltc268x_spi_read(dev, reg, &regval);
	if (ret < 0)
		return ret;

	regval &= ~mask;
	regval |= val;

	return _ltc268x_spi_write(dev, reg, regval);
}

/**
 * Power down the selected channels.
 * @param dev - The device structure.
 * @param setting - The setting.
 *		    Accepted values: LTC268X_PWDN(x) | LTC268X_PWDN(y) | ...
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ltc268x_set_pwr_dac(const struct device *dev, uint16_t setting)
{
	struct ltc268x_data *data = dev->data;
	int32_t ret;

	ret = _ltc268x_spi_write(dev, LTC268X_CMD_POWERDOWN_REG, setting);
	if (ret < 0)
		return ret;

	data->pwd_dac_setting = setting;

	return 0;
}

/**
 * Enable dither/toggle for selected channels.
 * @param dev - The device structure.
 * @param setting - The setting.
 *		    Accepted values: LTC268X_DITH_EN(x) | LTC268X_DITH_EN(y) | ...
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ltc268x_set_dither_toggle(const struct device *dev, uint16_t setting)
{
	struct ltc268x_data *data = dev->data;
	int32_t ret;

	ret = _ltc268x_spi_write(dev, LTC268X_CMD_TOGGLE_DITHER_EN_REG, setting);
	if (ret < 0)
		return ret;

	data->dither_toggle_en = setting;

	return 0;
}

/**
 * Set channel to dither mode.
 * @param dev - The device structure.
 * @param channel - The channel for which to change the mode.
 * @param en - enable or disable dither mode
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ltc268x_set_dither_mode(const struct device *dev, uint8_t channel, bool en)
{
	const struct ltc268x_config *config = dev->config;
	struct ltc268x_data *data = dev->data;
	uint16_t val = 0;
	int32_t ret;

	if (channel >= config->nchannels)
		return -ENOENT;

	if (en)
		val = LTC268X_CH_MODE;

	ret = _ltc268x_spi_update_bits(dev, LTC268X_CMD_CH_SETTING(channel, config->dev_id),
				       LTC268X_CH_MODE, val);
	if (ret < 0)
		return ret;

	data->dither_mode[channel] = en;

	return 0;
}

/**
 * Set channel span.
 * @param dev - The device structure.
 * @param channel - The channel for which to change the mode.
 * @param range - Voltage range.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ltc268x_set_span(const struct device *dev, uint8_t channel,
			 enum ltc268x_voltage_range range)
{
	const struct ltc268x_config *config = dev->config;
	struct ltc268x_data *data = dev->data;
	int32_t ret;

	if (channel >= config->nchannels)
		return -ENOENT;

	ret = _ltc268x_spi_update_bits(dev, LTC268X_CMD_CH_SETTING(channel, config->dev_id),
				       LTC268X_CH_SPAN_MSK, LTC268X_CH_SPAN(range));
	if (ret < 0)
		return ret;

	data->crt_range[channel] = range;

	return 0;
}

/**
 * Set channel dither phase.
 * @param dev - The device structure.
 * @param channel - The channel for which to change the mode.
 * @param phase - Dither phase.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ltc268x_set_dither_phase(const struct device *dev, uint8_t channel,
				 enum ltc268x_dither_phase phase)
{
	const struct ltc268x_config *config = dev->config;
	struct ltc268x_data *data = dev->data;
	int32_t ret;

	if (channel >= config->nchannels)
		return -ENOENT;

	ret = _ltc268x_spi_update_bits(dev, LTC268X_CMD_CH_SETTING(channel, config->dev_id),
				       LTC268X_CH_DIT_PH_MSK, LTC268X_CH_DIT_PH(phase));
	if (ret < 0)
		return ret;

	data->dither_phase[channel] = phase;

	return 0;
}

/**
 * Set channel dither period.
 * @param dev - The device structure.
 * @param channel - The channel for which to change the mode.
 * @param period - Dither period.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ltc268x_set_dither_period(const struct device *dev, uint8_t channel,
				  enum ltc268x_dither_period period)
{
	const struct ltc268x_config *config = dev->config;
	struct ltc268x_data *data = dev->data;
	int32_t ret;

	if (channel >= config->nchannels)
		return -ENOENT;

	ret = _ltc268x_spi_update_bits(dev, LTC268X_CMD_CH_SETTING(channel, config->dev_id),
				       LTC268X_CH_DIT_PER_MSK, LTC268X_CH_DIT_PER(period));
	if (ret < 0)
		return ret;

	data->dither_period[channel] = period;

	return 0;
}

/**
 * Select register A or B for value.
 * @param dev - The device structure.
 * @param channel - The channel for which to change the mode.
 * @param sel_reg - Select register A or B to store DAC output value.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ltc268x_select_reg(const struct device *dev, uint8_t channel,
			   enum ltc268x_a_b_register sel_reg)
{
	const struct ltc268x_config *config = dev->config;
	struct ltc268x_data *data = dev->data;
	int32_t ret;

	if (channel >= config->nchannels)
		return -ENOENT;

	ret = _ltc268x_spi_update_bits(dev, LTC268X_CMD_A_B_SELECT_REG, BIT(channel),
				       sel_reg << channel);
	if (ret < 0)
		return ret;

	data->reg_select[channel] = sel_reg;

	return 0;
}

/**
 * Select dither/toggle clock input.
 * @param dev - The device structure.
 * @param channel - The channel for which to change the mode.
 * @param clk_input - Select the source for the clock input.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ltc268x_select_tg_dith_clk(const struct device *dev, uint8_t channel,
				   enum ltc268x_clk_input clk_input)
{
	const struct ltc268x_config *config = dev->config;
	struct ltc268x_data *data = dev->data;
	int32_t ret;

	if (channel >= config->nchannels)
		return -ENOENT;

	ret = _ltc268x_spi_update_bits(dev, LTC268X_CMD_CH_SETTING(channel, config->dev_id),
				       LTC268X_CH_TD_SEL_MSK, LTC268X_CH_TD_SEL(clk_input));
	if (ret < 0)
		return ret;

	data->clk_input[channel] = clk_input;

	return 0;
}

/**
 * Toggle the software source for dither/toggle.
 * @param dev - The device structure.
 * @param channel - The channel for which to change the mode.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ltc268x_software_toggle(const struct device *dev, uint8_t channel)
{
	int32_t ret;
	uint16_t regval;
	const struct ltc268x_config *config = dev->config;

	if (channel >= config->nchannels)
		return -ENOENT;

	ret = _ltc268x_spi_read(dev, LTC268X_CMD_SW_TOGGLE_REG, &regval);
	if (ret < 0)
		return ret;

	regval ^= BIT(channel);

	return _ltc268x_spi_write(dev, LTC268X_CMD_SW_TOGGLE_REG, regval);
}

/**
 * Software reset the device.
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ltc268x_software_reset(const struct device *dev)
{
	return _ltc268x_spi_update_bits(dev, LTC268X_CMD_CONFIG_REG, LTC268X_CONFIG_RST,
					LTC268X_CONFIG_RST);
}

/**
 *  Sets the output voltage of a channel.
 *
 * @param dev     - The device structure.
 * @param channel - Channel option.
 *
 * @param voltage - Value to be outputted by the DAC(Volts).
 *
 * @return The actual voltage value that can be outputted by the channel.
 */
int32_t ltc268x_set_voltage(const struct device *dev, uint8_t channel, float voltage)
{
	const struct ltc268x_config *config = dev->config;
	struct ltc268x_data *data = dev->data;
	uint16_t offset, gain, code;
	int32_t range_offset, v_ref, ret;

	/* Get the offset, gain and range of the selected channel. */
	ret = _ltc268x_spi_read(dev, LTC268X_CMD_CH_OFFSET(channel, config->dev_id), &offset);
	if (ret < 0)
		return ret;

	ret = _ltc268x_spi_read(dev, LTC268X_CMD_CH_GAIN(channel, config->dev_id), &gain);
	if (ret < 0)
		return ret;

	range_offset = ltc268x_span_tbl[data->crt_range[channel]].min;
	v_ref = ltc268x_span_tbl[data->crt_range[channel]].max -
		ltc268x_span_tbl[data->crt_range[channel]].min;

	/* Compute the binary code from the value(mA) provided by user. */
	code = (uint32_t)((voltage - range_offset) * (1l << 16) / v_ref);
	if (code > 0xFFFF)
		code = 0xFFFF;

	data->dac_code[channel] = code;

	/* Write to the Data Register of the DAC. */
	return _ltc268x_spi_write(dev, LTC268X_CMD_CH_CODE_UPDATE(channel, config->dev_id), code);
}

/**
 * Initialize the device.
 * @param device - The device structure.
 * @param init_param - The structure that contains the device initial
 *		       parameters.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ltc268x_init(const struct device *dev)
{
	LOG_INF("Starting initialization of LTC268X device");
	if (dev == NULL) {
		return -ENODEV;
	}

	const struct ltc268x_config *config = dev->config;
	uint8_t channel = 0;
	int ret;

	/* SPI */
	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		ret = -ENODEV;
		goto error;
	}

	/* Device Settings */
	ret = ltc268x_software_reset(dev);
	if (ret < 0)
		goto error;

	// wait 100 msecs for reset
	k_busy_wait(100 * 1000);

	/* Powerdown/up channels */
	ret = ltc268x_set_pwr_dac(dev, data_defaults.pwd_dac_setting);
	if (ret < 0)
		goto error;

	/* Enable dither/toggle */
	ret = ltc268x_set_dither_toggle(dev, data_defaults.dither_toggle_en);
	if (ret < 0)
		goto error;

	for (channel = 0; channel < config->nchannels; channel++) {
		/* Setup channel span */
		ret = ltc268x_set_span(dev, channel, data_defaults.crt_range[channel]);
		if (ret < 0)
			goto error;

		/* Set dither phase */
		ret = ltc268x_set_dither_phase(dev, channel, data_defaults.dither_phase[channel]);
		if (ret < 0)
			goto error;

		/* Set dither period */
		ret = ltc268x_set_dither_period(dev, channel, data_defaults.dither_period[channel]);
		if (ret < 0)
			goto error;

		ret = ltc268x_set_dither_mode(dev, channel, data_defaults.dither_mode[channel]);
		if (ret < 0)
			goto error;

		/* Set toggle/dither clock */
		ret = ltc268x_select_tg_dith_clk(dev, channel, data_defaults.clk_input[channel]);
		if (ret < 0)
			goto error;
	}

	/* Update all dac channels */
	ret = _ltc268x_spi_write(dev, LTC268X_CMD_UPDATE_ALL, 0);
	if (ret < 0)
		goto error;

	LOG_INF("LTC268X successfully initialized\n");
	return ret;

error:
	LOG_ERR("LTC268X initialization error (%d)\n", ret);
	return ret;
}

static int ltc268x_channel_setup(const struct device *dev,
				 const struct dac_channel_cfg *channel_cfg)
{
	const struct ltc268x_config *config = dev->config;

	if (channel_cfg->channel_id > config->nchannels - 1) {
		LOG_ERR("Unsupported channel %d", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != config->resolution) {
		LOG_ERR("Unsupported resolution %d", channel_cfg->resolution);
		return -ENOTSUP;
	}

	return 0;
}

static int ltc268x_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct ltc268x_config *config = dev->config;
	struct ltc268x_data *data = dev->data;

	if (channel > config->nchannels - 1) {
		LOG_ERR("%s: Unsupported channel %d", __func__, channel);
		return -ENOTSUP;
	}

	if (value >= (1 << config->resolution)) {
		LOG_ERR("%s: Value %d out of range", __func__, value);
		return -EINVAL;
	}

	data->dac_code[channel] = value;

	/* Write to the Data Register of the DAC. */
	return _ltc268x_spi_write(dev, LTC268X_CMD_CH_CODE_UPDATE(channel, config->dev_id), value);
}

static const struct dac_driver_api ltc268x_driver_api = {
	.channel_setup = ltc268x_channel_setup,
	.write_value = ltc268x_write_value,
};

#define DT_INST_LTC268X(instance, model) DT_INST(instance, lltc_ltc##model)

#define LTC268X_DEVICE(instance, model, deviceid, res, nchan)                                      \
	static struct ltc268x_data ltc##model##_data_##instance = {};                              \
	static const struct ltc268x_config ltc##model##_config_##instance = {                      \
		.bus = SPI_DT_SPEC_GET(DT_INST_LTC268X(instance, model),                           \
				       SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),                   \
		.dev_id = deviceid,                                                                \
		.resolution = res,                                                                 \
		.nchannels = nchan,                                                                \
	};                                                                                         \
	DEVICE_DT_DEFINE(DT_INST_LTC268X(instance, model), &ltc268x_init, NULL,                    \
			 &ltc##model##_data_##instance, &ltc##model##_config_##instance,           \
			 POST_KERNEL, CONFIG_DAC_LTC268X_INIT_PRIORITY, &ltc268x_driver_api);

/*
 * LTC2688: 16-channel / 16-bit
 */
#define LTC2688_DEVICE(instance) LTC268X_DEVICE(instance, 2688, LTC2688, 16, 16)

/*
 * LTC2686: 8-channel / 16-bit
 */
#define LTC2686_DEVICE(instance) LTC268X_DEVICE(instance, 2686, LTC2686, 8, 16)

#define CALL_WITH_ARG(arg, expr) expr(arg)

#define INST_DT_LTC268X_FOREACH(inst_expr, model)                                                  \
	LISTIFY(DT_NUM_INST_STATUS_OKAY(lltc_ltc##model), CALL_WITH_ARG, (), inst_expr)

INST_DT_LTC268X_FOREACH(LTC2688_DEVICE, 2688);
INST_DT_LTC268X_FOREACH(LTC2686_DEVICE, 2686);

#endif // DT_HAS_COMPAT_STATUS_OKAY(lltc_ltc2688) || DT_HAS_COMPAT_STATUS_OKAY(lltc_ltc2686)
