/* sensor_bmp280.c - Driver for Bosch BMP280 temperature and pressure sensor */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <i2c.h>
#include <sensor.h>
#include <init.h>
#include <gpio.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>

#include "sensor_bmp280.h"

#ifndef CONFIG_SENSOR_DEBUG
#define DBG(...) { ; }
#else
#include <misc/printk.h>
#define DBG printk
#endif /* CONFIG_SENSOR_DEBUG */

/*
 * Compensation code taken from BMP280 datasheet, Section 3.11.3
 * "Compensation formula".
 */
static void bmp280_compensate_temp(struct bmp280_data *data, int32_t adc_temp)
{
	int32_t var1, var2;

	var1 = (((adc_temp >> 3) - ((int32_t)data->dig_t1 << 1)) *
		((int32_t)data->dig_t2)) >> 11;
	var2 = (((((adc_temp >> 4) - ((int32_t)data->dig_t1)) *
		  ((adc_temp >> 4) - ((int32_t)data->dig_t1))) >> 12) *
		((int32_t)data->dig_t3)) >> 14;

	data->t_fine = var1 + var2;
	data->comp_temp = (data->t_fine * 5 + 128) >> 8;
}

static void bmp280_compensate_press(struct bmp280_data *data, int32_t adc_press)
{
	int64_t var1, var2, p;

	var1 = ((int64_t)data->t_fine) - 128000;
	var2 = var1 * var1 * (int64_t)data->dig_p6;
	var2 = var2 + ((var1 * (int64_t)data->dig_p5) << 17);
	var2 = var2 + (((int64_t)data->dig_p4) << 35);
	var1 = ((var1 * var1 * (int64_t)data->dig_p3) >> 8) +
		((var1 * (int64_t)data->dig_p2) << 12);
	var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)data->dig_p1) >> 33;

	/* Avoid exception caused by division by zero. */
	if (var1 == 0) {
		data->comp_press = 0;
		return;
	}

	p = 1048576 - adc_press;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = (((int64_t)data->dig_p9) * (p >> 13) * (p >> 13)) >> 25;
	var2 = (((int64_t)data->dig_p8) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + (((int64_t)data->dig_p7) << 4);

	data->comp_press = (uint32_t)p;
}

static int bmp280_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct bmp280_data *data = dev->driver_data;
	uint8_t buf[6];
	int32_t adc_press, adc_temp;
	int ret;

	__ASSERT(chan == SENSOR_CHAN_ALL);

	ret = i2c_burst_read(data->i2c_master, data->i2c_slave_addr,
			     BMP280_REG_PRESS_MSB, buf, sizeof(buf));
	if (ret) {
		return ret;
	}

	adc_press = (buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4);
	adc_temp = (buf[3] << 12) | (buf[4] << 4) | (buf[5] >> 4);

	bmp280_compensate_temp(data, adc_temp);
	bmp280_compensate_press(data, adc_press);

	return 0;
}

static int bmp280_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bmp280_data *data = dev->driver_data;

	switch (chan) {
	case SENSOR_CHAN_TEMP:
		/*
		 * data->comp_temp has a resolution of 0.01 degC.  So
		 * 5123 equals 51.23 degC.
		 */
		val->type = SENSOR_TYPE_INT_PLUS_MICRO;
		val->val1 = data->comp_temp / 100;
		val->val2 = data->comp_temp % 100 * 10000;
		break;
	case SENSOR_CHAN_PRESS:
		/*
		 * data->comp_press has 24 integer bits and 8
		 * fractional.  Output value of 24674867 represents
		 * 24674867/256 = 96386.2 Pa = 963.862 hPa
		 */
		val->type = SENSOR_TYPE_INT_PLUS_MICRO;
		val->val1 = (data->comp_press >> 8) / 1000;
		val->val2 = (data->comp_press >> 8) % 1000 * 1000 +
			(((data->comp_press & 0xff) * 1000) >> 8);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct sensor_driver_api bmp280_api_funcs = {
	.sample_fetch = bmp280_sample_fetch,
	.channel_get = bmp280_channel_get,
};

static void bmp280_read_compensation(struct bmp280_data *data)
{
	uint16_t buf[12];

	i2c_burst_read(data->i2c_master, data->i2c_slave_addr,
		       BMP280_REG_COMP_START, (uint8_t *)buf,
		       sizeof(buf));

	data->dig_t1 = sys_le16_to_cpu(buf[0]);
	data->dig_t2 = sys_le16_to_cpu(buf[1]);
	data->dig_t3 = sys_le16_to_cpu(buf[2]);

	data->dig_p1 = sys_le16_to_cpu(buf[3]);
	data->dig_p2 = sys_le16_to_cpu(buf[4]);
	data->dig_p3 = sys_le16_to_cpu(buf[5]);
	data->dig_p4 = sys_le16_to_cpu(buf[6]);
	data->dig_p5 = sys_le16_to_cpu(buf[7]);
	data->dig_p6 = sys_le16_to_cpu(buf[8]);
	data->dig_p7 = sys_le16_to_cpu(buf[9]);
	data->dig_p8 = sys_le16_to_cpu(buf[10]);
	data->dig_p9 = sys_le16_to_cpu(buf[11]);
}

static int bmp280_chip_init(struct device *dev)
{
	struct bmp280_data *data = (struct bmp280_data *) dev->driver_data;
	uint8_t buf;

	i2c_reg_read_byte(data->i2c_master, data->i2c_slave_addr,
			  BMP280_REG_ID, &buf);
	if (buf != BMP280_CHIP_ID) {
		DBG("bmp280: bad chip id %x\n", buf);
		return -ENOTSUP;
	}

	bmp280_read_compensation(data);
	i2c_reg_write_byte(data->i2c_master, data->i2c_slave_addr,
			   BMP280_REG_CTRL_MEAS, BMP280_CTRL_MEAS_VAL);
	i2c_reg_write_byte(data->i2c_master, data->i2c_slave_addr,
			   BMP280_REG_CONFIG, BMP280_CONFIG_VAL);

	return 0;
}

int bmp280_init(struct device *dev)
{
	struct bmp280_data *data = dev->driver_data;
	int ret;

	data->i2c_master = device_get_binding(CONFIG_BMP280_I2C_MASTER_DEV_NAME);
	if (!data->i2c_master) {
		DBG("bmp280: i2c master not found: %s\n",
		    CONFIG_BMP280_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

	data->i2c_slave_addr = CONFIG_BMP280_I2C_ADDR;

	ret = bmp280_chip_init(dev);
	if (ret) {
		return ret;
	}

	dev->driver_api = &bmp280_api_funcs;

	return 0;
}

static struct bmp280_data bmp280_data;

DEVICE_INIT(bmp280, CONFIG_BMP280_DEV_NAME, bmp280_init, &bmp280_data,
	    NULL, SECONDARY, CONFIG_BMP280_INIT_PRIORITY);
