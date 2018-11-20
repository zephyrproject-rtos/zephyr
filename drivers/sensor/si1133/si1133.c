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

static int si1133_reg_read(struct device *dev, uint8_t addr, uint8_t *dst)
{
	int err;

	const struct si1133_config *config = dev->config->config_info;
	struct si1133_data *data = dev->driver_data;

	addr |= 0x40;

	err = i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr, addr, dst);
	if(err < 0)
	{
		LOG_ERR("I2C read error");
		return -EIO;
	}

	return 0;
}

static int si1133_reg_write(struct device *dev, uint8_t addr, uint8_t val)
{
	int err;

	const struct si1133_config *config = dev->config->config_info;
	struct si1133_data *data = dev->driver_data;

	addr |= 0x40;

	err = i2c_reg_write_byte(data->i2c_master, config->i2c_slave_addr, addr, val);
	if(err < 0)
	{
		LOG_ERR("I2C write error");
		return -EIO;
	}

	return 0;
}

static int si1133_burst_read(struct device *dev, uint8_t addr, bool incr, uint8_t *dst, int count)
{
	int err;

	const struct si1133_config *config = dev->config->config_info;
	struct si1133_data *data = dev->driver_data;

	addr &= ~0xC0;
	if(!incr)
	{
		addr |= SI1133_I2C_ADDR_INCR_DIS_MASK;
	}

	err = i2c_burst_read(data->i2c_master, config->i2c_slave_addr, addr, dst, count);
	if(err < 0)
	{
		return -EIO;
	}

	return 0;
}

static int si1133_burst_write(struct device *dev, uint8_t addr, bool incr, uint8_t *src, int count)
{
	int err;

	const struct si1133_config *config = dev->config->config_info;
	struct si1133_data *data = dev->driver_data;

	addr &= ~0xC0;
	if(!incr)
	{
		addr |= SI1133_I2C_ADDR_INCR_DIS_MASK;
	}

	err = i2c_burst_write(data->i2c_master, config->i2c_slave_addr, addr, src, count);
	if(err < 0)
	{
		return -EIO;
	}

	return 0;
}

int si1133_reset(struct device *dev)
{
	int err;

	err = si1133_reg_write(dev, SI1133_REG_COMMAND, SI1133_CMD_RESET);
	if(err < 0)
	{
		return err;
	}

	k_sleep(10);

	return 0;
}

