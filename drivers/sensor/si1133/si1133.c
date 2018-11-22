/* si1133.c - Driver for SI1133 UV index and ambient light sensor */

/*
 * Copyright (c) 2018 Thomas Li Fredriksen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "si1133.h"

#include <gpio.h>
#include <i2c.h>
#include <sensor.h>
#include <logging/log.h>

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(SI1133);

static const struct si1133_adc_config_params {

	u8_t ADCCONFIG;
	u8_t ADCSENS;
	u8_t ADCPOST;
	u8_t MEASCONFIG;
} si1133_adc_config_params[] = {
	{
		.ADCCONFIG = SI1133_PARAM_ADCCONFIG0,
		.ADCSENS = SI1133_PARAM_ADCSENS0,
		.ADCPOST = SI1133_PARAM_ADCPOST0,
		.MEASCONFIG = SI1133_PARAM_MEASCONFIG0
	},
	{
		.ADCCONFIG = SI1133_PARAM_ADCCONFIG1,
		.ADCSENS = SI1133_PARAM_ADCSENS1,
		.ADCPOST = SI1133_PARAM_ADCPOST1,
		.MEASCONFIG = SI1133_PARAM_MEASCONFIG1
	},
	{
		.ADCCONFIG = SI1133_PARAM_ADCCONFIG2,
		.ADCSENS = SI1133_PARAM_ADCSENS2,
		.ADCPOST = SI1133_PARAM_ADCPOST2,
		.MEASCONFIG = SI1133_PARAM_MEASCONFIG2
	},
	{
		.ADCCONFIG = SI1133_PARAM_ADCCONFIG3,
		.ADCSENS = SI1133_PARAM_ADCSENS3,
		.ADCPOST = SI1133_PARAM_ADCPOST3,
		.MEASCONFIG = SI1133_PARAM_MEASCONFIG3
	},
	{
		.ADCCONFIG = SI1133_PARAM_ADCCONFIG4,
		.ADCSENS = SI1133_PARAM_ADCSENS4,
		.ADCPOST = SI1133_PARAM_ADCPOST4,
		.MEASCONFIG = SI1133_PARAM_MEASCONFIG4
	},
	{
		.ADCCONFIG = SI1133_PARAM_ADCCONFIG5,
		.ADCSENS = SI1133_PARAM_ADCSENS5,
		.ADCPOST = SI1133_PARAM_ADCPOST5,
		.MEASCONFIG = SI1133_PARAM_MEASCONFIG5
	}
};

int si1133_init_chip(struct device *dev);

static int si1133_reg_read(struct device *dev, u8_t addr, u8_t *dst)
{
	int err;

	const struct si1133_config *config = dev->config->config_info;
	struct si1133_data *data = dev->driver_data;

	addr |= 0x40;

	err = i2c_reg_read_byte(data->i2c_master,
			config->i2c_slave_addr, addr, dst);
	if (err < 0) {
		LOG_ERR("I2C read error");
		return -EIO;
	}

	return 0;
}

static int si1133_reg_write(struct device *dev, u8_t addr, u8_t val)
{
	int err;

	const struct si1133_config *config = dev->config->config_info;
	struct si1133_data *data = dev->driver_data;

	addr |= 0x40;

	err = i2c_reg_write_byte(data->i2c_master,
			config->i2c_slave_addr, addr, val);
	if (err < 0) {
		LOG_ERR("I2C write error");
		return -EIO;
	}

	return 0;
}

static int si1133_burst_read(struct device *dev,
		u8_t addr, bool incr, u8_t *dst, int count)
{
	int err;

	const struct si1133_config *config = dev->config->config_info;
	struct si1133_data *data = dev->driver_data;

	addr &= ~0xC0;
	if (!incr) {
		addr |= SI1133_I2C_ADDR_INCR_DIS_MASK;
	}

	err = i2c_burst_read(data->i2c_master,
			config->i2c_slave_addr, addr, dst, count);
	if (err < 0) {
		return -EIO;
	}

	return 0;
}

static int si1133_burst_write(struct device *dev,
		u8_t addr, bool incr, u8_t *src, int count)
{
	int err;

	const struct si1133_config *config = dev->config->config_info;
	struct si1133_data *data = dev->driver_data;

	addr &= ~0xC0;
	if (!incr) {
		addr |= SI1133_I2C_ADDR_INCR_DIS_MASK;
	}

	err = i2c_burst_write(data->i2c_master,
			config->i2c_slave_addr, addr, src, count);
	if (err < 0) {
		return -EIO;
	}

	return 0;
}

int si1133_reset(struct device *dev)
{
	int err;

	err = si1133_reg_write(dev, SI1133_REG_COMMAND, SI1133_CMD_RESET);
	if (err < 0) {
		return err;
	}

	k_sleep(10);

	return 0;
}

static int si1133_handle_error(struct device *dev)
{
	int err;

	struct si1133_data *data = dev->driver_data;

	u16_t err_code = data->resp & (SI1133_REG_RESPONSE0_CMD_ERR_MASK
			| SI1133_REG_RESPONSE0_CMMND_CTR_MASK);

	switch (err_code) {
	case SI1133_CMD_ERR_NONE:
		return 0;
	case SI1133_CMD_ERR_CMD_INVALID:
		LOG_ERR("Invalid command");
		break;
	case SI1133_CMD_ERR_PARAM_INVALID:
		LOG_ERR("Parameter access to an invalid location");
		break;
	case SI1133_CMD_ERR_ADC_OVERFLOW:
		LOG_ERR("Saturation of the ADC or overflow of accumulation");
		break;
	case SI1133_CMD_ERR_BUFFER_OVERFLOW:
		LOG_ERR("Output buffer overflow");
		break;
	default:
		LOG_ERR("Unrecognized error");
	}

	/* Reset chip */

	err = si1133_reset(dev);
	if (err < 0) {
		return err;
	}

	/* Re-initialize */

	return si1133_init_chip(dev);
}

