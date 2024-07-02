/*
 * Copyright (c) 2024 Yann Gaillard
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_sdp3x

#define SDP3X_CMD_READ_SERIAL   0x89
#define SDP3X_CMD_RESET         0x0006

#define SDP3X_RESET_WAIT_MS     25

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include "sdp3x.h"

LOG_MODULE_REGISTER(SDP3X, CONFIG_SENSOR_LOG_LEVEL);

/*
 * CRC parameters were taken from the
 * "Checksum Calculation" section of the datasheet.
 */
#define SDP3X_CRC_POLY          0x31
#define SDP3X_CRC_INIT          0xFF

struct sdp3x_config {
        struct i2c_dt_spec bus;
        uint8_t mesure_mode;
};

struct sdp3x_data {
        uint16_t t_sample;
        uint16_t p_sample;
};

#ifdef CONFIG_SDP3X_PERIODIC_MODE
static const uint16_t measure_cmd[4] = {
        0x3603, 0x3608, 0x3615, 0x361E
};
#endif

#ifdef CONFIG_SDP3X_SINGLE_SHOT_MODE
static const uint16_t measure_cmd[4] = {
        0x3624, 0x3726, 0x362F, 0x372D
};
#endif

static uint8_t sdp3x_compute_crc(uint16_t value)
{
	uint8_t buf[2];

	sys_put_be16(value, buf);

	return crc8(buf, 2, SDP3X_CRC_POLY, SDP3X_CRC_INIT, false);
}

static int sdp3x_write_command(const struct device *dev, uint16_t cmd)
{
	const struct sdp3x_config *cfg = dev->config;
	uint8_t tx_buf[2];

	sys_put_be16(cmd, tx_buf);
	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int sdp3x_read_sample(const struct device *dev,
		uint16_t *t_sample,
		uint16_t *p_sample)
{
	const struct sdp3x_config *cfg = dev->config;
	uint8_t rx_buf[9];
	int rc;

	rc = i2c_read_dt(&cfg->bus, rx_buf, sizeof(rx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to read data from device.");
		return rc;
	}

	*p_sample = sys_get_be16(&rx_buf);
	if (sdp3x_compute_crc(*p_sample) != rx_buf[2]) {
		LOG_ERR("Invalid CRC for P.");
		return -EIO;
	}

	*t_sample = sys_get_be16(&rx_buf[3]);
	if (sdp3x_compute_crc(*t_sample) != rx_buf[5]) {
		LOG_ERR("Invalid CRC for T.");
		return -EIO;
	}

	return 0;
}

static int sdp3x_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	const struct sdp3x_config *cfg = dev->config;
	struct sdp3x_data *data = dev->data;
	int rc;

	if (chan != SENSOR_CHAN_ALL &&
		chan != SENSOR_CHAN_AMBIENT_TEMP &&
		chan != SENSOR_CHAN_PRESS) {
		return -ENOTSUP;
	}

#ifdef CONFIG_SDP3X_SINGLE_SHOT_MODE
	rc = sdp3x_write_command(dev, measure_cmd[cfg->mesure_mode]);
	if (rc < 0) {
		LOG_ERR("Failed to start measurement.");
		return rc;
	}
	k_sleep(K_MSEC(50));
#endif

	rc = sdp3x_read_sample(dev, &data->t_sample, &data->p_sample);
	if (rc < 0) {
		LOG_ERR("Failed to fetch data.");
		return rc;
	}

	return 0;
}

static int sdp3x_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct sdp3x_data *data = dev->data;

	/*
	 * See datasheet "Conversion of Signal Output" section
	 * for more details on processing sample data.
	 */
	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		int64_t tmp;

		tmp = data->t_sample;
		val->val1 = (int32_t) (tmp / 200);
		val->val2 = ((tmp % 200) * 10000);
	} else if (chan == SENSOR_CHAN_PRESS) {
		uint64_t tmp;

		tmp = data->p_sample;
		val->val1 = (uint32_t)(tmp / 60);
		val->val2 = (tmp % 60) * 10000;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int sdp3x_init(const struct device *dev)
{
	const struct sdp3x_config *cfg = dev->config;
	int rc = 0;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	rc = sdp3x_write_command(dev, SDP3X_CMD_RESET);
	if (rc < 0) {
		LOG_ERR("Failed to reset the device.");
		return rc;
	}

	k_sleep(K_MSEC(SDP3X_RESET_WAIT_MS));

#ifdef CONFIG_SDP3X_PERIODIC_MODE
	rc = sdp3x_write_command(dev, measure_cmd[cfg->mesure_mode]);
	if (rc < 0) {
		LOG_ERR("Failed to start measurement.");
		return rc;
	}
#endif

	return 0;
}


static const struct sensor_driver_api sdp3x_api = {
	.sample_fetch = sdp3x_sample_fetch,
	.channel_get = sdp3x_channel_get,
};

#define SDP3X_INIT(n)						\
	static struct sdp3x_data sdp3x_data_##n;		\
								\
	static const struct sdp3x_config sdp3x_config_##n = {	\
		.bus = I2C_DT_SPEC_INST_GET(n),			\
		.mesure_mode = DT_INST_PROP(n, mesure_mode)	\
	};							\
								\
	SENSOR_DEVICE_DT_INST_DEFINE(n,				\
			      sdp3x_init,			\
			      NULL,				\
			      &sdp3x_data_##n,			\
			      &sdp3x_config_##n,		\
			      POST_KERNEL,			\
			      CONFIG_SENSOR_INIT_PRIORITY,	\
			      &sdp3x_api);

DT_INST_FOREACH_STATUS_OKAY(SDP3X_INIT)