static int si1133_handle_error(struct device *dev)
{
	int err;

	struct si1133_data *data = dev->driver_data;

	switch(data->resp & (SI1133_REG_RESPONSE0_CMD_ERR_MASK | SI1133_REG_RESPONSE0_CMMND_CTR_MASK))
	{
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
	if(err < 0)
	{
		return err;
	}

	/* Re-initialize */

	return si1133_init_chip(dev);
}

static int si1133_cmd(struct device *dev, uint8_t cmd)
{
	uint8_t cnt;
	int err;

	struct si1133_data *data = dev->driver_data;

	int retries = 10;

	/* Read response-register for counter-value */

	err = si1133_reg_read(dev, SI1133_REG_RESPONSE0, &data->resp);
	if(err < 0)
	{
		return err;
	}

	/* Check error-bit */

	if(data->resp & SI1133_REG_RESPONSE0_CMD_ERR_MASK)
	{
		return -ECANCELED;
	}

	cnt = data->resp & SI1133_REG_RESPONSE0_CMMND_CTR_MASK;

	/* Send command */

	err = si1133_reg_write(dev, SI1133_REG_COMMAND, cmd);
	if(err < 0)
	{
		return err;
	}

	/* Wait for command to complete */

	do {
		err = si1133_reg_read(dev, SI1133_REG_RESPONSE0, &data->resp);
		if(err < 0)
		{
			return err;
		}

		/* Check error-bit */

		if(data->resp & SI1133_REG_RESPONSE0_CMD_ERR_MASK)
		{
			si1133_handle_error(dev);

			return -EIO;
		}

		/* Check if counter has changed */

		if((data->resp & SI1133_REG_RESPONSE0_CMMND_CTR_MASK) != cnt)
		{
			break;
		}

		k_sleep(10);

	} while(--retries);

	return 0;
}

static int si1133_set_param(struct device *dev, uint8_t param, uint8_t val)
{
	uint8_t cnt;
	int err;

	struct si1133_data *data = dev->driver_data;
	int retries = 10;

	uint8_t tx[] = {
		val,
		0x80 | (param & 0x3F)
	};

	/* Get status */

	err = si1133_reg_read(dev, SI1133_REG_RESPONSE0, &data->resp);
	if(err < 0)
	{
		return err;
	}
	if(data->resp & SI1133_REG_RESPONSE0_CMD_ERR_MASK)
	{
		/* The chip is indicating an error */

		return -ECANCELED;
	}

	cnt = data->resp & SI1133_REG_RESPONSE0_CMMND_CTR_MASK;

	err = si1133_burst_write(dev, SI1133_REG_HOSTIN0, true, tx, sizeof(tx));
	if(err < 0)
	{
		return err;
	}

	do {
		err = si1133_reg_read(dev, SI1133_REG_RESPONSE0, &data->resp);
		if(err < 0)
		{
			return err;
		}
		if(data->resp & SI1133_REG_RESPONSE0_CMD_ERR_MASK)
		{
			/* The chip is indicating an error */

			return -ECANCELED;
		}
		if((data->resp & SI1133_REG_RESPONSE0_CMMND_CTR_MASK) != cnt)
		{
			break;
		}

		k_sleep(1);
	} while(--retries);

	return 0;
}

int si1133_init_chip(struct device *dev)
{
	int err;
	int i;

	const struct si1133_config *config = dev->config->config_info;
	struct si1133_data *data = dev->driver_data;

	data->chn_mask = 0;

	for(i = 0; i < config->channel_count; i++)
	{
		err = si1133_set_param(dev, si1133_adc_config_params[i].ADCCONFIG,
				config->channels[i].decim_rate | config->channels[i].adc_select);
		if(err < 0)
		{
			return err;
		}

		err = si1133_set_param(dev, si1133_adc_config_params[i].ADCSENS,
				config->channels[i].hsig | config->channels[i].sw_gain | config->channels[i].hw_gain);
		if(err < 0)
		{
			return err;
		}

		err = si1133_set_param(dev, si1133_adc_config_params[i].ADCPOST,
				SI1133_ADCPOSTX_24BIT_OUT(1) | SI1133_ADCPOSTX_THRESH_EN(0) | config->channels[i].post_shift);
		if(err < 0)
		{
			return err;
		}

		err = si1133_set_param(dev, si1133_adc_config_params[i].MEASCONFIG,
				SI1133_MEASCONFIGX_COUNTER_INDEX(0));
		if(err < 0)
		{
			return err;
		}

		data->chn_mask |= BIT(i);
	}

    err = si1133_set_param(dev, SI1133_PARAM_CH_LIST, data->chn_mask);
	if(err < 0)
	{
		return err;
	}

	return 0;
}

static int si1133_wait_for_sample(struct device *dev)
{
	uint8_t regval;
	int err;

	struct si1133_data *data = dev->driver_data;

	int retries = 10;

	do {
		err = si1133_reg_read(dev, SI1133_REG_IRQ_STATUS, &regval);
		if(err < 0)
		{
			return err;
		}
		if((regval & data->chn_mask) != data->chn_mask)
		{
			break;
		}
		
		k_sleep(1);
	} while(--retries);

	return retries > 0 ? 0 : -EIO;
}

static int si1133_read_sample(struct device *dev)
{
	uint8_t hostout[3];
	int err;
	int i;

	const struct si1133_config *config = dev->config->config_info;
	struct si1133_data *data = dev->driver_data;

	/* Read all samples */

	for(i = 0; i < config->channel_count; i++)
	{
		err = si1133_burst_read(dev, SI1133_REG_HOSTOUT0 + i * sizeof(hostout),
				true, hostout, sizeof(hostout));
		if(err < 0)
		{
			return err;
		}

		/* Convert from 24-bit signed integer */

		if(hostout[0] & 0x80)
		{
			data->channels[i].hostout = (0xFF << 24);
		}
		else
		{
			data->channels[i].hostout = 0;
		}

		data->channels[i].hostout |= (hostout[0] << 16)
				| (hostout[1] << 8) | hostout[2];

#warning "TEST CODE"
		printk("SI1133 HOSTOUT[%d]: hex=0x%02X%02X%02X, dec=%d\n",
				i, hostout[0], hostout[1], hostout[2], data->channels[i].hostout);
	}

	return 0;
}

/********************************************
 ******************* API ********************
 ********************************************/

static int si1133_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	int err;

	/* Trigger sample */

	err = si1133_cmd(dev, SI1133_CMD_FORCE_CH);
	if(err == -ECANCELED)
	{
		err = si1133_handle_error(dev);
		if(err == 0)
		{
			si1133_init_chip(dev);
		}

		return -EIO;
	}
	if(err < 0)
	{
		return -EIO;
	}

#ifndef CONFIG_SI1133_TRIGGER

	/* Wait for sample */

	err = si1133_wait_for_sample(dev);
	if(err < 0)
	{
		return -EIO;
	}

	err = si1133_read_sample(dev);
	if(err < 0)
	{
		return -EIO;
	}
#endif

	return 0;
}

