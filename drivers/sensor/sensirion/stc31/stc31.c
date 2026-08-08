/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_stc31

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/stc31.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include "stc31.h"

LOG_MODULE_REGISTER(STC31, CONFIG_SENSOR_LOG_LEVEL);

static uint8_t stc31_crc(const uint8_t *data, size_t len)
{
	return crc8(data, len, STC31_CRC_POLY, STC31_CRC_INIT, false);
}

static int stc31_write_cmd(const struct device *dev, uint16_t cmd)
{
	const struct stc31_config *cfg = dev->config;
	uint8_t buf[2];

	sys_put_be16(cmd, buf);
	return i2c_write_dt(&cfg->bus, buf, sizeof(buf));
}

static int stc31_write_cmd_arg(const struct device *dev, uint16_t cmd, uint16_t arg)
{
	const struct stc31_config *cfg = dev->config;
	uint8_t buf[5];

	sys_put_be16(cmd, buf);
	sys_put_be16(arg, &buf[2]);
	buf[4] = stc31_crc(&buf[2], 2);

	return i2c_write_dt(&cfg->bus, buf, sizeof(buf));
}

static int stc31_read_words(const struct device *dev, uint8_t *rx_buf, size_t rx_len)
{
	const struct stc31_config *cfg = dev->config;
	int ret;

	ret = i2c_read_dt(&cfg->bus, rx_buf, rx_len);
	if (ret < 0) {
		LOG_ERR("Failed to read I2C data (%d)", ret);
		return ret;
	}

	for (size_t i = 0; i < rx_len; i += STC31_WORD_SIZE) {
		if (stc31_crc(&rx_buf[i], 2) != rx_buf[i + 2]) {
			LOG_ERR("Invalid CRC");
			return -EIO;
		}
	}

	return 0;
}

static uint16_t stc31_measure_delay_ms(uint16_t binary_gas)
{
	/* Low-cross-sensitivity modes use arguments 0x0010-0x0013 */
	if (binary_gas >= STC31_DT_BINARY_GAS_CO2_IN_N2_100) {
		return STC31_MEASURE_DURATION_LOW_XSENS_MS;
	}

	return STC31_MEASURE_DURATION_LN_MS;
}

/* C[vol%] = 100 * (raw - 2^14) / 2^15 */
static float stc31_raw_to_vol_percent(uint16_t raw)
{
	return (100.0f * ((float)raw - 16384.0f)) / 32768.0f;
}

static uint16_t stc31_vol_percent_to_raw(float vol_percent)
{
	float raw = (vol_percent / 100.0f) * 32768.0f + 16384.0f;

	if (raw < 0.0f) {
		return 0;
	}
	if (raw > (float)UINT16_MAX) {
		return UINT16_MAX;
	}

	return (uint16_t)(raw + 0.5f);
}

static uint16_t stc31_rh_to_raw(const struct sensor_value *val)
{
	float rh = sensor_value_to_float(val);

	if (rh < 0.0f) {
		rh = 0.0f;
	} else if (rh > 100.0f) {
		rh = 100.0f;
	}

	return (uint16_t)((rh * 65535.0f) / 100.0f + 0.5f);
}

static int16_t stc31_temp_to_raw(const struct sensor_value *val)
{
	float t_c = sensor_value_to_float(val);

	return (int16_t)(t_c * 200.0f);
}

static int stc31_set_binary_gas(const struct device *dev, uint16_t binary_gas)
{
	struct stc31_data *data = dev->data;
	int ret;

	ret = stc31_write_cmd_arg(dev, STC31_CMD_SET_BINARY_GAS, binary_gas);
	if (ret < 0) {
		return ret;
	}

	/* Sensirion embedded driver waits 1 ms after set_binary_gas */
	k_msleep(1);
	data->binary_gas = binary_gas;
	return 0;
}

static int stc31_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct stc31_data *data = dev->data;
	uint8_t rx[6];
	int16_t temp_raw;
	uint16_t conc_raw;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_CO2 &&
	    chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	ret = stc31_write_cmd(dev, STC31_CMD_MEASURE_GAS_CONCENTRATION);
	if (ret < 0) {
		LOG_ERR("Measure command failed (%d)", ret);
		return ret;
	}

	k_msleep(stc31_measure_delay_ms(data->binary_gas));

	/* Concentration + temperature (2 words); reserved word is optional */
	ret = stc31_read_words(dev, rx, sizeof(rx));
	if (ret < 0) {
		return ret;
	}

	conc_raw = sys_get_be16(&rx[0]);
	temp_raw = (int16_t)sys_get_be16(&rx[3]);

	/* SENSOR_CHAN_CO2 is ppm; 1 vol% = 10000 ppm */
	data->co2_ppm = stc31_raw_to_vol_percent(conc_raw) * 10000.0f;
	data->temp_c = (float)temp_raw / 200.0f;

	return 0;
}

static int stc31_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	const struct stc31_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_CO2:
		return sensor_value_from_float(val, data->co2_ppm);
	case SENSOR_CHAN_AMBIENT_TEMP:
		return sensor_value_from_float(val, data->temp_c);
	default:
		return -ENOTSUP;
	}
}

