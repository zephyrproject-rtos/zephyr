/*
 * Copyright (c) 2021 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_shtcx

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/logging/log.h>

#include "shtcx.h"

LOG_MODULE_REGISTER(SHTCX, CONFIG_SENSOR_LOG_LEVEL);

/* all cmds read temp first: cmd[MEASURE_MODE][Clock_stretching_enabled] */
static const uint16_t measure_cmd[2][2] = {
	{ 0x7866, 0x7CA2 },
	{ 0x609C, 0x6458 },
};

/* measure_wait_us[shtcx_chip][MEASURE_MODE] */
static const uint16_t measure_wait_us[2][2] = {
	/* shtc1: 14.4ms, 0.94ms */
	{ 14400, 940 }, /* shtc1 */
	/* shtc3: 12.1ms, 0.8ms */
	{ 12100, 800 }, /* shtc3 */
};

/*
 * CRC algorithm parameters were taken from the
 * "Checksum Calculation" section of the datasheet.
 */
static uint8_t shtcx_compute_crc(uint16_t value)
{
	uint8_t buf[2];

	sys_put_be16(value, buf);
	return crc8(buf, 2, 0x31, 0xFF, false);
}

/* val = -45 + 175 * sample / (2^16) */
static void shtcx_temperature_from_raw(uint16_t raw, struct sensor_value *val)
{
	int32_t tmp;

	tmp = (int32_t)raw * 175U - (45 << 16);
	val->val1 = tmp / 0x10000;
	/* x * 1.000.000 / 65.536 == x * 15625 / 2^10 */
	val->val2 = ((tmp % 0x10000) * 15625) / 1024;
}

/* val = 100 * sample / (2^16) */
static void shtcx_humidity_from_raw(uint16_t raw, struct sensor_value *val)
{
	uint32_t tmp;

	tmp = (uint32_t)raw * 100U;
	val->val1 = tmp / 0x10000;
	/* x * 1.000.000 / 65.536 == x * 15625 / 1024 */
	val->val2 = (tmp % 0x10000) * 15625U / 1024;
}

static int shtcx_write_command(const struct device *dev, uint16_t cmd)
{
	const struct shtcx_config *cfg = dev->config;
	uint8_t tx_buf[2];

	sys_put_be16(cmd, tx_buf);
	return i2c_write_dt(&cfg->i2c, tx_buf, sizeof(tx_buf));
}

static int shtcx_read_words(const struct device *dev, uint16_t cmd, uint16_t *data,
		     uint16_t num_words, uint16_t max_duration_us)
{
	const struct shtcx_config *cfg = dev->config;
	int status = 0;
	uint32_t raw_len = num_words * (SHTCX_WORD_LEN + SHTCX_CRC8_LEN);
	uint16_t temp16;
	uint8_t rx_buf[SHTCX_MAX_READ_LEN];
	int dst = 0;

	status = shtcx_write_command(dev, cmd);
	if (status != 0) {
		LOG_DBG("Failed to initiate read");
		return -EIO;
	}

	if (!cfg->clock_stretching) {
		k_sleep(K_USEC(max_duration_us));
	}

	status = i2c_read_dt(&cfg->i2c, rx_buf, raw_len);
	if (status != 0) {
		LOG_DBG("Failed to read data");
		return -EIO;
	}

	for (int i = 0; i < raw_len; i += (SHTCX_WORD_LEN + SHTCX_CRC8_LEN)) {
		temp16 = sys_get_be16(&rx_buf[i]);
		if (shtcx_compute_crc(temp16) != rx_buf[i+2]) {
			LOG_DBG("invalid received invalid crc");
			return -EIO;
		}

		data[dst++] = temp16;
	}

	return 0;
}

static int shtcx_sleep(const struct device *dev)
{
	if (shtcx_write_command(dev, SHTCX_CMD_SLEEP) < 0) {
		return -EIO;
	}

	return 0;
}

