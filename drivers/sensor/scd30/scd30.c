/*
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * SPDX-License-Identifier: BSD-3-CLAUSE
 */

#define DT_DRV_COMPAT sensirion_scd30

#include <device.h>
#include <drivers/i2c.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <sys/crc.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(SCD30, CONFIG_SENSOR_LOG_LEVEL);

#include "scd30.h"

static int scd30_write_command(const struct device *dev, uint16_t cmd)
{
	const struct scd30_config *cfg = dev->config;
	uint8_t tx_buf[2];

	sys_put_be16(cmd, tx_buf);
	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static uint8_t scd30_compute_crc(uint8_t *data, uint8_t data_len)
{
	return crc8(data, data_len, SCD30_CRC8_POLYNOMIAL, SCD30_CRC8_INIT, false);
}

static int scd30_check_crc(uint8_t *data, uint8_t data_len, uint8_t checksum)
{
	uint8_t actual_crc = scd30_compute_crc(data, data_len);

	if (checksum != actual_crc) {
		LOG_ERR("CRC check failed. Expected: %x, got %x", checksum, actual_crc);
		return -EIO;
	}

	return 0;
}

static int scd30_write_register(const struct device *dev, uint16_t cmd, uint16_t val)
{
	const struct scd30_config *cfg = dev->config;
	uint8_t tx_buf[SCD30_CMD_SINGLE_WORD_BUF_LEN];

	sys_put_be16(cmd, &tx_buf[0]);
	sys_put_be16(val, &tx_buf[2]);
	tx_buf[4] = scd30_compute_crc(&tx_buf[2], sizeof(val));

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int scd30_read_register(const struct device *dev, uint16_t reg, uint16_t *val)
{
	const struct scd30_config *cfg = dev->config;
	struct scd30_word rx_word;
	int rc;

	rc = scd30_write_command(dev, reg);
	if (rc != 0) {
		return rc;
	}

	rc = i2c_read_dt(&cfg->bus, (uint8_t *)&rx_word, sizeof(rx_word));
	if (rc != 0) {
		return rc;
	}

	if (scd30_check_crc(rx_word.word, sizeof(rx_word.word), rx_word.crc) != 0) {
		return -EIO;
	}

	*val = sys_get_be16(rx_word.word);
	return 0;
}

/**
 * @brief Take the received  word (16 bit), if the data is valid, add it to dst
 *
 * @param data data received from sensor
 * @param dst buffer to store data
 * @return int
 */
static int scd30_fill_data_buf(struct scd30_word word, uint8_t *dst)
{
	if (scd30_check_crc(word.word, SCD30_WORD_SIZE, word.crc)) {
		return -EIO;
	}

	dst[0] = word.word[0];
	dst[1] = word.word[1];

	return 0;
}

static float scd30_bytes_to_float(const uint8_t *bytes)
{
	union {
		uint32_t u32_value;
		float float32;
	} tmp;

	tmp.u32_value = sys_get_be32(bytes);
	return tmp.float32;
}

static int scd30_get_sample_time(const struct device *dev)
{
	struct scd30_data *data = dev->data;
	uint16_t sample_time;
	int rc;

	rc = scd30_read_register(dev, SCD30_CMD_SET_MEASUREMENT_INTERVAL, &sample_time);
	if (rc != 0) {
		return rc;
	}

	data->sample_time = sample_time;

	return 0;
}

static int scd30_set_sample_time(const struct device *dev, uint16_t sample_time)
{
	struct scd30_data *data = dev->data;
	int rc;

	if (sample_time < SCD30_MIN_SAMPLE_TIME || sample_time > SCD30_MAX_SAMPLE_TIME) {
		return -EINVAL;
	}

	rc = scd30_write_command(dev, SCD30_CMD_STOP_PERIODIC_MEASUREMENT);
	if (rc != 0) {
		return rc;
	}

	rc = scd30_write_register(dev, SCD30_CMD_SET_MEASUREMENT_INTERVAL, sample_time);
	if (rc != 0) {
		return rc;
	}

	data->sample_time = sample_time;

	return scd30_write_register(dev, SCD30_CMD_START_PERIODIC_MEASUREMENT,
				    SCD30_MEASUREMENT_DEF_AMBIENT_PRESSURE);
	;
}

static int scd30_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	uint16_t data_ready;
	struct scd30_data *data = dev->data;
	const struct scd30_config *cfg = dev->config;
	int rc;

	/*
	 * Struct represensting data as received from the SCD30
	 * each scd30_word conists of a 16 bit data word followed
	 * by an 8 bit crc
	 */
	struct scd30_rx_data {
		struct scd30_word co2_msw;
		struct scd30_word co2_lsw;
		struct scd30_word temp_msw;
		struct scd30_word temp_lsw;
		struct scd30_word humidity_msw;
		struct scd30_word humidity_lsw;
	} raw_rx_data;

	/*
	 * struct representing the received data from the SCD30
	 * in big endian order with the CRC's removed.
	 */
	struct rx_data {
		uint8_t co2_be[sizeof(float)];
		uint8_t temp_be[sizeof(float)];
		uint8_t humidity_be[sizeof(float)];
	} rx_data;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	rc = scd30_read_register(dev, SCD30_CMD_GET_DATA_READY, &data_ready);
	if (rc != 0) {
		return rc;
	}

	if (!data_ready) {
		return -ENODATA;
	}

	rc = scd30_write_command(dev, SCD30_CMD_READ_MEASUREMENT);
	if (rc != 0) {
		LOG_DBG("Failed to send command. (rc = %d)", rc);
		return rc;
	}

	/* delay for 3 msec as per datasheet. */
	k_sleep(K_MSEC(3));

	rc = i2c_read_dt(&cfg->bus, (uint8_t *)&raw_rx_data, sizeof(raw_rx_data));
	if (rc != 0) {
		LOG_DBG("Failed to read data. (rc = %d)", rc);
		return rc;
	}

	/* C02 data */
	rc = scd30_fill_data_buf(raw_rx_data.co2_msw, &rx_data.co2_be[0]);
	if (rc != 0) {
		return rc;
	}
	rc = scd30_fill_data_buf(raw_rx_data.co2_lsw, &rx_data.co2_be[SCD30_WORD_SIZE]);
	if (rc != 0) {
		return rc;
	}

	/* temperature data */
	rc = scd30_fill_data_buf(raw_rx_data.temp_msw, &rx_data.temp_be[0]);
	if (rc != 0) {
		return rc;
	}
	rc = scd30_fill_data_buf(raw_rx_data.temp_lsw, &rx_data.temp_be[SCD30_WORD_SIZE]);
	if (rc != 0) {
		return rc;
	}

	/* relative humidity */
	rc = scd30_fill_data_buf(raw_rx_data.humidity_msw, &rx_data.humidity_be[0]);
	if (rc != 0) {
		return rc;
	}
	rc = scd30_fill_data_buf(raw_rx_data.humidity_lsw, &rx_data.humidity_be[SCD30_WORD_SIZE]);
	if (rc != 0) {
		return rc;
	}

	data->co2_ppm = scd30_bytes_to_float(rx_data.co2_be);
	data->temp = scd30_bytes_to_float(rx_data.temp_be);
	data->rel_hum = scd30_bytes_to_float(rx_data.humidity_be);

	return 0;
}

static int scd30_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct scd30_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_CO2:
		sensor_value_from_double(val, data->co2_ppm);
		break;

	case SENSOR_CHAN_AMBIENT_TEMP:
		sensor_value_from_double(val, data->temp);
		break;

	case SENSOR_CHAN_HUMIDITY:
		sensor_value_from_double(val, data->rel_hum);
		break;

	default:
		return -1;
	}
	return 0;
}