static int si1133_cmd(struct device *dev, u8_t cmd)
{
	u8_t cnt;
	int err;

	struct si1133_data *data = dev->driver_data;

	int retries = 10;

	/* Read response-register for counter-value */

	err = si1133_reg_read(dev, SI1133_REG_RESPONSE0, &data->resp);
	if (err < 0) {
		return err;
	}

	/* Check error-bit */

	if (data->resp & SI1133_REG_RESPONSE0_CMD_ERR_MASK) {
		return -ECANCELED;
	}

	cnt = data->resp & SI1133_REG_RESPONSE0_CMMND_CTR_MASK;

	/* Send command */

	err = si1133_reg_write(dev, SI1133_REG_COMMAND, cmd);
	if (err < 0) {
		return err;
	}

	/* Wait for command to complete */

	do {
		err = si1133_reg_read(dev, SI1133_REG_RESPONSE0, &data->resp);
		if (err < 0) {
			return err;
		}

		/* Check error-bit */

		if (data->resp & SI1133_REG_RESPONSE0_CMD_ERR_MASK) {
			si1133_handle_error(dev);

			return -ECANCELED;
		}

		/* Check if counter has changed */

		if ((data->resp & SI1133_REG_RESPONSE0_CMMND_CTR_MASK) != cnt) {
			break;
		}

		k_sleep(10);

	} while (--retries);

	return 0;
}

static int si1133_set_param(struct device *dev, u8_t param, u8_t val)
{
	u8_t cnt;
	int err;

	struct si1133_data *data = dev->driver_data;
	int retries = 10;

	u8_t tx[] = {
		val,
		0x80 | (param & 0x3F)
	};

	/* Get status */

	err = si1133_reg_read(dev, SI1133_REG_RESPONSE0, &data->resp);
	if (err < 0) {
		return err;
	}
	if (data->resp & SI1133_REG_RESPONSE0_CMD_ERR_MASK) {
		/* The chip is indicating an error */

		return -ECANCELED;
	}

	cnt = data->resp & SI1133_REG_RESPONSE0_CMMND_CTR_MASK;

	err = si1133_burst_write(dev, SI1133_REG_HOSTIN0, true, tx, sizeof(tx));
	if (err < 0) {
		return err;
	}

	do {
		err = si1133_reg_read(dev, SI1133_REG_RESPONSE0, &data->resp);
		if (err < 0) {
			return err;
		}
		if (data->resp & SI1133_REG_RESPONSE0_CMD_ERR_MASK) {
			/* The chip is indicating an error */

			return -ECANCELED;
		}
		if ((data->resp & SI1133_REG_RESPONSE0_CMMND_CTR_MASK) != cnt) {
			break;
		}

		k_sleep(1);
	} while (--retries);

	return 0;
}

