/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_sht3xd

#include <device.h>
#include <drivers/i2c.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "sht3xd.h"

LOG_MODULE_REGISTER(SHT3XD, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_SHT3XD_SINGLE_SHOT_MODE
static const uint16_t measure_cmd[3] = {
	0x2400, 0x240B, 0x2416
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
	uint8_t buf[2] = { value >> 8, value & 0xFF };
	uint8_t crc = 0xFF;
	uint8_t polynom = 0x31;
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

int sht3xd_write_command(const struct device *dev, uint16_t cmd)
{
	uint8_t tx_buf[2] = { cmd >> 8, cmd & 0xFF };

	return i2c_write(sht3xd_i2c_device(dev), tx_buf, sizeof(tx_buf),
			 sht3xd_i2c_address(dev));
}

int sht3xd_write_reg(const struct device *dev, uint16_t cmd, uint16_t val)
{
	uint8_t tx_buf[5];

	tx_buf[0] = cmd >> 8;
	tx_buf[1] = cmd & 0xFF;
	tx_buf[2] = val >> 8;
	tx_buf[3] = val & 0xFF;
	tx_buf[4] = sht3xd_compute_crc(val);

	return i2c_write(sht3xd_i2c_device(dev), tx_buf, sizeof(tx_buf),
			 sht3xd_i2c_address(dev));
}

static int sht3xd_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct sht3xd_data *data = dev->data;
	const struct device *i2c = sht3xd_i2c_device(dev);
	uint8_t address = sht3xd_i2c_address(dev);
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

	if (i2c_read(i2c, rx_buf, sizeof(rx_buf), address) < 0) {
		LOG_DBG("Failed to read data sample!");
		return -EIO;
	}
#endif
#ifdef CONFIG_SHT3XD_PERIODIC_MODE
	uint8_t tx_buf[2] = {
		SHT3XD_CMD_FETCH >> 8,
		SHT3XD_CMD_FETCH & 0xFF
	};

	if (i2c_write_read(i2c, address, tx_buf, sizeof(tx_buf),
			   rx_buf, sizeof(rx_buf)) < 0) {
		LOG_DBG("Failed to read data sample!");
		return -EIO;
	}
#endif

	t_sample = (rx_buf[0] << 8) | rx_buf[1];
	if (sht3xd_compute_crc(t_sample) != rx_buf[2]) {
		LOG_DBG("Received invalid temperature CRC!");
		return -EIO;
	}

	rh_sample = (rx_buf[3] << 8) | rx_buf[4];
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

static const struct sensor_driver_api sht3xd_driver_api = {
#ifdef CONFIG_SHT3XD_TRIGGER
	.attr_set = sht3xd_attr_set,
	.trigger_set = sht3xd_trigger_set,
#endif
	.sample_fetch = sht3xd_sample_fetch,
	.channel_get = sht3xd_channel_get,
};

static int sht3xd_init(const struct device *dev)
{
	struct sht3xd_data *data = dev->data;
	const struct sht3xd_config *cfg = dev->config;
	const struct device *i2c = device_get_binding(cfg->bus_name);

	if (i2c == NULL) {
		LOG_DBG("Failed to get pointer to %s device!",
			cfg->bus_name);
		return -EINVAL;
	}
	data->bus = i2c;

	if (!cfg->base_address) {
		LOG_DBG("No I2C address");
		return -EINVAL;
	}
	data->dev = dev;

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
	if (sht3xd_init_interrupt(dev) < 0) {
		LOG_DBG("Failed to initialize interrupt");
		return -EIO;
	}
#endif

	return 0;
}

struct sht3xd_data sht3xd0_driver;
static const struct sht3xd_config sht3xd0_cfg = {
	.bus_name = DT_INST_BUS_LABEL(0),
#ifdef CONFIG_SHT3XD_TRIGGER
	.alert_gpio_name = DT_INST_GPIO_LABEL(0, alert_gpios),
#endif
	.base_address = DT_INST_REG_ADDR(0),
#ifdef CONFIG_SHT3XD_TRIGGER
	.alert_pin = DT_INST_GPIO_PIN(0, alert_gpios),
	.alert_flags = DT_INST_GPIO_FLAGS(0, alert_gpios),
#endif
};

DEVICE_AND_API_INIT(sht3xd0, DT_INST_LABEL(0),
		    sht3xd_init, &sht3xd0_driver, &sht3xd0_cfg,
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &sht3xd_driver_api);
