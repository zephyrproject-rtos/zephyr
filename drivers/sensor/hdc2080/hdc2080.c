/* hdc2080.c - Driver for TI HDC2080 temperature and pressure sensor */

/*
 * Copyright (c) 2020 ZedBlox logitech Pvt Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/sensor.h>
#include <init.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>

#define DT_DRV_COMPAT ti_hdc2080

#define HDC2080_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

#if HDC2080_BUS_I2C
#include <drivers/i2c.h>
#endif
#include <logging/log.h>

#include "hdc2080.h"

#define HDC2080_CONFIG_VAL 0x00

LOG_MODULE_REGISTER(HDC2080, CONFIG_SENSOR_LOG_LEVEL);

struct hdc2080_data {
#if HDC2080_BUS_I2C
	struct device *i2c_master;
	u16_t i2c_slave_addr;
#else
#error "HDC2080 device type not specified"
#endif
	/* Compensated values. */
	s32_t comp_temp;
	u32_t comp_humidity;

	u16_t chip_id;
};

static int hdc2080_reg_read(struct hdc2080_data *data,
			  u8_t start, u8_t *buf, int size)
{
	return i2c_burst_read(data->i2c_master, data->i2c_slave_addr,
			      start, buf, size);
	return 0;
}

static int hdc2080_reg_write(struct hdc2080_data *data, u8_t reg, u8_t val)
{
	return i2c_reg_write_byte(data->i2c_master, data->i2c_slave_addr,
				  reg, val);
	return 0;
}

static int hdc2080_setrate(struct hdc2080_data *data, u8_t val)
{
    int err;

    err = hdc2080_reg_read(data, CONFIG, &val, 1);
    if (err < 0)
    {
        return err;
    }

    /* FIXME Hack */
    val = (val & 0xF1) | (AMM_MODE_ONE_HZ << AMM_MODE_OFFSET);

    err = hdc2080_reg_write(data, CONFIG, val);
    if (err < 0)
    {
        return err;
    }

    return err;
}

static int hdc2080_set_config(struct hdc2080_data *data, u8_t val)
{
    int err;

    err = hdc2080_reg_write(data, MEASUREMENT_CONFIG, val);

    return err;
}

static int hdc2080_trigger_measurement(struct hdc2080_data *data)
{
    u8_t val;
    int err;

    err = hdc2080_reg_read(data, MEASUREMENT_CONFIG, &val, 1);
    if (err < 0)
    {
        return err;
    }

    val |= 0x1;

    err = hdc2080_reg_write(data, MEASUREMENT_CONFIG, val);

    return err;
}

static int hdc2080_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct hdc2080_data *data = dev->driver_data;
	u8_t buf[4];
    int err;

	if (data->chip_id == HDC2080_CHIP_ID) {
        err = hdc2080_trigger_measurement(data);
		if (err < 0) {
			return err;
		}

		err = hdc2080_reg_read(data, TEMP_LOW,
				     (u8_t *) buf, sizeof(buf));
		if (err < 0) {
			return err;
		}
	}

    //LOG_INF("%d, %d, %d, %d", buf[0], buf[1], buf[2], buf[3]);
    //float t = (float)((float)((buf[1] << 8) | buf[0]) * (float)165 * (float)100) / (float)65536 - (float)4000;
    //float h = (float)((float)((buf[3] << 8) | buf[2]) * (float)100) / (float)65536 * 100;
	data->comp_temp = ((((buf[1] << 8) | buf[0]) * 165 * 100) / 65536) - 4000;
	data->comp_humidity = ((((buf[3] << 8) | buf[2]) * 100) / 65536) * 100;

	return 0;
}

static int hdc2080_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct hdc2080_data *data = dev->driver_data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/*
		 * data->comp_temp has a resolution of 0.01 degC.  So
		 * 5123 equals 51.23 degC.
		 */
		val->val1 = data->comp_temp / 100;
		val->val2 = data->comp_temp % 100 * 10000;
		break;
	case SENSOR_CHAN_HUMIDITY:
		/*
		 */
		val->val1 = data->comp_humidity / 100;
		val->val2 = data->comp_humidity % 100 * 10000;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api hdc2080_api_funcs = {
	.sample_fetch = hdc2080_sample_fetch,
	.channel_get = hdc2080_channel_get,
};

static int hdc2080_chip_init(struct device *dev)
{
	struct hdc2080_data *data = (struct hdc2080_data *) dev->driver_data;
	int err;

	err = hdc2080_reg_read(data, DEVICE_ID_L, (u8_t *) &data->chip_id, 2);
	if (err < 0) {
        LOG_INF("err: %d", err);
		return err;
	}

	if (data->chip_id == HDC2080_CHIP_ID) {
		LOG_INF("HDC2080 chip detected");
	} else {
		LOG_INF("bad chip id 0x%x", data->chip_id);
		return -ENOTSUP;
	}

    //err = hdc2080_setrate(data, 0);
	if (err < 0) {
		return err;
	}

	err = hdc2080_set_config(data, HDC2080_CONFIG_VAL);
	if (err < 0) {
		return err;
	}

    err = hdc2080_trigger_measurement(data);
    if (err < 0) {
        return err;
    }

	return 0;
}

#define HDC2080_DEVICE(inst)				                                \
static int hdc2080_##inst##_init(struct device *dev)			            \
{                                                                           \
	struct hdc2080_data *data = dev->driver_data;                           \
	data->i2c_master = device_get_binding(DT_INST_BUS_LABEL(inst));         \
	if (!data->i2c_master) {                                                \
		LOG_INF("i2c master not found: %s",                                 \
			    DT_INST_BUS_LABEL(inst));                                   \
		return -EINVAL;                                                     \
	}                                                                       \
	data->i2c_slave_addr = DT_INST_REG_ADDR(inst);                          \
	if (hdc2080_chip_init(dev) < 0) {                                       \
		return -EINVAL;                                                     \
	}                                                                       \
    return 0;                                                               \
}                                                                           \
                                                                            \
static struct hdc2080_data hdc2080_##inst##_data;                           \
                                                                            \
DEVICE_AND_API_INIT(hdc2080_##inst,				                            \
          DT_INST_LABEL(inst),		                                        \
          hdc2080_##inst##_init,					                        \
          &hdc2080_##inst##_data,				                            \
          NULL,              				                                \
          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,		                    \
          &hdc2080_api_funcs);


DT_INST_FOREACH_STATUS_OKAY(HDC2080_DEVICE)
