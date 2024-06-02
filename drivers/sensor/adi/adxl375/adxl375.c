/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl375

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include "zephyr/sys/byteorder.h"
#include <zephyr/sys/__assert.h>
#include <stdlib.h>
#include <string.h>

#include "adxl375.h"

LOG_MODULE_REGISTER(ADXL375, CONFIG_SENSOR_LOG_LEVEL);

static int adxl375_check_id(const struct device *dev)
{
	struct adxl375_data *data = dev->data;
	uint8_t device_id = 0;

	int ret = data->hw_tf->read_reg(dev, ADXL375_DEVID, &device_id);

	if (ret != 0) {
		return ret;
	}

	if (ADXL375_DEVID_VAL != device_id) {
		return -ENODEV;
	}

	return 0;
}

static int adxl375_set_odr_and_lp(const struct device *dev, uint32_t data_rate,
				  const bool low_power)
{
	struct adxl375_data *data = dev->data;

	if (low_power) {
		data_rate |= 0x8;
	}

	return data->hw_tf->write_reg(dev, ADXL375_BW_RATE, data_rate);
}

static int adxl375_set_op_mode(const struct device *dev, enum adxl375_op_mode op_mode)
{
	struct adxl375_data *data = dev->data;

	return data->hw_tf->write_reg(dev, ADXL375_POWER_CTL, op_mode);
}

static int adxl375_set_data_format(const struct device *dev, uint8_t val)
{
	struct adxl375_data *data = dev->data;

	return data->hw_tf->write_reg(dev, ADXL375_DATA_FORMAT, val);
}

static int adxl375_init(const struct device *dev)
{
	int ret;
	const struct adxl375_dev_config *cfg = dev->config;

	ret = cfg->bus_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize sensor bus.");
		return ret;
	}

	ret = adxl375_check_id(dev);
	if (ret < 0) {
		LOG_ERR("Failed to get valid device ID.");
		return ret;
	}

	ret = adxl375_set_odr_and_lp(dev, cfg->odr, cfg->lp);
	if (ret < 0) {
		LOG_ERR("Failed to set ODR and LP mode");
		return ret;
	}

	ret = adxl375_set_data_format(dev, ADXL375_DATA_FORMAT_DEFAULT_BITS);
	if (ret < 0) {
		LOG_ERR("Failed to initialize data format");
		return ret;
	}

	ret = adxl375_set_op_mode(dev, ADXL375_MEASUREMENT);
	if (ret < 0) {
		LOG_ERR("Failed to put in standby mode");
	}

	return 0;
}

static int adxl375_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct adxl375_data *data = dev->data;
	uint8_t buff[6] = {0};

	int ret = data->hw_tf->read_reg_multiple(dev, ADXL375_DATAX0, buff, 6);

	data->sample.x = sys_get_le16(&buff[0]);
	data->sample.y = sys_get_le16(&buff[2]);
	data->sample.z = sys_get_le16(&buff[4]);

	return ret;
}

static void adxl375_accel_convert(struct sensor_value *result, int16_t sample_val)
{
	int64_t value = (int64_t)sample_val * SENSOR_G * ADXL375_MG2G_MULTIPLIER;

	result->val1 = value / 1000000;
	result->val2 = value % 1000000;
}

static int adxl375_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adxl375_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		adxl375_accel_convert(val, data->sample.x);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl375_accel_convert(val, data->sample.y);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl375_accel_convert(val, data->sample.z);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl375_accel_convert(val++, data->sample.x);
		adxl375_accel_convert(val++, data->sample.y);
		adxl375_accel_convert(val, data->sample.z);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api adxl375_api_funcs = {.channel_get = adxl375_channel_get,
							   .sample_fetch = adxl375_sample_fetch};

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "ADXL375 driver enabled without any devices"
#endif

#define ADXL375_DEVICE_INIT(inst)                                                                  \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adxl375_init, NULL, &adxl375_data_##inst,               \
				     &adxl375_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &adxl375_api_funcs);

#define ADXL375_CONFIG(inst)                                                                       \
	.odr = DT_INST_ENUM_IDX(inst, odr), .lp = DT_INST_PROP(inst, lp), .op_mode = ADXL375_STANDBY

/*
 * Instantiation macros used when a device is on a SPI bus.
 */
#define ADXL375_CONFIG_SPI(inst)                                                                   \
	{.bus_init = adxl375_spi_init,                                                             \
	 .spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),                 \
	 ADXL375_CONFIG(inst)}

#define ADXL375_DEFINE_SPI(inst)                                                                   \
	static struct adxl375_data adxl375_data_##inst;                                            \
	static const struct adxl375_dev_config adxl375_config_##inst = ADXL375_CONFIG_SPI(inst);   \
	ADXL375_DEVICE_INIT(inst)

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define ADXL375_CONFIG_I2C(inst)                                                                   \
	{.bus_init = adxl375_i2c_init, .i2c = I2C_DT_SPEC_INST_GET(inst), ADXL375_CONFIG(inst)}

#define ADXL375_DEFINE_I2C(inst)                                                                   \
	static struct adxl375_data adxl375_data_##inst;                                            \
	static const struct adxl375_dev_config adxl375_config_##inst = ADXL375_CONFIG_I2C(inst);   \
	ADXL375_DEVICE_INIT(inst)
/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define ADXL375_DEFINE(inst)                                                                       \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi), (ADXL375_DEFINE_SPI(inst)),                         \
		    (ADXL375_DEFINE_I2C(inst)))

DT_INST_FOREACH_STATUS_OKAY(ADXL375_DEFINE)
