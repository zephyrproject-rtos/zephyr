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

static int si1133_reg_read(struct device *dev, uint8_t addr, uint8_t *dst)
{
	int err;

	struct si1133_data *data = dev->driver_data;

	addr |= 0x40;

	err = i2c_reg_read_byte(data->i2c_master, data->i2c_slave_addr, addr, dst);
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

	struct si1133_data *data = dev->driver_data;

	addr |= 0x40;

	err = i2c_reg_write_byte(data->i2c_master, data->i2c_slave_addr, addr, val);
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

	struct si1133_data *data = dev->driver_data;

	addr &= ~0xC0;
	if(!incr)
	{
		addr |= SI1133_I2C_ADDR_INCR_DIS_MASK;
	}

	err = i2c_burst_read(data->i2c_master, data->i2c_slave_addr, addr, dst, count);
	if(err < 0)
	{
		return -EIO;
	}

	return 0;
}

static int si1133_burst_write(struct device *dev, uint8_t addr, bool incr, uint8_t *src, int count)
{
	int err;

	struct si1133_data *data = dev->driver_data;

	addr &= ~0xC0;
	if(!incr)
	{
		addr |= SI1133_I2C_ADDR_INCR_DIS_MASK;
	}

	err = i2c_burst_write(data->i2c_master, data->i2c_slave_addr, addr, src, count);
	if(err < 0)
	{
		return -EIO;
	}

	return 0;
}

static int si1133_wait_until_sleep(struct device *dev)
{
	uint8_t regval;
	int err;

	int retries = 5;

	do {
		err = si1133_reg_read(dev, SI1133_REG_RESPONSE0, &regval);
		if(err < 0)
		{
			return err;
		}
		if(regval == SI1133_RSP0_SLEEP)
		{
			return 0;
		}
		k_sleep(1);

	} while(--retries);

	return 0;
}

static int si1133_wait_for_command(struct device *dev, uint8_t resp)
{
	uint8_t regval;
	int err;

	resp &= SI1133_RSP0_COUNTER_MASK;

	do{
		err = si1133_reg_read(dev, SI1133_REG_RESPONSE0, &regval);
		if(err < 0)
		{
			return err;
		}
	} while((regval & SI1133_RSP0_COUNTER_MASK) == resp);

	return 0;
}

static int si1133_wait_until_stable(struct device *dev, uint8_t *resp)
{
	uint8_t regval;
	int err;

	int retries = 5;

	err = si1133_reg_read(dev, SI1133_REG_RESPONSE0, resp);
	if(err < 0)
	{
		return err;
	}

	*resp &= SI1133_RSP0_COUNTER_MASK;

	/* Ensure response is stable */

	do {
		err = si1133_wait_until_sleep(dev);
		if(err < 0)
		{
			return err;
		}

		err = si1133_reg_read(dev, SI1133_REG_RESPONSE0, &regval);
		if(err < 0)
		{
			return err;
		}

		regval &= SI1133_RSP0_COUNTER_MASK;

		if(regval == *resp)
		{
			break;
		}
		else
		{
			*resp = regval;
		}
	} while(--retries);

	return 0;
}

static int si1133_cmd(struct device *dev, uint8_t cmd)
{
	uint8_t resp;
	int err;

	/* Wait for response-register to stabilize */

	err = si1133_wait_until_stable(dev, &resp);
	if(err < 0)
	{
		return err;
	}

	/* Send command */

	err = si1133_reg_write(dev, SI1133_REG_COMMAND, cmd);
	if(err < 0)
	{
		return err;
	}

	/* Wait for command to complete */

	return si1133_wait_for_command(dev, resp);
}

static int si1133_set_param(struct device *dev, uint8_t addr, uint8_t val)
{
	uint8_t resp;
	int err;

	uint8_t tx[] = {
		val,
		0x80 | (addr & 0x3F)
	};

	err = si1133_wait_until_sleep(dev);
	if(err < 0)
	{
		return err;
	}

	err = si1133_reg_read(dev, SI1133_REG_RESPONSE0, &resp);
	if(err < 0)
	{
		return err;
	}

	resp &= SI1133_RSP0_COUNTER_MASK;

	err = si1133_burst_write(dev, SI1133_REG_HOSTIN0, true, tx, ARRAY_SIZE(tx));
	if(err < 0)
	{
		return err;
	}

	err = si1133_wait_for_command(dev, resp);
	if(err < 0)
	{
		return err;
	}

	return 0;
}

int si1133_reset(struct device *dev)
{
	int err;

	err = si1133_reg_write(dev, SI1133_REG_COMMAND, 0x01);
	if(err < 0)
	{
		LOG_ERR("Failed to send reset-command");
		return err;
	}

	k_sleep(10);

	uint8_t regval;

	err = si1133_reg_read(dev, SI1133_REG_PART_ID, &regval);
	if(err < 0)
	{
		LOG_ERR("Failed to get part-id");
		return err;
	}

	printk("SI1133 PART-ID: 0x%02X\n", regval);

	err = si1133_reg_read(dev, SI1133_REG_REV_ID, &regval);
	if(err < 0)
	{
		LOG_ERR("Failed to get revision");
		return err;
	}

	printk("SI1133 REV-ID: 0x%02X\n", regval);

	return 0;
}

