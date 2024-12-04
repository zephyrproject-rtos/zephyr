/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_sht3xd

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/logging/log.h>

#include "sht3xd.h"

LOG_MODULE_REGISTER(SHT3XD, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_SHT3XD_SINGLE_SHOT_MODE
static const uint16_t measure_cmd[3] = {
	0x2416, 0x240B, 0x2400
};
#endif
#ifdef CONFIG_SHT3XD_PERIODIC_MODE
static const uint16_t measure_cmd[5][3] = {
	{ 0x202F, 0x2024, 0x2032 },
	{ 0x212D, 0x2126, 0x2130 },
	{ 0x222B, 0x2220, 0x2236 },
	{ 0x2329, 0x2322, 0x2334 },
	{ 0x272A, 0x2721, 0x2737 }
};
#endif

static const int measure_wait[3] = {
	4000, 6000, 15000
};

/*
 * CRC algorithm parameters were taken from the
 * "Checksum Calculation" section of the datasheet.
 */
static uint8_t sht3xd_compute_crc(uint16_t value)
{
	uint8_t buf[2];

	sys_put_be16(value, buf);
	return crc8(buf, 2, 0x31, 0xFF, false);
}

int sht3xd_write_command(const struct device *dev, uint16_t cmd)
{
	const struct sht3xd_config *config = dev->config;
	uint8_t tx_buf[2];

	sys_put_be16(cmd, tx_buf);
	return i2c_write_dt(&config->bus, tx_buf, sizeof(tx_buf));
}

int sht3xd_write_reg(const struct device *dev, uint16_t cmd, uint16_t val)
{
	const struct sht3xd_config *config = dev->config;
	uint8_t tx_buf[5];

	sys_put_be16(cmd, &tx_buf[0]);
	sys_put_be16(val, &tx_buf[2]);
	tx_buf[4] = sht3xd_compute_crc(val);

	return i2c_write_dt(&config->bus, tx_buf, sizeof(tx_buf));
}

static int sht3xd_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	const struct sht3xd_config *config = dev->config;
	struct sht3xd_data *data = dev->data;
	uint8_t rx_buf[6];
	uint16_t t_sample, rh_sample;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

#ifdef CONFIG_SHT3XD_SINGLE_SHOT_MODE
	/* start single shot measurement */
	if (sht3xd_write_command(dev,
				 measure_cmd[SHT3XD_REPEATABILITY_IDX])
	    < 0) {
		LOG_DBG("Failed to set single shot measurement mode!");
		return -EIO;
	}
	k_sleep(K_MSEC(measure_wait[SHT3XD_REPEATABILITY_IDX] / USEC_PER_MSEC));

	if (i2c_read_dt(&config->bus, rx_buf, sizeof(rx_buf)) < 0) {
		LOG_DBG("Failed to read data sample!");
		return -EIO;
	}
#endif
#ifdef CONFIG_SHT3XD_PERIODIC_MODE
	uint8_t tx_buf[2];

	sys_put_be16(SHT3XD_CMD_FETCH, tx_buf);

	if (i2c_write_read_dt(&config->bus, tx_buf, sizeof(tx_buf),
			      rx_buf, sizeof(rx_buf)) < 0) {
		LOG_DBG("Failed to read data sample!");
		return -EIO;
	}
#endif

	t_sample = sys_get_be16(&rx_buf[0]);
	if (sht3xd_compute_crc(t_sample) != rx_buf[2]) {
		LOG_DBG("Received invalid temperature CRC!");
		return -EIO;
	}

	rh_sample = sys_get_be16(&rx_buf[3]);
	if (sht3xd_compute_crc(rh_sample) != rx_buf[5]) {
		LOG_DBG("Received invalid relative humidity CRC!");
		return -EIO;
	}

	data->t_sample = t_sample;
	data->rh_sample = rh_sample;

	return 0;
}

static int sht3xd_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct sht3xd_data *data = dev->data;
	uint64_t tmp;

	/*
	 * See datasheet "Conversion of Signal Output" section
	 * for more details on processing sample data.
	 */
	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		/* val = -45 + 175 * sample / (2^16 -1) */
		tmp = (uint64_t)data->t_sample * 175U;
		val->val1 = (int32_t)(tmp / 0xFFFF) - 45;
		val->val2 = ((tmp % 0xFFFF) * 1000000U) / 0xFFFF;
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		/* val = 100 * sample / (2^16 -1) */
		uint32_t tmp2 = (uint32_t)data->rh_sample * 100U;
		val->val1 = tmp2 / 0xFFFF;
		/* x * 100000 / 65536 == x * 15625 / 1024 */
		val->val2 = (tmp2 % 0xFFFF) * 15625U / 1024;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, sht3xd_driver_api) = {
#ifdef CONFIG_SHT3XD_TRIGGER
	.attr_set = sht3xd_attr_set,
	.trigger_set = sht3xd_trigger_set,
#endif
	.sample_fetch = sht3xd_sample_fetch,
	.channel_get = sht3xd_channel_get,
};

static int sht3xd_init(const struct device *dev)
{
	const struct sht3xd_config *cfg = dev->config;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("I2C bus %s is not ready!", cfg->bus.bus->name);
		return -EINVAL;
	}

	/* clear status register */
	if (sht3xd_write_command(dev, SHT3XD_CMD_CLEAR_STATUS) < 0) {
		LOG_DBG("Failed to clear status register!");
		return -EIO;
	}

	k_busy_wait(SHT3XD_CLEAR_STATUS_WAIT_USEC);

#ifdef CONFIG_SHT3XD_PERIODIC_MODE
	/* set periodic measurement mode */
	if (sht3xd_write_command(dev,
				 measure_cmd[SHT3XD_MPS_IDX][SHT3XD_REPEATABILITY_IDX])
	    < 0) {
		LOG_DBG("Failed to set measurement mode!");
		return -EIO;
	}

	k_busy_wait(measure_wait[SHT3XD_REPEATABILITY_IDX]);
#endif
#ifdef CONFIG_SHT3XD_TRIGGER
	struct sht3xd_data *data = dev->data;

	data->dev = dev;
	if (sht3xd_init_interrupt(dev) < 0) {
		LOG_DBG("Failed to initialize interrupt");
		return -EIO;
	}
#endif

	return 0;
}

#ifdef CONFIG_SHT3XD_TRIGGER
#define SHT3XD_TRIGGER_INIT(inst)						\
	.alert_gpio = GPIO_DT_SPEC_INST_GET(inst, alert_gpios),
#else
#define SHT3XD_TRIGGER_INIT(inst)
#endif

#define SHT3XD_DEFINE(inst)							\
	struct sht3xd_data sht3xd0_data_##inst;					\
	static const struct sht3xd_config sht3xd0_cfg_##inst = {		\
		.bus = I2C_DT_SPEC_INST_GET(inst),				\
		SHT3XD_TRIGGER_INIT(inst)					\
	};									\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sht3xd_init, NULL,			\
		&sht3xd0_data_##inst, &sht3xd0_cfg_##inst,			\
		POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,			\
		&sht3xd_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SHT3XD_DEFINE)