static int stc31_attr_set(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, const struct sensor_value *val)
{
	ARG_UNUSED(chan);

	switch ((enum sensor_attribute_stc31)attr) {
	case SENSOR_ATTR_STC31_REL_HUMIDITY:
		return stc31_write_cmd_arg(dev, STC31_CMD_SET_RELATIVE_HUMIDITY,
					   stc31_rh_to_raw(val));
	case SENSOR_ATTR_STC31_TEMPERATURE:
		return stc31_write_cmd_arg(dev, STC31_CMD_SET_TEMPERATURE,
					   (uint16_t)stc31_temp_to_raw(val));
	case SENSOR_ATTR_STC31_PRESSURE:
		if (val->val1 < 0 || val->val1 > UINT16_MAX) {
			return -EINVAL;
		}
		return stc31_write_cmd_arg(dev, STC31_CMD_SET_PRESSURE, (uint16_t)val->val1);
	case SENSOR_ATTR_STC31_BINARY_GAS:
		if (val->val1 < 0 || val->val1 > UINT16_MAX) {
			return -EINVAL;
		}
		return stc31_set_binary_gas(dev, (uint16_t)val->val1);
	default:
		return -ENOTSUP;
	}
}

static int stc31_attr_get(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, struct sensor_value *val)
{
	const struct stc31_data *data = dev->data;

	ARG_UNUSED(chan);

	switch ((enum sensor_attribute_stc31)attr) {
	case SENSOR_ATTR_STC31_BINARY_GAS:
		val->val1 = data->binary_gas;
		val->val2 = 0;
		return 0;
	default:
		return -ENOTSUP;
	}
}

int stc31_forced_recalibration(const struct device *dev, uint16_t reference_ppm)
{
	struct stc31_data *data = dev->data;
	float vol_percent = (float)reference_ppm / 10000.0f;
	uint16_t raw = stc31_vol_percent_to_raw(vol_percent);
	int ret;

	ret = stc31_write_cmd_arg(dev, STC31_CMD_FORCED_RECALIBRATION, raw);
	if (ret < 0) {
		return ret;
	}

	/* FRC takes the same time as a concentration measurement */
	k_msleep(stc31_measure_delay_ms(data->binary_gas));
	return 0;
}

static int stc31_soft_reset_hw(const struct device *dev)
{
	const struct stc31_config *cfg = dev->config;
	uint8_t cmd = STC31_SOFT_RESET_CMD;
	int ret;

	ret = i2c_write(cfg->bus.bus, &cmd, sizeof(cmd), STC31_GENERAL_CALL_ADDR);
	if (ret < 0) {
		return ret;
	}

	k_msleep(STC31_SOFT_RESET_TIME_MS);
	return 0;
}

int stc31_soft_reset(const struct device *dev)
{
	struct stc31_data *data = dev->data;
	int ret;

	ret = stc31_soft_reset_hw(dev);
	if (ret < 0) {
		return ret;
	}

	/* Soft reset clears binary gas selection */
	return stc31_set_binary_gas(dev, data->binary_gas);
}

static int stc31_read_product_id(const struct device *dev, uint32_t *product)
{
	uint8_t rx[18];
	int ret;

	ret = stc31_write_cmd(dev, STC31_CMD_READ_PRODUCT_ID_1);
	if (ret < 0) {
		LOG_ERR("Product ID prepare failed (%d)", ret);
		return ret;
	}

	ret = stc31_write_cmd(dev, STC31_CMD_READ_PRODUCT_ID_2);
	if (ret < 0) {
		LOG_ERR("Product ID read cmd failed (%d)", ret);
		return ret;
	}

	/* Sensirion embedded driver waits 10 ms before reading the response */
	k_msleep(10);

	/* 6 words: 32-bit product + 64-bit serial (with CRC bytes) */
	ret = stc31_read_words(dev, rx, sizeof(rx));
	if (ret < 0) {
		LOG_ERR("Product ID data read failed (%d)", ret);
		return ret;
	}

	*product = ((uint32_t)sys_get_be16(&rx[0]) << 16) | sys_get_be16(&rx[3]);
	return 0;
}

static int stc31_init(const struct device *dev)
{
	const struct stc31_config *cfg = dev->config;
	uint32_t product;
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	k_msleep(STC31_POWER_UP_TIME_MS);

	/* General-call reset; ignore error if bus has no listeners yet */
	(void)stc31_soft_reset_hw(dev);

	ret = stc31_read_product_id(dev, &product);
	if (ret < 0) {
		LOG_ERR("Failed to read product ID (%d)", ret);
		return ret;
	}

	if ((product & STC31_PRODUCT_MASK) != STC31_PRODUCT_NUMBER) {
		LOG_ERR("Unexpected product ID 0x%08x", product);
		return -ENODEV;
	}

	LOG_DBG("Product ID: 0x%08x", product);

	ret = stc31_set_binary_gas(dev, cfg->binary_gas);
	if (ret < 0) {
		LOG_ERR("Failed to set binary gas 0x%04x (%d)", cfg->binary_gas, ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, stc31_api) = {
	.sample_fetch = stc31_sample_fetch,
	.channel_get = stc31_channel_get,
	.attr_set = stc31_attr_set,
	.attr_get = stc31_attr_get,
};

#define STC31_INIT(inst)                                                                           \
	static struct stc31_data stc31_data_##inst;                                                \
	static const struct stc31_config stc31_config_##inst = {                                   \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.binary_gas = DT_INST_PROP(inst, binary_gas),                                      \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, stc31_init, NULL, &stc31_data_##inst,                   \
				     &stc31_config_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &stc31_api);

DT_INST_FOREACH_STATUS_OKAY(STC31_INIT)
