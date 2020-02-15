/*
 * Copyright (c) 2020 Philémon Jaermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>
#include <init.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "sht21.h"

LOG_MODULE_REGISTER(SHT21, CONFIG_SENSOR_LOG_LEVEL);

static int sht21_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct sht21_data *drv_data = dev->driver_data;
	s32_t raw = 0;

	/* Conversion according to SHT21 Datasheet
	 * formulas:
	 *	- RH = -6 + 125 * (Srh / 2^16)
	 *	- Temp = -46.85 + 175.72 * (Stemp / 2^16)
	 *
	 * Status bits (bit 0 and 1) of the raw sample have to
	 * be set to 0 before doing a conversion.
	 */
	if (chan == SENSOR_CHAN_HUMIDITY) {
		raw = (s32_t)drv_data->rh_sample & 0xFFFC;
		raw *= 12500;
		raw = raw >> 16;
		raw -= 600;
		val->val1 = raw / 100;
		val->val2 = (raw % 100) * 10000;
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		raw = (s32_t)drv_data->t_sample & 0xFFFC;
		raw *= 17572;
		raw = raw >> 16;
		raw -= 4685;
		val->val1 = raw / 100;
		val->val2 = (raw % 100) * 10000;
	} else {
		LOG_ERR("Non supported channel");
		return -EIO;
	}

	return 0;
}

static u8_t compute_crc(u16_t value)
{
	u8_t buf[2] = { value >> 8, value & 0xFF };
	u8_t crc = 0x00;
	u8_t polynom = 0x31;
	int i, j;

	for (i = 0; i < 2; ++i) {
		crc = crc ^ buf[i];
		for (j = 0; j < 8; ++j) {
			if (crc & 0x80) {
				crc = (crc << 1) ^ polynom;
			} else {
				crc = crc << 1;
			}
		}
	}

	return crc;
}

static int sht21_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct sht21_data *drv_data = dev->driver_data;
	const struct sht21_config *cfg = dev->config->config_info;
	u8_t cmd[2] = {SHT21_HUMIDITY_MEAS_NO_HOLD,
		       SHT21_TEMPERATURE_MEAS_NO_HOLD};
	u8_t buf[3] = {0};

	if (i2c_write(drv_data->i2c, &cmd[0], 1, cfg->i2c_addr)) {
		LOG_ERR("Failed to start humidity measurement");
		return -EIO;
	}
	k_sleep(K_MSEC(SHT21_MEAS_RH_WAIT_TIME));

	if (i2c_read(drv_data->i2c, buf, sizeof(buf), cfg->i2c_addr)) {
		LOG_ERR("Failed to retrieve humidity measurement");
		return -EIO;
	}

	if (!(buf[1] & SHT21_STATUS_BIT_RH_MEAS)) {
		LOG_ERR("Received unexpected non-humidity data");
		return -EIO;
	}

	drv_data->rh_sample = (buf[0] << 8) | buf[1];
	if (compute_crc(drv_data->rh_sample) != buf[2]) {
		LOG_ERR("Received invalid humidity. CRC mismatch");
		return -EIO;
	}

	if (i2c_write(drv_data->i2c, &cmd[1], 1, cfg->i2c_addr)) {
		LOG_ERR("Failed to start temperature measurement");
		return -EIO;
	}
	k_sleep(K_MSEC(SHT21_MEAS_TEMP_WAIT_TIME));

	memset(buf, 0, sizeof(buf));
	if (i2c_read(drv_data->i2c, buf, sizeof(buf), cfg->i2c_addr)) {
		LOG_ERR("Failed to retrieve temperature measurement");
		return -EIO;
	}

	if (buf[1] & SHT21_STATUS_BIT_RH_MEAS) {
		LOG_ERR("Received unexpected non-temperature data");
		return -EIO;
	}

	drv_data->t_sample = (buf[0] << 8) | buf[1];
	if (compute_crc(drv_data->t_sample) != buf[2]) {
		LOG_ERR("Received invalid temperature. CRC mismatch");
		return -EIO;
	}

	return 0;
}

static const struct sensor_driver_api sht21_driver_api = {
	.sample_fetch = sht21_sample_fetch,
	.channel_get = sht21_channel_get,
};

static int sht21_init(struct device *dev)
{
	struct sht21_data *drv_data = dev->driver_data;
	const struct sht21_config *cfg = dev->config->config_info;
	u8_t user_cfg;

	drv_data->i2c = device_get_binding(cfg->i2c_label);
	if (drv_data->i2c == NULL) {
		LOG_ERR("Could not get pointer to %s device.",
			    cfg->i2c_label);
		return -EINVAL;
	}


	if (i2c_reg_read_byte(drv_data->i2c, cfg->i2c_addr,
			      SHT21_READ_USER_REG, &user_cfg) < 0) {
		LOG_ERR("Failed to read user config.");
		return -EIO;
	}

	user_cfg &= ~(BIT(SHT21_RH_RESOLUTION_BIT_POS) |
		      BIT(SHT21_TEMP_RESOLUTION_BIT_POS) |
		      BIT(SHT21_ON_CHIP_HEATER_BIT_POS) |
		      BIT(SHT21_OTP_RELOAD_BIT_POS));

	user_cfg |= (SHT21_RH_RESOLUTION |
		     SHT21_TEMP_RESOLUTION |
		     SHT21_HEATER |
		     SHT21_OTP_RELOAD);

	if (i2c_reg_write_byte(drv_data->i2c, cfg->i2c_addr,
			      SHT21_WRITE_USER_REG, user_cfg) < 0) {
		LOG_ERR("Failed to write user config.");
		return -EIO;
	}

	return 0;
}

static struct sht21_data sht21_driver;

static const struct sht21_config sht21_cfg = {
	.i2c_label = DT_INST_0_SENSIRION_SHT21_BUS_NAME,
	.i2c_addr = DT_INST_0_SENSIRION_SHT21_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(sht21, DT_INST_0_SENSIRION_SHT21_LABEL, sht21_init,
		    &sht21_driver, &sht21_cfg, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &sht21_driver_api);