static int shtcx_wakeup(const struct device *dev)
{
	if (shtcx_write_command(dev, SHTCX_CMD_WAKEUP)) {
		return -EIO;
	}

	k_sleep(K_USEC(100));
	return 0;
}

static int shtcx_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct shtcx_data *data = dev->data;
	const struct shtcx_config *cfg = dev->config;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (cfg->chip == SHTC3) {
		if (shtcx_wakeup(dev)) {
			return -EIO;
		}
	}

	if (shtcx_read_words(dev,
			     measure_cmd[cfg->measure_mode][cfg->clock_stretching],
			     (uint16_t *)&data->sample, 2,
			     measure_wait_us[cfg->chip][cfg->measure_mode]) < 0) {
		LOG_DBG("Failed read measurements!");
		return -EIO;
	}

	if (cfg->chip == SHTC3) {
		if (shtcx_sleep(dev)) {
			LOG_DBG("Failed to initiate sleep");
			return -EIO;
		}
	}

	return 0;
}

static int shtcx_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct shtcx_data *data = dev->data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		shtcx_temperature_from_raw(data->sample.temp, val);
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		shtcx_humidity_from_raw(data->sample.humidity, val);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api shtcx_driver_api = {
	.sample_fetch = shtcx_sample_fetch,
	.channel_get = shtcx_channel_get,
};

static int shtcx_init(const struct device *dev)
{
	const struct shtcx_config *cfg = dev->config;
	uint16_t product_id;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	k_sleep(K_USEC(SHTCX_POWER_UP_TIME_US));
	if (cfg->chip == SHTC3) {
		if (shtcx_wakeup(dev)) {
			LOG_ERR("Wakeup failed");
			return -EIO;
		}
	}

	if (shtcx_write_command(dev, SHTCX_CMD_SOFT_RESET) < 0) {
		LOG_ERR("soft reset failed");
		return -EIO;
	}

	k_sleep(K_USEC(SHTCX_SOFT_RESET_TIME_US));
	if (shtcx_read_words(dev, SHTCX_CMD_READ_ID, &product_id, 1, 0) < 0) {
		LOG_ERR("Failed to read product id!");
		return -EIO;
	}

	if (cfg->chip == SHTC1) {
		if ((product_id & SHTC1_ID_MASK) != SHTC1_ID_VALUE) {
			LOG_ERR("Device is not a SHTC1");
			return -EINVAL;
		}
	}
	if (cfg->chip == SHTC3) {
		if ((product_id & SHTC3_ID_MASK) != SHTC3_ID_VALUE) {
			LOG_ERR("Device is not a SHTC3");
			return -EINVAL;
		}
		shtcx_sleep(dev);
	}

	LOG_DBG("Clock-stretching enabled: %d", cfg->clock_stretching);
	LOG_DBG("Measurement mode: %d", cfg->measure_mode);
	LOG_DBG("Init SHTCX");
	return 0;
}

#define SHTCX_CHIP(inst) \
	(DT_INST_NODE_HAS_COMPAT(inst, sensirion_shtc1) ? CHIP_SHTC1 : CHIP_SHTC3)

#define SHTCX_CONFIG(inst)						       \
	{								       \
		.i2c = I2C_DT_SPEC_INST_GET(inst),			       \
		.chip = SHTCX_CHIP(inst),				       \
		.measure_mode = DT_INST_ENUM_IDX(inst, measure_mode),	       \
		.clock_stretching = DT_INST_PROP(inst, clock_stretching)       \
	}

#define SHTCX_DEFINE(inst)						\
	static struct shtcx_data shtcx_data_##inst;			\
	static struct shtcx_config shtcx_config_##inst =		\
		SHTCX_CONFIG(inst);					\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			      shtcx_init,				\
			      NULL,					\
			      &shtcx_data_##inst,			\
			      &shtcx_config_##inst,			\
			      POST_KERNEL,				\
			      CONFIG_SENSOR_INIT_PRIORITY,		\
			      &shtcx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SHTCX_DEFINE)
