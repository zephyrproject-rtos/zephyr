/*
 * Copyright (c) 2026 Shi Weizhi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>

#include <zephyr/drivers/sensor/sps30.h>
#include "sps30_internal.h"

LOG_MODULE_REGISTER(SPS30, CONFIG_SENSOR_LOG_LEVEL);

/* CRC-8 for 2-byte packet (datasheet §6.2) */
static uint8_t sps30_crc8(const uint8_t data[2])
{
	return crc8(data, 2, SPS30_CRC_POLY, SPS30_CRC_INIT, false);
}

/* Set 16-bit pointer address (datasheet §6.1) */
static int sps30_set_pointer(const struct device *dev, uint16_t pointer)
{
	const struct sps30_config *cfg = dev->config;
	uint8_t buf[2];

	sys_put_be16(pointer, buf);

	return i2c_write_dt(&cfg->bus, buf, sizeof(buf));
}

/* Set pointer and read one 16-bit word with CRC validation (datasheet §6.1) */
static int sps30_read_word(const struct device *dev, uint16_t pointer, uint16_t *word)
{
	const struct sps30_config *cfg = dev->config;
	uint8_t buf[3];
	int ret;

	ret = sps30_set_pointer(dev, pointer);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_read_dt(&cfg->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	if (sps30_crc8(buf) != buf[2]) {
		LOG_ERR("CRC mismatch on read");
		return -EIO;
	}

	*word = sys_get_be16(buf);

	return 0;
}

/* Start measurement in float output format (datasheet §6.3.1)
 * Write: subcommand 0x03 (float), dummy 0x00, CRC
 */
static int sps30_start_measurement(const struct device *dev)
{
	const struct sps30_config *cfg = dev->config;
	uint8_t data[3];

	data[0] = SPS30_MEAS_FLOAT;
	data[1] = 0x00;
	data[2] = sps30_crc8(data);

	uint8_t buf[5];

	sys_put_be16(SPS30_PTR_START_MEASUREMENT, buf);
	buf[2] = data[0];
	buf[3] = data[1];
	buf[4] = data[2];

	return i2c_write_dt(&cfg->bus, buf, sizeof(buf));
}

/* Stop measurement — return to Idle mode (datasheet §6.3.2) */
static int sps30_stop_measurement(const struct device *dev)
{
	return sps30_set_pointer(dev, SPS30_PTR_STOP_MEASUREMENT);
}

/* Check Data-Ready flag (datasheet §6.3.3)
 * Returns 1 if ready, 0 if not, negative on error
 */
static int sps30_data_ready(const struct device *dev)
{
	uint16_t word;
	int ret;

	ret = sps30_read_word(dev, SPS30_PTR_READ_DATA_READY, &word);
	if (ret < 0) {
		return ret;
	}

	/* Byte 1 of the response is the flag (0x00 or 0x01) */
	return (word & 0xFF) == SPS30_DATA_READY ? 1 : 0;
}

/* Read measured values in float format (datasheet §6.3.4)
 * 60 bytes = 10 floats × (2 data bytes + 1 CRC) × 2 halves
 */
static int sps30_read_measured_values(const struct device *dev)
{
	struct sps30_data *data = dev->data;
	const struct sps30_config *cfg = dev->config;
	uint8_t buf[SPS30_FLOAT_RESPONSE_SIZE];
	int ret;
	int pos;

	ret = sps30_set_pointer(dev, SPS30_PTR_READ_MEASURED_VALUES);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_read_dt(&cfg->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	/* Validate CRC for every 3-byte chunk */
	for (pos = 0; pos < SPS30_FLOAT_RESPONSE_SIZE; pos += 3) {
		if (sps30_crc8(&buf[pos]) != buf[pos + 2]) {
			LOG_ERR("CRC mismatch at offset %d", pos);
			return -EIO;
		}
	}

	/* Reassemble big-endian IEEE754 floats.
	 * Each float is split into two 16-bit halves, each followed by CRC.
	 * Layout: [half0_hi, half0_lo, crc, half1_hi, half1_lo, crc] per value.
	 */
	float values[SPS30_NUM_VALUES];

	for (int i = 0; i < SPS30_NUM_VALUES; i++) {
		int base = i * 6;
		uint32_t raw = (uint32_t)buf[base] << 24 |
			       (uint32_t)buf[base + 1] << 16 |
			       (uint32_t)buf[base + 3] << 8 |
			       (uint32_t)buf[base + 4];

		memcpy(&values[i], &raw, sizeof(float));
	}

	/* Datasheet §4.3 float format:
	 * Index 0: PM1.0  [µg/m³]
	 * Index 1: PM2.5  [µg/m³]
	 * Index 2: PM4.0  [µg/m³]
	 * Index 3: PM10   [µg/m³]
	 * Index 4-8: Number concentration (not used here)
	 * Index 9: Typical particle size (not used here)
	 */
	data->pm1  = values[0];
	data->pm25 = values[1];
	data->pm4  = values[2];
	data->pm10 = values[3];

	return 0;
}

/* Enter Sleep mode (datasheet §6.3.5)
 * Must be in Idle mode first (i.e., measurement stopped)
 */
static int sps30_enter_sleep(const struct device *dev)
{
	return sps30_set_pointer(dev, SPS30_PTR_SLEEP);
}

/* Wake from Sleep mode (datasheet §6.3.6)
 * Send wake command twice: first activates I2C interface, second wakes sensor
 */
static int sps30_wake_up(const struct device *dev)
{
	int ret;

	/* First command — activates I2C interface, may NACK */
	sps30_set_pointer(dev, SPS30_PTR_WAKE_UP);
	k_msleep(SPS30_WAKE_DELAY_MS);

	/* Second command — within 100ms of first, actually wakes sensor */
	ret = sps30_set_pointer(dev, SPS30_PTR_WAKE_UP);
	if (ret < 0) {
		LOG_WRN("Second wake command failed: %d", ret);
	}

	return ret;
}

/* Start fan cleaning (datasheet §6.3.7) */
static int sps30_start_fan_cleaning(const struct device *dev)
{
	return sps30_set_pointer(dev, SPS30_PTR_START_FAN_CLEANING);
}

/* Read auto cleaning interval (datasheet §6.3.8) */
static int sps30_read_cleaning_interval(const struct device *dev, uint32_t *interval)
{
	const struct sps30_config *cfg = dev->config;
	uint8_t buf[6];
	int ret;

	ret = sps30_set_pointer(dev, SPS30_PTR_AUTO_CLEANING_INTERVAL);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_read_dt(&cfg->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	/* Validate CRCs for two 2-byte packets */
	if (sps30_crc8(&buf[0]) != buf[2]) {
		return -EIO;
	}
	if (sps30_crc8(&buf[3]) != buf[5]) {
		return -EIO;
	}

	*interval = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
		    ((uint32_t)buf[3] << 8) | (uint32_t)buf[4];

	return 0;
}

/* Write auto cleaning interval (datasheet §6.3.8)
 * Write 6 bytes to pointer 0x8004:
 * Byte 0,1: MSB of interval (uint32)
 * Byte 2:   CRC for bytes 0,1
 * Byte 3,4: LSB of interval (uint32)
 * Byte 5:   CRC for bytes 3,4
 */
static int sps30_write_cleaning_interval(const struct device *dev, uint32_t interval)
{
	const struct sps30_config *cfg = dev->config;
	uint8_t buf[8];
	uint8_t msb[2], lsb[2];

	sys_put_be16(SPS30_PTR_AUTO_CLEANING_INTERVAL, buf);

	sys_put_be16((uint16_t)(interval >> 16), msb);
	sys_put_be16((uint16_t)(interval & 0xFFFF), lsb);

	buf[2] = msb[0];
	buf[3] = msb[1];
	buf[4] = sps30_crc8(msb);
	buf[5] = lsb[0];
	buf[6] = lsb[1];
	buf[7] = sps30_crc8(lsb);

	return i2c_write_dt(&cfg->bus, buf, sizeof(buf));
}

/* Read Device Status Register (datasheet §6.3.11) */
static int sps30_read_device_status(const struct device *dev, uint32_t *status)
{
	const struct sps30_config *cfg = dev->config;
	uint8_t buf[6];
	int ret;

	ret = sps30_set_pointer(dev, SPS30_PTR_READ_DEVICE_STATUS);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_read_dt(&cfg->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	if (sps30_crc8(&buf[0]) != buf[2]) {
		return -EIO;
	}
	if (sps30_crc8(&buf[3]) != buf[5]) {
		return -EIO;
	}

	*status = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
		  ((uint32_t)buf[3] << 8) | (uint32_t)buf[4];

	return 0;
}

/* Clear Device Status Register (datasheet §6.3.12) */
static int sps30_clear_device_status(const struct device *dev)
{
	return sps30_set_pointer(dev, SPS30_PTR_CLEAR_DEVICE_STATUS);
}

/* Device Reset (datasheet §6.3.13) */
static int sps30_device_reset(const struct device *dev)
{
	return sps30_set_pointer(dev, SPS30_PTR_DEVICE_RESET);
}

/* Read device information string (datasheet §6.3.9)
 * Response: pairs of (2 data bytes + 1 CRC).
 * @param num_chars  Number of characters to read from sensor.
 */
static int sps30_read_device_info_str(const struct device *dev, uint16_t pointer,
				      char *buf, size_t buf_size, size_t num_chars)
{
	const struct sps30_config *cfg = dev->config;
	uint8_t raw[48]; /* max response: 32 chars × 3/2 = 48 bytes */
	size_t raw_len;
	int ret;

	ret = sps30_set_pointer(dev, pointer);
	if (ret < 0) {
		return ret;
	}

	raw_len = (num_chars / 2) * 3;
	if (raw_len > sizeof(raw)) {
		raw_len = sizeof(raw);
	}

	ret = i2c_read_dt(&cfg->bus, raw, raw_len);
	if (ret < 0) {
		return ret;
	}

	for (size_t pos = 0, out = 0; pos < raw_len; pos += 3, out += 2) {
		if (sps30_crc8(&raw[pos]) != raw[pos + 2]) {
			return -EIO;
		}
		if (out < buf_size) {
			buf[out] = raw[pos];
		}
		if (out + 1 < buf_size) {
			buf[out + 1] = raw[pos + 1];
		}
		if (raw[pos] == '\0' || raw[pos + 1] == '\0') {
			break;
		}
	}

	buf[buf_size - 1] = '\0';
	return 0;
}

/* Read firmware version (datasheet §6.3.10) */
static int sps30_read_firmware_version(const struct device *dev, uint8_t *major, uint8_t *minor)
{
	const struct sps30_config *cfg = dev->config;
	uint8_t buf[3];
	int ret;

	ret = sps30_set_pointer(dev, SPS30_PTR_READ_VERSION);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_read_dt(&cfg->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	if (sps30_crc8(buf) != buf[2]) {
		return -EIO;
	}

	*major = buf[0];
	*minor = buf[1];

	return 0;
}

/* ---- Public API ---- */

int sps30_device_info_get(const struct device *dev, struct sps30_info *info)
{
	const struct sps30_data *data = dev->data;

	info->product_type = data->product_type;
	info->serial_number = data->serial_number;
	info->fw_major = data->fw_major;
	info->fw_minor = data->fw_minor;

	return 0;
}

/* ---- Sensor API implementation ---- */

static int sps30_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_PM_1_0 &&
	    chan != SENSOR_CHAN_PM_2_5 && chan != SENSOR_CHAN_PM_4_0 &&
	    chan != SENSOR_CHAN_PM_10) {
		return -ENOTSUP;
	}

	ret = sps30_data_ready(dev);
	if (ret > 0) {
		return sps30_read_measured_values(dev);
	}

	if (ret == 0) {
		return -EAGAIN;
	}

	return ret;
}

static int sps30_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	const struct sps30_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_PM_1_0:
		val->val1 = (int32_t)data->pm1;
		val->val2 = (int32_t)((data->pm1 - val->val1) * 1000000);
		break;
	case SENSOR_CHAN_PM_2_5:
		val->val1 = (int32_t)data->pm25;
		val->val2 = (int32_t)((data->pm25 - val->val1) * 1000000);
		break;
	case SENSOR_CHAN_PM_4_0:
		val->val1 = (int32_t)data->pm4;
		val->val2 = (int32_t)((data->pm4 - val->val1) * 1000000);
		break;
	case SENSOR_CHAN_PM_10:
		val->val1 = (int32_t)data->pm10;
		val->val2 = (int32_t)((data->pm10 - val->val1) * 1000000);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int sps30_attr_set(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, const struct sensor_value *val)
{
	int ret;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_sps30)attr) {
	case SENSOR_ATTR_SPS30_FAN_CLEANING:
		ret = sps30_start_fan_cleaning(dev);
		if (ret < 0) {
			LOG_ERR("Fan cleaning failed: %d", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_SPS30_CLEANING_INTERVAL:
		ret = sps30_write_cleaning_interval(dev, (uint32_t)val->val1);
		if (ret < 0) {
			LOG_ERR("Write cleaning interval failed: %d", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_SPS30_DEVICE_RESET:
		ret = sps30_device_reset(dev);
		if (ret < 0) {
			LOG_ERR("Device reset failed: %d", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_SPS30_CLEAR_STATUS:
		ret = sps30_clear_device_status(dev);
		if (ret < 0) {
			LOG_ERR("Clear device status failed: %d", ret);
			return ret;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int sps30_attr_get(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, struct sensor_value *val)
{
	int ret;
	uint32_t tmp;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_sps30)attr) {
	case SENSOR_ATTR_SPS30_CLEANING_INTERVAL:
		ret = sps30_read_cleaning_interval(dev, &tmp);
		if (ret < 0) {
			return ret;
		}
		val->val1 = (int32_t)tmp;
		val->val2 = 0;
		break;

	case SENSOR_ATTR_SPS30_DEVICE_STATUS:
		ret = sps30_read_device_status(dev, &tmp);
		if (ret < 0) {
			return ret;
		}
		val->val1 = (int32_t)tmp;
		val->val2 = 0;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int sps30_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = sps30_stop_measurement(dev);
		if (ret < 0) {
			return ret;
		}
		k_msleep(SPS30_FAN_SPIN_DOWN_MS);
		return sps30_enter_sleep(dev);

	case PM_DEVICE_ACTION_RESUME:
		ret = sps30_wake_up(dev);
		if (ret < 0) {
			return ret;
		}
		k_msleep(SPS30_CMD_DELAY_MS);
		return sps30_start_measurement(dev);

	default:
		return -ENOTSUP;
	}
}

static int sps30_init(const struct device *dev)
{
	const struct sps30_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->bus.bus);
		return -ENODEV;
	}

	/* Stop any running measurement (from previous power cycle) */
	ret = sps30_stop_measurement(dev);
	if (ret < 0) {
		LOG_DBG("Stop measurement failed (sensor may be in sleep): %d", ret);
		/* Try wake sequence — sensor might be in sleep mode */
		sps30_wake_up(dev);
		k_msleep(SPS30_CMD_DELAY_MS);

		ret = sps30_stop_measurement(dev);
		if (ret < 0) {
			LOG_ERR("Stop measurement still failed after wake: %d", ret);
			return ret;
		}
	}

	k_msleep(SPS30_CMD_DELAY_MS);

	/* Read device information (non-fatal on failure) */
	struct sps30_data *data = dev->data;

	ret = sps30_read_device_info_str(dev, SPS30_PTR_READ_PRODUCT_TYPE,
					 data->product_type, sizeof(data->product_type), 8);
	if (ret < 0) {
		LOG_WRN("Failed to read product type: %d", ret);
	}

	ret = sps30_read_device_info_str(dev, SPS30_PTR_READ_SERIAL_NUMBER,
					 data->serial_number, sizeof(data->serial_number), 32);
	if (ret < 0) {
		LOG_WRN("Failed to read serial number: %d", ret);
	}

	ret = sps30_read_firmware_version(dev, &data->fw_major, &data->fw_minor);
	if (ret < 0) {
		LOG_WRN("Failed to read firmware version: %d", ret);
	}

	/* Start measurement in float output format */
	ret = sps30_start_measurement(dev);
	if (ret < 0) {
		LOG_ERR("Start measurement failed: %d", ret);
		return ret;
	}

	return pm_device_driver_init(dev, sps30_pm_action);
}

static DEVICE_API(sensor, sps30_api_funcs) = {
	.sample_fetch = sps30_sample_fetch,
	.channel_get = sps30_channel_get,
	.attr_set = sps30_attr_set,
	.attr_get = sps30_attr_get,
};

#define SPS30_INIT(inst)                                                               \
	static struct sps30_data sps30_data_##inst;                                      \
	static const struct sps30_config sps30_config_##inst = {                         \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                      \
	};                                                                              \
	PM_DEVICE_DT_INST_DEFINE(inst, sps30_pm_action);                                \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sps30_init, PM_DEVICE_DT_INST_GET(inst),     \
				     &sps30_data_##inst,                               \
				     &sps30_config_##inst, POST_KERNEL,                 \
				     CONFIG_SENSOR_INIT_PRIORITY, &sps30_api_funcs);

#define DT_DRV_COMPAT sensirion_sps30
DT_INST_FOREACH_STATUS_OKAY(SPS30_INIT)
#undef DT_DRV_COMPAT