int si1133_init_chip(struct device *dev)
{
	int err;
	int i;

	const struct si1133_config *config = dev->config->config_info;
	struct si1133_data *data = dev->driver_data;

	data->chn_mask = 0;

	/*
	 * Initialize all channels
	 */

	for (i = 0; i < config->channel_count; i++) {
		err = si1133_set_param(dev, si1133_adc_config_params[i].ADCCONFIG,
				config->channels[i].decim_rate
					| config->channels[i].adc_select);
		if (err < 0) {
			return err;
		}

		err = si1133_set_param(dev, si1133_adc_config_params[i].ADCSENS,
				config->channels[i].hsig
					| config->channels[i].sw_gain
					| config->channels[i].hw_gain);
		if (err < 0) {
			return err;
		}

		err = si1133_set_param(dev, si1133_adc_config_params[i].ADCPOST,
				SI1133_ADCPOSTX_24BIT_OUT(1)
					| SI1133_ADCPOSTX_THRESH_EN(0)
					| config->channels[i].post_shift);
		if (err < 0) {
			return err;
		}

		err = si1133_set_param(dev, si1133_adc_config_params[i].MEASCONFIG,
				SI1133_MEASCONFIGX_COUNTER_INDEX(0));
		if (err < 0) {
			return err;
		}

		data->chn_mask |= BIT(i);
	}

    err = si1133_set_param(dev, SI1133_PARAM_CH_LIST, data->chn_mask);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int si1133_calc_output(s32_t x, s32_t y, u8_t x_order, u8_t y_order,
		u8_t input_fraction, s8_t sign, const struct si1133_coeff *coeff)
{
	s8_t shift;

	int x1 = 1;
	int x2 = 1;
	int y1 = 1;
	int y2 = 1;

	shift = ((u16_t) coeff->info & 0xFF00) >> 8;
	shift ^= 0xFF;
	shift += 1;
	shift = -shift;

	if (x_order > 0) {
		x1 = ((x << input_fraction) / coeff->mag) << shift;

		if (x_order > 1) {
			x2 = x1;
		}
	}

	if (y_order > 0) {
		y1 = ((y << input_fraction) / coeff->mag) << shift;

		if (y_order > 1) {
			y2 = y1;
		}
	}

	return sign * x1 * x2 * y1 * y2;
}

/*
 * Ported algorithm from Linux-driver:
 *   https://github.com/torvalds/linux/blob/master/drivers/iio/light/si1133.c#L279
 */
static int si1133_calc_polynomial(s32_t white, s32_t ir, u8_t input_fraction,
		const struct si1133_coeff *coeffs, int coeff_count)
{
	u8_t x_order;
	u8_t y_order;
	s8_t sign;
	int i;

	int ret = 0;

	for (i = 0; i < coeff_count; i++) {

		if (coeffs[i].info < 0) {
			sign = -1;
		} else {
			sign = 1;
		}

		x_order = (coeffs[i].info & SI1133_X_ORDER_MASK) >> SI1133_X_ORDER_MASK_SHIFT;
		y_order = (coeffs[i].info & SI1133_Y_ORDER_MASK) >> SI1133_Y_ORDER_MASK_SHIFT;

		if (x_order == 0 && y_order == 0) {
			ret += sign * coeffs[i].mag << SI1133_LUX_OUTPUT_FRACTION;
		} else {
			ret += si1133_calc_output(white, ir, x_order, y_order, input_fraction, sign, &coeffs[i]);
		}
	}

	return ret < 0 ? -ret : ret;
}

static int si1133_get_lux(struct device *dev, int *val)
{
	int channel;
	int lux;

	struct si1133_data *data = dev->driver_data;
	const struct si1133_config *config = dev->config->config_info;

	/*
	 * Select which photodiode to use for the LUX-calculation
	 */

	if (data->channels[SI1133_CHANNEL_HIGH_VIS].hostout > SI1133_ADC_THRESHOLD
			|| data->channels[SI1133_CHANNEL_IR].hostout > SI1133_ADC_THRESHOLD) {
		channel  = SI1133_CHANNEL_HIGH_VIS;
	} else {
		if ((data->chn_mask) & BIT(SI1133_CHANNEL_LOW_VIS)) {
			/* Use Low-light photodiode */

			channel  = SI1133_CHANNEL_HIGH_VIS;
		} else {
			/*
			 * Low-light photodiode was not sampled
			 * Use High-light photodiode instead
			 */

			channel  = SI1133_CHANNEL_HIGH_VIS;
		}
	}

	lux = si1133_calc_polynomial(data->channels[channel].hostout, data->channels[SI1133_CHANNEL_IR].hostout,
			config->channels[channel].input_fraction, config->channels[channel].coeff, config->channels[channel].coeff_count);

	*val = lux >> SI1133_LUX_OUTPUT_FRACTION;

	return 0;
}

static int si1133_wait_for_sample(struct device *dev)
{
	u8_t regval;
	int err;

	struct si1133_data *data = dev->driver_data;

	int retries = 10;

	/* Wait for interrupt-status do indicate that the samples are ready */

	do {
		err = si1133_reg_read(dev, SI1133_REG_IRQ_STATUS, &regval);
		if (err < 0) {
			return err;
		}
		if ((regval & data->chn_mask) != data->chn_mask) {
			break;
		}

		k_sleep(1);
	} while (--retries);

	return retries > 0 ? 0 : -EIO;
}

static int si1133_read_sample(struct device *dev)
{
	u8_t hostout[3];
	int err;
	int i;

	const struct si1133_config *config = dev->config->config_info;
	struct si1133_data *data = dev->driver_data;

	/* Read all samples */

	for (i = 0; i < config->channel_count; i++) {
		if (!(data->chn_mask & BIT(i))) {
			continue;
		}

		err = si1133_burst_read(dev, SI1133_REG_HOSTOUT0 + i * sizeof(hostout),
				true, hostout, sizeof(hostout));
		if (err < 0) {
			return err;
		}

		/* Convert from 24-bit signed integer */

		if (hostout[0] & 0x80) {
			data->channels[i].hostout = (0xFF << 24);
		} else {
			data->channels[i].hostout = 0;
		}

		data->channels[i].hostout |= (hostout[0] << 16)
				| (hostout[1] << 8) | hostout[2];
	}

	if (data->channels[SI1133_CHANNEL_HIGH_VIS].hostout > SI1133_ADC_THRESHOLD
			|| data->channels[SI1133_CHANNEL_IR].hostout > SI1133_ADC_THRESHOLD) {
		data->chn_mask &= ~BIT(SI1133_CHANNEL_LOW_VIS);
	} else {
		data->chn_mask |= BIT(SI1133_CHANNEL_LOW_VIS);
	}

	return 0;
}

/********************************************
 ******************* API ********************
 ********************************************/

static int si1133_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	int err;

	struct si1133_data *data = dev->driver_data;

	/* Update channel list */

    err = si1133_set_param(dev, SI1133_PARAM_CH_LIST, data->chn_mask);
	if (err < 0) {
		return err;
	}

	/* Trigger sample */

	err = si1133_cmd(dev, SI1133_CMD_FORCE_CH);
	if (err == -ECANCELED) {

		err = si1133_handle_error(dev);
		if (err == 0) {
			si1133_init_chip(dev);
		}

		/* Disable low-light photodiode */

		data->chn_mask &= ~BIT(SI1133_CHANNEL_LOW_VIS);

		return -EIO;
	}
	if (err < 0) {
		return -EIO;
	}

	/* Wait for sample */

	err = si1133_wait_for_sample(dev);
	if (err < 0) {
		return -EIO;
	}

	err = si1133_read_sample(dev);
	if (err < 0) {
		return -EIO;
	}

	return 0;
}

