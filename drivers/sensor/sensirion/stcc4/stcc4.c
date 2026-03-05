/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_stcc4

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include "stcc4.h"

LOG_MODULE_REGISTER(STCC4, CONFIG_SENSOR_LOG_LEVEL);

/** Compute Sensirion CRC-8 over a single 16-bit word. */
static uint8_t stcc4_compute_crc(uint16_t value)
{
	uint8_t buf[2];

	sys_put_be16(value, buf);

	return crc8(buf, 2, STCC4_CRC_POLY, STCC4_CRC_INIT, false);
}

/**
 * Send a command to the sensor.
 *
 * Most commands are 16-bit (big-endian). EXIT_SLEEP is a special case:
 * only a single 0x00 byte is sent on the bus.
 *
 * Blocks for the command's required duration after sending.
 */
static int stcc4_write_command(const struct device *dev, uint8_t cmd)
{
	const struct stcc4_config *cfg = dev->config;
	int ret;

	if (cmd == STCC4_CMD_EXIT_SLEEP) {
		uint8_t tx_buf[1] = {STCC4_WAKEUP_BYTE};

		ret = i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
	} else {
		uint8_t tx_buf[2];

		sys_put_be16(stcc4_cmds[cmd].code, tx_buf);
		ret = i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
	}

	/* Wait for command-specific processing time per datasheet */
	if (stcc4_cmds[cmd].duration_ms != 0U) {
		k_msleep(stcc4_cmds[cmd].duration_ms);
	}

	return ret;
}

/**
 * Read word+CRC tuples from the sensor.
 *
 * Each word is 3 bytes on the wire: 2 data bytes (big-endian) + 1 CRC byte.
 * The CRC is validated for every word; returns -EIO on mismatch.
 */
static int stcc4_read_reg(const struct device *dev, uint8_t *rx_buf, uint8_t rx_buf_size)
{
	const struct stcc4_config *cfg = dev->config;
	uint8_t num_words = rx_buf_size / STCC4_WORD_SIZE;
	int ret;

	ret = i2c_read_dt(&cfg->bus, rx_buf, rx_buf_size);
	if (ret < 0) {
		return ret;
	}

	for (uint8_t i = 0U; i < num_words; i++) {
		uint8_t offset = i * STCC4_WORD_SIZE;
		uint8_t crc = stcc4_compute_crc(sys_get_be16(&rx_buf[offset]));

		if (crc != rx_buf[offset + 2U]) {
			LOG_ERR("Invalid CRC (expected 0x%02x, got 0x%02x)", crc,
				rx_buf[offset + 2U]);
			return -EIO;
		}
	}

	return 0;
}

/**
 * Read a measurement from the sensor.
 *
 * Returns 4 words (12 bytes): CO2 (int16), temperature (uint16),
 * humidity (uint16), and status (uint16, currently ignored).
 */
static int stcc4_read_sample(const struct device *dev)
{
	struct stcc4_data *data = dev->data;
	uint8_t rx_buf[STCC4_RX_BUF_LEN];
	int ret;

	ret = stcc4_write_command(dev, STCC4_CMD_READ_MEASUREMENT_RAW);
	if (ret < 0) {
		LOG_ERR("Failed to write read_measurement_raw command");
		return ret;
	}

	ret = stcc4_read_reg(dev, rx_buf, sizeof(rx_buf));
	if (ret < 0) {
		return ret;
	}

	data->co2_sample = (int16_t)sys_get_be16(rx_buf);
	data->temp_sample = sys_get_be16(&rx_buf[3]);
	data->humi_sample = sys_get_be16(&rx_buf[6]);
	/* rx_buf[9..11] = status word (ignored) */

	return 0;
}