static int si1133_channel_get(struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	/* TODO */

	return 0;
}

static const struct sensor_driver_api si1133_api_funcs = {
	.sample_fetch = si1133_sample_fetch,
	.channel_get = si1133_channel_get,
#ifdef CONFIG_SI1133_TRIGGER
	.trigger_set = si1133_trigger_set,
#endif
};

/********************************************
 ****************** INIT ********************
 ********************************************/

static int si1133_init(struct device *dev)
{
	int err;

	const struct si1133_config *config = dev->config->config_info;
	struct si1133_data *data = dev->driver_data;

	LOG_DBG("TST");

	data->i2c_master = device_get_binding(config->i2c_dev_name);
	if(!data->i2c_master)
	{
		LOG_ERR("SI1133: I2C master not found");
		return -EINVAL;
	}

	err = si1133_reset(dev);
	if(err < 0)
	{
		LOG_ERR("SI1133: Failed to reset chip");
		return -EIO;
	}

	err = si1133_init_chip(dev);
	if(err < 0)
	{
		LOG_ERR("SI1133: Failed to init chip");
		return -EIO;
	}

#ifdef CONFIG_SI1133_TRIGGER
	err = si1133_init_interrupt(dev);
	if(err < 0)
	{
		LOG_ERR("SI1133: Failed to init interrupt");
		return -EIO;
	}
#endif /* CONFIG_SI1133_TRIGGER */

	return 0;
}

static const struct si1133_config si1133_config = {

	.i2c_dev_name = CONFIG_SI1133_I2C_DEV_NAME,
	.i2c_slave_addr = CONFIG_SI1133_I2C_ADDR,

	.channel_count = SI1133_CHANNEL_COUNT,
	.channels = {
		{
			.decim_rate = SI1133_ADCCONFIGX_DECIM_RATE(1),
			.adc_select = SI1133_ADCCONFIG_ADCMUX_LARGE_WHITE,
			.sw_gain = SI1133_ADCSENSX_SW_GAIN(6),
			.hw_gain = SI1133_ADCSENSX_HW_GAIN(1),
			.post_shift = SI1133_ADCPOSTX_POSTSHIFT(0),
			.hsig = SI1133_ADCSENSX_HSIG(1),
		},
		{
			.decim_rate = SI1133_ADCCONFIGX_DECIM_RATE(1),
			.adc_select = SI1133_ADCCONFIG_ADCMUX_LARGE_WHITE,
			.sw_gain = SI1133_ADCSENSX_SW_GAIN(0),
			.hw_gain = SI1133_ADCSENSX_HW_GAIN(7),
			.post_shift = SI1133_ADCPOSTX_POSTSHIFT(2),
			.hsig = SI1133_ADCSENSX_HSIG(1),
		},
		{
			.decim_rate = SI1133_ADCCONFIGX_DECIM_RATE(1),
			.adc_select = SI1133_ADCCONFIG_ADCMUX_MEDIUM_IR,
			.sw_gain = SI1133_ADCSENSX_SW_GAIN(6),
			.hw_gain = SI1133_ADCSENSX_HW_GAIN(2),
			.post_shift = SI1133_ADCPOSTX_POSTSHIFT(2),
			.hsig = SI1133_ADCSENSX_HSIG(1),
		},
		{
			.decim_rate = SI1133_ADCCONFIGX_DECIM_RATE(0),
			.adc_select = SI1133_ADCCONFIG_ADCMUX_UV,
			.sw_gain = SI1133_ADCSENSX_SW_GAIN(0),
			.hw_gain = SI1133_ADCSENSX_HW_GAIN(0),
			.post_shift = SI1133_ADCPOSTX_POSTSHIFT(0),
			.hsig = SI1133_ADCSENSX_HSIG(0),
		},
	},
};

static struct si1133_data si1133_data;

DEVICE_AND_API_INIT(si1133, CONFIG_SI1133_DEV_NAME, si1133_init, &si1133_data,
		&si1133_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &si1133_api_funcs);