static int si1133_channel_get(struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	int lux;
	int err;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -EINVAL;
	}

	err = si1133_get_lux(dev, &lux);
	if (err < 0) {
		return err;
	}

	val->val1 = lux;
	val->val2 = 0;

	return 0;
}

static const struct sensor_driver_api si1133_api_funcs = {
	.sample_fetch = si1133_sample_fetch,
	.channel_get = si1133_channel_get,
};

/********************************************
 ****************** INIT ********************
 ********************************************/

static int si1133_init(struct device *dev)
{
	int err;

	const struct si1133_config *config = dev->config->config_info;
	struct si1133_data *data = dev->driver_data;

	data->i2c_master = device_get_binding(config->i2c_dev_name);
	if (!data->i2c_master) {
		LOG_ERR("SI1133: I2C master not found");
		return -EINVAL;
	}

	err = si1133_reset(dev);
	if (err < 0) {
		LOG_ERR("SI1133: Failed to reset chip");
		return -EIO;
	}

	err = si1133_init_chip(dev);
	if (err < 0) {
		LOG_ERR("SI1133: Failed to init chip");
		return -EIO;
	}

	return 0;
}

/*
 * Coefficients used with LUX-calculation
 *
 * Picked from SI1133 Linux-driver
 * https://github.com/torvalds/linux/blob/master/drivers/iio/light/si1133.c#L217
 */
