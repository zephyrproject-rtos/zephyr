/* Driver for TI hdc20x0
 * hdc20x0 ±2% digital humidity sensor with temperature sensor */

/*
 * Copyright (c) 2020 seeed.cc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_hdc20x0

#include <kernel.h>
#include <drivers/sensor.h>
#include <init.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include <drivers/i2c.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(hdc20x0, CONFIG_SENSOR_LOG_LEVEL);

/* Register addresses as in the datasheet */
#define hdc20x0_REG_TEMP_LOW 0x00
#define hdc20x0_REG_TEMP_HIGH 0x01
#define hdc20x0_REG_HUMIDITY_LOW 0x02
#define hdc20x0_REG_HUMIDITY_HIGH 0x03
#define hdc20x0_REG_INTERRUPT_DRDY 0x04
#define hdc20x0_REG_TEMP_MAX 0x05
#define hdc20x0_REG_HUMIDITY_MAX 0x06
#define hdc20x0_REG_INTERRUPT_EN 0x07
#define hdc20x0_REG_TEMP_OFFSET_ADJ 0x08
#define hdc20x0_REG_HUMIDITY_OFFSET_ADJ 0x09
#define hdc20x0_REG_TEMP_THR_L 0x0a
#define hdc20x0_REG_TEMP_THR_H 0x0b
#define hdc20x0_REG_RH_THR_L 0x0c
#define hdc20x0_REG_RH_THR_H 0x0d
#define hdc20x0_REG_RESET_DRDY_INT_CONF 0x0e
#define hdc20x0_REG_CONF 0x0f

#define hdc20x0_MEAS_CONF GENMASK(2, 1)
#define hdc20x0_MEAS_TRIG BIT(0)
#define hdc20x0_HEATER_EN BIT(3)
#define hdc20x0_AMM GENMASK(6, 4)

struct hdc20x0_data {
	const struct device *i2c_master;

	/* Temperature Values */
	int32_t temp_val1;
	int32_t temp_val2;
	/* Humidity Values*/
	int32_t humidity_val1;
	int32_t humidity_val2;
};

struct hdc20x0_cfg {
	char *i2c_bus_name;
	uint16_t i2c_addr;
};

static int hdc20x0_read_temperature(struct hdc20x0_data *data,
				    const struct hdc20x0_cfg *config)
{
	uint8_t val;
	float celsius;

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_addr,
			      hdc20x0_REG_TEMP_LOW, &val)) {
		LOG_ERR("Failed to get temperature LSB ");
		return -EIO;
	}
	data->temp_val1 = val;
	if (i2c_reg_read_byte(data->i2c_master, config->i2c_addr,
			      hdc20x0_REG_TEMP_HIGH, &val)) {
		LOG_ERR("Failed to get temperature MSB ");
		return -EIO;
	}
	data->temp_val2 = val;
	LOG_DBG("temperature,  val1:0x%x, val2:0x%x", data->temp_val1,
		data->temp_val2);

	/*https://www.ti.com/lit/ds/symlink/hdc20x0.pdf*/
	/*7.6.2 Address 0x01 Temperature MSB*/
	celsius = ((data->temp_val2 * 256 + data->temp_val1) / 65536.0) * 165 -
		  40;
	data->temp_val1 = (int)celsius;
	data->temp_val2 = (celsius - data->temp_val1) * 1000000LL;
	return 0;
}

static int hdc20x0_read_humidity(struct hdc20x0_data *data,
				 const struct hdc20x0_cfg *config)
{
	uint8_t val;
	float rh;

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_addr,
			      hdc20x0_REG_HUMIDITY_LOW, &val)) {
		LOG_ERR("Failed to get humidity LSB ");
		return -EIO;
	}
	data->humidity_val1 = val;
	if (i2c_reg_read_byte(data->i2c_master, config->i2c_addr,
			      hdc20x0_REG_HUMIDITY_HIGH, &val)) {
		LOG_ERR("Failed to get humidity MSB ");
		return -EIO;
	}
	data->humidity_val2 = val;
	LOG_DBG("humidity , val1:0x%x, val2:0x%x", data->humidity_val1,
		data->humidity_val2);
	/*https://www.ti.com/lit/ds/symlink/hdc20x0.pdf*/
	/*7.6.4 Address 0x03 Humidity MSB*/
	rh = ((data->humidity_val2 * 256 + data->humidity_val1) / 65536.0) *
	     100;

	data->humidity_val1 = (int)rh;
	data->humidity_val2 = (rh - data->humidity_val1) * 1000000LL;
	return 0;
}