int si1133_init_chip(struct device *dev)
{
	int err;

    err = si1133_set_param(dev, SI1133_PARAM_CH_LIST, 0x0f);
	if(err < 0)
	{
		return err;
	}

    err = si1133_set_param(dev, SI1133_PARAM_ADCCONFIG0,
			SI1133_ADCCONFIGX_DECIM_RATE(3) | SI1133_ADCCONFIG_ADCMUX_UV);
	if(err < 0)
	{
		return err;
	}

    err = si1133_set_param(dev, SI1133_PARAM_ADCSENS0,
			SI1133_ADCSENSX_HW_GAIN(9));
	if(err < 0)
	{
		return err;
	}

    err = si1133_set_param(dev, SI1133_PARAM_ADCPOST0,
			SI1133_ADCPOSTX_24BIT_OUT(1));
	if(err < 0)
	{
		return err;
	}

	err = si1133_set_param(dev, SI1133_PARAM_MEASCONFIG0,
			SI1133_MEASCONFIGX_COUNTER_INDEX(0));
	if(err < 0)
	{
		return err;
	}

    err = si1133_set_param(dev, SI1133_PARAM_ADCCONFIG1, 
			SI1133_ADCCONFIGX_DECIM_RATE(2) | SI1133_ADCCONFIG_ADCMUX_LARGE_WHITE);
	if(err < 0)
	{
		return err;
	}

    err = si1133_set_param(dev, SI1133_PARAM_ADCSENS1,
			SI1133_ADCSENSX_SW_GAIN(6) | SI1133_ADCSENSX_HW_GAIN(1));
	if(err < 0)
	{
		return err;
	}

    err = si1133_set_param(dev, SI1133_PARAM_ADCPOST1,
			SI1133_ADCPOSTX_24BIT_OUT(1));
	if(err < 0)
	{
		return err;
	}

	err = si1133_set_param(dev, SI1133_PARAM_MEASCONFIG1,
			SI1133_MEASCONFIGX_COUNTER_INDEX(0));
	if(err < 0)
	{
		return err;
	}

    err = si1133_set_param(dev, SI1133_PARAM_ADCCONFIG2,
			SI1133_ADCCONFIGX_DECIM_RATE(2) | SI1133_ADCCONFIG_ADCMUX_MEDIUM_IR);
	if(err < 0)
	{
		return err;
	}

    err = si1133_set_param(dev, SI1133_PARAM_ADCSENS2,
			SI1133_ADCSENSX_SW_GAIN(6) | SI1133_ADCSENSX_HW_GAIN(1));
	if(err < 0)
	{
		return err;
	}

    err = si1133_set_param(dev, SI1133_PARAM_ADCPOST2,
			SI1133_ADCPOSTX_24BIT_OUT(1) | SI1133_ADCPOSTX_POSTSHIFT(2));
	if(err < 0)
	{
		return err;
	}

	err = si1133_set_param(dev, SI1133_PARAM_MEASCONFIG2,
			SI1133_MEASCONFIGX_COUNTER_INDEX(0));
	if(err < 0)
	{
		return err;
	}

    err = si1133_set_param(dev, SI1133_PARAM_ADCCONFIG3,
			SI1133_ADCCONFIGX_DECIM_RATE(2) | SI1133_ADCCONFIG_ADCMUX_LARGE_WHITE);
	if(err < 0)
	{
		return err;
	}

    err = si1133_set_param(dev, SI1133_PARAM_ADCSENS3,
			SI1133_ADCSENSX_HW_GAIN(7));
	if(err < 0)
	{
		return err;
	}

    err = si1133_set_param(dev, SI1133_PARAM_ADCPOST3,
			SI1133_ADCPOSTX_24BIT_OUT(1));
	if(err < 0)
	{
		return err;
	}

	err = si1133_set_param(dev, SI1133_PARAM_MEASCONFIG3,
			SI1133_MEASCONFIGX_COUNTER_INDEX(0));
	if(err < 0)
	{
		return err;
	}

	return 0;
}

static int si1133_wait_for_sample(struct device *dev, uint8_t mask)
{
	uint8_t regval;
	int err;

	do {
		err = si1133_reg_read(dev, SI1133_REG_IRQ_STATUS, &regval);
		if(err < 0)
		{
			return err;
		}
	} while((regval & mask) != mask);

	return 0;
}

/********************************************
 ******************* API ********************
 ********************************************/

static int si1133_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	int err;

	/* Trigger sample */

	err = si1133_cmd(dev, 0x11);
	if(err < 0)
	{
		return -EIO;
	}

//#ifndef CONFIG_SI1133_TRIGGER

	/* Wait for sample */

	err = si1133_wait_for_sample(dev, 0x0F);
	if(err < 0)
	{
		return -EIO;
	}

	/* Read all samples */

	uint8_t hostout[SI1133_CHANNEL_COUNT * 3];

	err = si1133_burst_read(dev, SI1133_REG_HOSTOUT0, true,
			hostout, 3 * SI1133_CHANNEL_COUNT);
	if(err < 0)
	{
		return err;
	}

	struct si1133_data *data = dev->driver_data;

	for(int i = 0; i < SI1133_CHANNEL_COUNT; i++)
	{
		int j = 3 * i;

		data->channels[i].hostout = (hostout[j] << 16)
				| (hostout[j + 1] << 8) | hostout[j + 2];

		printk("SI1133 HOSTOUT[%d]: 0x%06X\n", i, data->channels[i].hostout);
	}
//#endif

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

	struct si1133_data *data = dev->driver_data;

	LOG_DBG("TST");

	data->i2c_master = device_get_binding(CONFIG_SI1133_I2C_DEV_NAME);
	if(!data->i2c_master)
	{
		LOG_ERR("SI1133: I2C master not found");
		return -EINVAL;
	}

	data->i2c_slave_addr = CONFIG_SI1133_I2C_ADDR;

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

struct si1133_data si1133_data;

DEVICE_AND_API_INIT(si1133, CONFIG_SI1133_DEV_NAME, si1133_init, &si1133_data,
		NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &si1133_api_funcs);