static const struct si1133_coeff si1133_coeff_high[] = {

	{0,	209},
	{1665, 93},
	{2064, 65},
	{-2671, 234}
};

static const struct si1133_coeff si1133_coeff_low[] = {

	{0, 0},
	{1921, 29053},
	{-1022, 36363},
	{2320, 20789},
	{-367, 57909},
	{-1774, 38240},
	{-608, 46775},
	{-1503, 51831},
	{-1886, 58928}
};

static const struct si1133_config si1133_config = {

	.i2c_dev_name = CONFIG_SI1133_I2C_DEV_NAME,
	.i2c_slave_addr = CONFIG_SI1133_I2C_ADDR,

	.channel_count = SI1133_CHANNEL_COUNT,

	/*
	 * Configure three different channels for Lux measurement.
	 *
	 * Configuration picked from SI1133 Linux driver
	 * https://github.com/torvalds/linux/blob/master/drivers/iio/light/si1133.c#L875
	 */
	.channels = {
		{
			.decim_rate = SI1133_ADCCONFIGX_DECIM_RATE(1),
			.adc_select = SI1133_ADCCONFIG_ADCMUX_LARGE_WHITE,
			.sw_gain = SI1133_ADCSENSX_SW_GAIN(6),
			.hw_gain = SI1133_ADCSENSX_HW_GAIN(1),
			.post_shift = SI1133_ADCPOSTX_POSTSHIFT(0),
			.hsig = SI1133_ADCSENSX_HSIG(1),
			.input_fraction = SI1133_INPUT_FRACTION_HIGH,
			.coeff = si1133_coeff_high,
			.coeff_count = ARRAY_SIZE(si1133_coeff_high)
		},
		{
			.decim_rate = SI1133_ADCCONFIGX_DECIM_RATE(1),
			.adc_select = SI1133_ADCCONFIG_ADCMUX_LARGE_WHITE,
			.sw_gain = SI1133_ADCSENSX_SW_GAIN(0),
			.hw_gain = SI1133_ADCSENSX_HW_GAIN(7),
			.post_shift = SI1133_ADCPOSTX_POSTSHIFT(2),
			.hsig = SI1133_ADCSENSX_HSIG(1),
			.input_fraction = SI1133_INPUT_FRACTION_LOW,
			.coeff = si1133_coeff_low,
			.coeff_count = ARRAY_SIZE(si1133_coeff_low)
		},
		{
			.decim_rate = SI1133_ADCCONFIGX_DECIM_RATE(1),
			.adc_select = SI1133_ADCCONFIG_ADCMUX_MEDIUM_IR,
			.sw_gain = SI1133_ADCSENSX_SW_GAIN(6),
			.hw_gain = SI1133_ADCSENSX_HW_GAIN(2),
			.post_shift = SI1133_ADCPOSTX_POSTSHIFT(2),
			.hsig = SI1133_ADCSENSX_HSIG(1),
			.coeff = NULL,
			.coeff_count = 0
		},
	},
};

static struct si1133_data si1133_data;

DEVICE_AND_API_INIT(si1133, CONFIG_SI1133_DEV_NAME, si1133_init, &si1133_data,
		&si1133_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &si1133_api_funcs);