static int stcc4_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct stcc4_config *cfg = dev->config;
	int ret;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_AMBIENT_TEMP) &&
	    (chan != SENSOR_CHAN_HUMIDITY) && (chan != SENSOR_CHAN_CO2)) {
		return -ENOTSUP;
	}

	/* In single-shot mode, trigger a measurement and wait for completion */
	if (cfg->mode == STCC4_MODE_SINGLE_SHOT) {
		ret = stcc4_write_command(dev, STCC4_CMD_MEASURE_SINGLE_SHOT);
		if (ret < 0) {
			LOG_ERR("Failed to trigger single-shot measurement");
			return ret;
		}
	}

	ret = stcc4_read_sample(dev);
	if (ret < 0) {
		/*
		 * In continuous mode the sensor NACKs reads until the first
		 * measurement is ready (~1 s after START_CONTINUOUS).
		 * Return 0 so the caller keeps the previous (zero) values
		 * rather than treating this as a fatal error.
		 */
		if (cfg->mode == STCC4_MODE_CONTINUOUS) {
			LOG_DBG("Measurement not ready yet");
			return 0;
		}
		LOG_ERR("Failed to read sample data");
		return ret;
	}

	return 0;
}

/** Convert raw sensor data to Zephyr sensor_value for the requested channel. */
static int stcc4_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	const struct stcc4_data *data = dev->data;
	int64_t tmp;

	switch ((enum sensor_channel)chan) {
	case SENSOR_CHAN_CO2:
		/* CO2 raw value is directly in ppm */
		val->val1 = data->co2_sample;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* T[C] = -45 + 175 * (raw / 65535) */
		tmp = (int64_t)data->temp_sample * STCC4_MAX_TEMP;
		val->val1 = (int32_t)(tmp / (int64_t)65535) + STCC4_MIN_TEMP;
		val->val2 = (int32_t)(((tmp % (int64_t)65535) * (int64_t)1000000) / (int64_t)65535);
		break;
	case SENSOR_CHAN_HUMIDITY:
		/* RH[%] = -6 + 125 * (raw / 65535) */
		tmp = (int64_t)data->humi_sample * STCC4_MAX_HUMI;
		val->val1 = (int32_t)(tmp / (int64_t)65535) + STCC4_MIN_HUMI;
		val->val2 = (int32_t)(((tmp % (int64_t)65535) * (int64_t)1000000) / (int64_t)65535);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int stcc4_init(const struct device *dev)
{
	const struct stcc4_config *cfg = dev->config;
	uint8_t rx_buf[STCC4_RX_BUF_LEN];
	uint16_t id_hi;
	uint16_t id_lo;
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/* Wake sensor in case it is in sleep mode (NACK expected, ignore error) */
	(void)stcc4_write_command(dev, STCC4_CMD_EXIT_SLEEP);

	/* Stop any running measurement from a previous session */
	ret = stcc4_write_command(dev, STCC4_CMD_STOP_CONTINUOUS);
	if (ret < 0) {
		LOG_WRN("Failed to stop continuous measurement (may not have been running)");
	}

	/* Read and log the 4-word product ID */
	ret = stcc4_write_command(dev, STCC4_CMD_GET_PRODUCT_ID);
	if (ret < 0) {
		LOG_ERR("Failed to send get_product_id command");
		return ret;
	}

	ret = stcc4_read_reg(dev, rx_buf, sizeof(rx_buf));
	if (ret < 0) {
		LOG_ERR("Failed to read product ID");
		return ret;
	}

	id_hi = sys_get_be16(rx_buf);
	id_lo = sys_get_be16(&rx_buf[3]);
	LOG_INF("Product ID: 0x%04x%04x", id_hi, id_lo);

	/* Start continuous measurement if configured (single-shot triggers on fetch) */
	if (cfg->mode == STCC4_MODE_CONTINUOUS) {
		ret = stcc4_write_command(dev, STCC4_CMD_START_CONTINUOUS);
		if (ret < 0) {
			LOG_ERR("Failed to start continuous measurement");
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(sensor, stcc4_api) = {
	.sample_fetch = stcc4_sample_fetch,
	.channel_get = stcc4_channel_get,
};

#define STCC4_INIT(inst)                                                                           \
	static struct stcc4_data stcc4_data_##inst;                                                \
	static const struct stcc4_config stcc4_config_##inst = {                                   \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.mode = DT_INST_PROP(inst, measurement_mode),                                      \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, stcc4_init, NULL, &stcc4_data_##inst,                   \
				     &stcc4_config_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &stcc4_api);

DT_INST_FOREACH_STATUS_OKAY(STCC4_INIT)