static int hdc20x0_read_all(struct hdc20x0_data *data,
			    const struct hdc20x0_cfg *config)
{
	if (hdc20x0_read_temperature(data, config)) {
		return -EIO;
	}
	if (hdc20x0_read_humidity(data, config)) {
		return -EIO;
	}
	return 0;
}

static int hdc20x0_init(const struct device *dev)
{
	struct hdc20x0_data *data = dev->data;
	const struct hdc20x0_cfg *config = dev->config;
	int ret;

	data->i2c_master = device_get_binding(config->i2c_bus_name);
	if (data->i2c_master == NULL) {
		LOG_ERR("Failed to get I2C device");
		return -EINVAL;
	}

	LOG_DBG("Init hdc20x0");

	/* Enable Automatic Measurement Mode at 5Hz */
	if (i2c_reg_write_byte(data->i2c_master, config->i2c_addr,
			       hdc20x0_REG_RESET_DRDY_INT_CONF, hdc20x0_AMM)) {
		return -EIO;
	}
	/*
	 * We enable both temp and humidity measurement.
	 * However the measurement won't start even in AMM until triggered.
	 */
	ret = i2c_reg_write_byte(data->i2c_master, config->i2c_addr,
				 hdc20x0_REG_CONF, hdc20x0_MEAS_TRIG);
	if (ret) {
		LOG_ERR("Unable to set up measurement\n");
		return ret;
	}

	LOG_DBG("Init OK");
	return 0;
}

static int hdc20x0_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct hdc20x0_data *data = dev->data;
	const struct hdc20x0_cfg *config = dev->config;

	LOG_DBG("Fetching sample from hdc20x0");

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		if (hdc20x0_read_temperature(data, config)) {
			LOG_ERR("Failed to measure temperature");
			return -EIO;
		}
		break;
	case SENSOR_CHAN_HUMIDITY:
		if (hdc20x0_read_humidity(data, config)) {
			LOG_ERR("Failed to measure humidity");
			return -EIO;
		}
		break;
	case SENSOR_CHAN_ALL:
		if (hdc20x0_read_all(data, config)) {
			LOG_ERR("Failed to measure temperature and humidity");
			return -EIO;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int hdc20x0_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct hdc20x0_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		val->val1 = data->temp_val1;
		val->val2 = data->temp_val2;
		break;
	case SENSOR_CHAN_HUMIDITY:
		val->val1 = data->humidity_val1;
		val->val2 = data->humidity_val2;
		break;
	case SENSOR_CHAN_ALL:
		val->val1 = data->temp_val1;
		val->val2 = data->temp_val2;
		(val + 1)->val1 = data->humidity_val1;
		(val + 1)->val2 = data->humidity_val2;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api hdc20x0_api_funcs = {
	.sample_fetch = hdc20x0_sample_fetch,
	.channel_get = hdc20x0_channel_get,
};

static struct hdc20x0_data hdc20x0_data_0;
static const struct hdc20x0_cfg hdc20x0_cfg_0 = { .i2c_bus_name =
							  DT_INST_BUS_LABEL(0),
						  .i2c_addr =
							  DT_INST_REG_ADDR(0) };

DEVICE_AND_API_INIT(hdc20x0, DT_INST_LABEL(0), hdc20x0_init, &hdc20x0_data_0,
		    &hdc20x0_cfg_0, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &hdc20x0_api_funcs);
