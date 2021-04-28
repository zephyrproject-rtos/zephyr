/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_sht4x

#include <device.h>
#include <drivers/i2c.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include <sys/crc.h>

#include "sht4x.h"

LOG_MODULE_REGISTER(SHT4X, CONFIG_SENSOR_LOG_LEVEL);

/*
 * heater_cmd[POWER][LENGTH]
 * POWER = (high, med, low)
 * LENGTH = 1s, 0.1s)
 */
#if CONFIG_SHT4X_HEATER_ENABLE
static const int8_t heater_cmd[3][2] = {
	{ 0x39, 0x32 },
	{ 0x2F, 0x24 },
	{ 0x1E, 0x15 }
};

/* 1s , 0.1s */
static const int heater_wait[2] = {
	1000000, 100000
};
#endif

/*
 * CRC parameters were taken from the
 * "Checksum Calculation" section of the datasheet.
 */
static uint8_t sht4x_compute_crc(uint16_t value)
{
	uint8_t buf[2] = { value >> 8, value & 0xFF };
	return crc8(buf, 2, 0x31, 0xFF, false);;
}

int sht4x_write_command(const struct device *dev, uint8_t cmd)
{
	const struct sht4x_config *cfg = dev->config;
	uint8_t tx_buf[1] = { cmd };
	return i2c_write(cfg->bus, tx_buf, sizeof(tx_buf), cfg->i2c_addr);
}

static int sht4x_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	const struct sht4x_config *cfg = dev->config;
	struct sht4x_data *data = dev->data;
	uint8_t rx_buf[6];
	uint16_t t_sample, rh_sample;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* start single shot measurement */
	if (sht4x_write_command(dev, measure_cmd[SHT4X_REPEATABILITY_IDX])) {
		LOG_DBG("%s: Failed to start measurement.", dev->name);
		return -EIO;
	}
	k_sleep(K_MSEC(measure_wait_ms[SHT4X_REPEATABILITY_IDX]));

	if ( i2c_read(cfg->bus, rx_buf, sizeof(rx_buf), cfg->i2c_addr) ) {
		LOG_DBG("%s: Failed to read data from device.", dev->name);
		return -EIO;
	}

	t_sample = (rx_buf[0] << 8) | rx_buf[1];
	if (sht4x_compute_crc(t_sample) != rx_buf[2]) {
		LOG_DBG("%s: Invalid CRC for T.", dev->name);
		return -EIO;
	}

	rh_sample = (rx_buf[3] << 8) | rx_buf[4];
	if (sht4x_compute_crc(rh_sample) != rx_buf[5]) {
		LOG_DBG("%s: Invalid CRC for RH.", dev->name);
		return -EIO;
	}

	data->t_sample = t_sample;
	data->rh_sample = rh_sample;

	return 0;
}

static int sht4x_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct sht4x_data *data = dev->data;
	uint64_t tmp;

	/*
	 * See datasheet "Conversion of Signal Output" section
	 * for more details on processing sample data.
	 */
	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		tmp = data->t_sample * 175U;
		val->val1 = (int32_t)(tmp / 0xFFFF) - 45;
		val->val2 = ((tmp % 0xFFFF) * 1000000U) / 0xFFFF;
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		tmp = data->rh_sample * 125U;
		val->val1 = (int32_t)(tmp / 0xFFFF) - 6;
		val->val2 = (tmp % 0xFFFF) * 15625U / 1024;
	} else {
		return -ENOTSUP;
	}

	return 0;
}


static int sht4x_init(const struct device *dev)
{
	const struct sht4x_config *cfg = dev->config;

	if ( !device_is_ready(cfg->bus) ) {
		LOG_DBG("%s: Device not ready.", dev->name);
		return -ENODEV;
	}

	if ( sht4x_write_command(dev, SHT4X_CMD_RESET) ) {
		LOG_DBG("%s: Failed to reset the device.", dev->name);
		return -EIO;
	}

	k_sleep(K_MSEC(SHT4X_RESET_WAIT_MS));

	return 0;
}

static const struct sensor_driver_api sht4x_api = {
	.sample_fetch = sht4x_sample_fetch,
	.channel_get = sht4x_channel_get,
};

struct sht4x_data sht4x_data_0;

static const struct sht4x_config sht4x_cfg_0 = {
	.bus = DEVICE_DT_GET(DT_INST_BUS(0)),
	.i2c_addr = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, sht4x_init, NULL,
		    &sht4x_data_0, &sht4x_cfg_0,
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &sht4x_api);
