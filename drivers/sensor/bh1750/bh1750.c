/* Rohm BH1750 16 bit ambient light sensor
 *
 * Copyright (c) 2022 Lucas Heitzmann Gabrielli
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rohm_bh1750

#include <logging/log.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>

#include "bh1750.h"

LOG_MODULE_REGISTER(BH1750, CONFIG_SENSOR_LOG_LEVEL);

static int bh1750_read(const struct device *dev, uint8_t *byte, uint32_t count)
{
	const struct bh1750_config *cfg = dev->config;
	return i2c_read(cfg->bus, byte, count, cfg->bus_addr);
}

static int bh1750_write(const struct device *dev, uint8_t byte)
{
	const struct bh1750_config *cfg = dev->config;
	return i2c_write(cfg->bus, &byte, 1, cfg->bus_addr);
}

static int bh1750_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	int ret;
	uint8_t byte;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_LIGHT || chan == SENSOR_CHAN_ALL);
	__ASSERT_NO_MSG(attr == SENSOR_ATTR_OVERSAMPLING);

	if (val->val1 < 31 || val->val1 > 254) {
		LOG_ERR("Oversampling factor out of range [31, 254]: %d", val->val1);
		return -EINVAL;
	}

	byte = BH1750_MEAS_TIME_MSB | (((uint8_t)val->val1 >> 5) & 0x07);
	ret = bh1750_write(dev, byte);
	if (ret < 0) {
		LOG_ERR("Failed to configure high part of oversampling factor.");
		return ret;
	}

	byte = BH1750_MEAS_TIME_LSB | ((uint8_t)val->val1 & 0x1F);
	ret = bh1750_write(dev, byte);
	if (ret < 0) {
		LOG_ERR("Failed to configure low part of oversampling factor.");
		return ret;
	}

	struct bh1750_data *data = dev->data;
	data->oversampling_factor = (uint8_t)val->val1;

	return 0;
}

static int bh1750_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_LIGHT || chan == SENSOR_CHAN_ALL);
	__ASSERT_NO_MSG(attr == SENSOR_ATTR_OVERSAMPLING);

	struct bh1750_data *data = dev->data;
	val->val1 = (int32_t)data->oversampling_factor;

	return 0;
}

static int bh1750_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bh1750_data *data = dev->data;
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_LIGHT);

	ret = bh1750_write(dev, BH1750_SINGLE_MEASUREMENT);
	if (ret < 0) {
		LOG_ERR("Unable to issue measurement command.");
		return ret;
	}

	k_busy_wait(BH1750_TIME_FACTOR * (uint32_t)data->oversampling_factor);

	uint8_t buffer[2];
	ret = bh1750_read(dev, buffer, 2);
	if (ret < 0) {
		LOG_ERR("Unable to issue read command.");
		return ret;
	}
	data->adc_count = ((uint16_t)buffer[0] << 8) | (uint16_t)buffer[1];

	return 0;
}

static int bh1750_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_LIGHT);
	struct bh1750_data *data = dev->data;
	uint32_t tmp = (uint32_t)data->adc_count * (230 << 8) / data->oversampling_factor;
#if defined(CONFIG_BH1750_HIGH_RES)
	/* tmp = lux * 2^11 */
	val->val1 = (int32_t)(tmp >> 11);
	val->val2 = (int32_t)((tmp & 0x07FF) * 15625) >> 5;
#else
	/* tmp = lux * 2^10 */
	val->val1 = (int32_t)(tmp >> 10);
	val->val2 = (int32_t)((tmp & 0x03FF) * 15625) >> 4;
#endif
	return 0;
}

static const struct sensor_driver_api bh1750_api = {
	.attr_set = bh1750_attr_set,
	.attr_get = bh1750_attr_get,
	.sample_fetch = bh1750_sample_fetch,
	.channel_get = bh1750_channel_get,
};

static int bh1750_init(const struct device *dev)
{
	struct bh1750_data *data = dev->data;
	const struct bh1750_config *cfg = dev->config;

	if (!device_is_ready(cfg->bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	struct sensor_value tmp = { .val1 = data->oversampling_factor, .val2 = 0 };
	return bh1750_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_OVERSAMPLING, &tmp);
}

#define BH1750_INST(inst)                                                                          \
	static const struct bh1750_config bh1750_config_##inst = {                                 \
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),                                           \
		.bus_addr = DT_INST_REG_ADDR(inst),                                                \
	};                                                                                         \
	static struct bh1750_data bh1750_data_##inst = {                                           \
		.adc_count = 0,                                                                    \
		.oversampling_factor = DT_INST_PROP(inst, oversampling_factor),                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, bh1750_init, NULL, &bh1750_data_##inst, &bh1750_config_##inst, \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &bh1750_api);

DT_INST_FOREACH_STATUS_OKAY(BH1750_INST)