static int scd30_attr_get(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, struct sensor_value *val)
{
	struct scd30_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
	{
		float frequency = 1.0 / data->sample_time;

		sensor_value_from_double(val, frequency);
		return 0;
	}
	case SENSOR_ATTR_SAMPLING_PERIOD:
	{
		val->val1 = data->sample_time;
		val->val2 = 0;
		return 0;
	}

	default:
		return -ENOTSUP;
	}
}

static int scd30_attr_set(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
	{
		double frequency = sensor_value_to_double(val);
		double sample_time = 1.0 /  frequency;

		return scd30_set_sample_time(dev, (uint16_t)sample_time);
	}
	case SENSOR_ATTR_SAMPLING_PERIOD:
	{
		return scd30_set_sample_time(dev, (uint16_t)val->val1);
	}
	default:
		return -ENOTSUP;
	}
}

static const struct sensor_driver_api scd30_driver_api = {
	.sample_fetch = scd30_sample_fetch,
	.channel_get = scd30_channel_get,
	.attr_get = scd30_attr_get,
	.attr_set = scd30_attr_set,
};

static int scd30_init(const struct device *dev)
{
	LOG_DBG("Initializing SCD30");
	const struct scd30_config *cfg = dev->config;
	struct scd30_data *data = dev->data;
	int rc;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("Failed to get pointer to %s device!", cfg->bus.bus->name);
		return -ENODEV;
	}

	rc = scd30_set_sample_time(dev, data->sample_time);
	if (rc != 0) {
		LOG_WRN("Failed to set sample period. Using period stored of device");
		/* Try to read sample time from sensor to reflect actual sample period */
		rc = scd30_get_sample_time(dev);
	}

	LOG_DBG("Sample time: %d", data->sample_time);
	if (rc != 0) {
		return rc;
	}


	LOG_DBG("Starting periodic measurements");
	rc = scd30_write_register(dev, SCD30_CMD_START_PERIODIC_MEASUREMENT,
				  SCD30_MEASUREMENT_DEF_AMBIENT_PRESSURE);

	return rc;
}

#define SCD30_DEFINE(inst)									\
	static struct scd30_data scd30_data_##inst = {						\
		.sample_time = DT_INST_PROP(inst, sample_period)				\
	};											\
	static const struct scd30_config scd30_config_##inst = {				\
		.bus = I2C_DT_SPEC_INST_GET(inst),						\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, scd30_init, NULL, &scd30_data_##inst, &scd30_config_##inst,	\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &scd30_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SCD30_DEFINE);
