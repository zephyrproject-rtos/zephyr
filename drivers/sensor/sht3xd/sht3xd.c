/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <i2c.h>
#include <kernel.h>
#include <sensor.h>
#include <misc/__assert.h>

#include "sht3xd.h"

/*
 * CRC algorithm parameters were taken from the
 * "Checksum Calculation" section of the datasheet.
 */
static u8_t sht3xd_compute_crc(u16_t value)
{
	u8_t buf[2] = {value >> 8, value & 0xFF};
	u8_t crc = 0xFF;
	u8_t polynom = 0x31;
	int i, j;

	for (i = 0; i < 2; ++i) {
		crc  = crc ^ buf[i];
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

int sht3xd_write_command(struct sht3xd_data *drv_data, u16_t cmd)
{
	u8_t tx_buf[2] = {cmd >> 8, cmd & 0xFF};

	return i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf),
			 SHT3XD_I2C_ADDRESS);
}

int sht3xd_write_reg(struct sht3xd_data *drv_data, u16_t cmd,
		     u16_t val)
{
	u8_t tx_buf[5];

	tx_buf[0] = cmd >> 8;
	tx_buf[1] = cmd & 0xFF;
	tx_buf[2] = val >> 8;
	tx_buf[3] = val & 0xFF;
	tx_buf[4] = sht3xd_compute_crc(val);

	return i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf),
			 SHT3XD_I2C_ADDRESS);
}

static int sht3xd_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct sht3xd_data *drv_data = dev->driver_data;
	u8_t rx_buf[6];
	u16_t t_sample, rh_sample;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	u8_t tx_buf[2] = {
		SHT3XD_CMD_FETCH >> 8,
		SHT3XD_CMD_FETCH & 0xFF
	};

	struct i2c_msg msgs[2] = {
		{
			.buf = tx_buf,
			.len = sizeof(tx_buf),
			.flags = I2C_MSG_WRITE | I2C_MSG_RESTART,
		},
		{
			.buf = rx_buf,
			.len = sizeof(rx_buf),
			.flags = I2C_MSG_READ | I2C_MSG_STOP,
		},
	};

	if (i2c_transfer(drv_data->i2c, msgs, 2, SHT3XD_I2C_ADDRESS) < 0) {
		SYS_LOG_DBG("Failed to read data sample!");
		return -EIO;
	}

	t_sample = (rx_buf[0] << 8) | rx_buf[1];
	if (sht3xd_compute_crc(t_sample) != rx_buf[2]) {
		SYS_LOG_DBG("Received invalid temperature CRC!");
		return -EIO;
	}

	rh_sample = (rx_buf[3] << 8) | rx_buf[4];
	if (sht3xd_compute_crc(rh_sample) != rx_buf[5]) {
		SYS_LOG_DBG("Received invalid relative humidity CRC!");
		return -EIO;
	}

	drv_data->t_sample = t_sample;
	drv_data->rh_sample = rh_sample;

	return 0;
}

static int sht3xd_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct sht3xd_data *drv_data = dev->driver_data;
	u64_t tmp;

	/*
	 * See datasheet "Conversion of Signal Output" section
	 * for more details on processing sample data.
	 */
	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		/* val = -45 + 175 * sample / (2^16 -1) */
		tmp = 175 * (u64_t)drv_data->t_sample;
		val->val1 = (s32_t)(tmp / 0xFFFF) - 45;
		val->val2 = (1000000 * (tmp % 0xFFFF)) / 0xFFFF;
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		/* val = 100 * sample / (2^16 -1) */
		u32_t tmp2 = 100 * (u32_t)drv_data->rh_sample;
		val->val1 = tmp2 / 0xFFFF;
		/* x * 100000 / 65536 == x * 15625 / 1024 */
		val->val2 = (tmp2 % 0xFFFF) * 15625 / 1024;
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

static int sht3xd_init(struct device *dev)
{
	struct sht3xd_data *drv_data = dev->driver_data;

	drv_data->i2c = device_get_binding(CONFIG_SHT3XD_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device!",
		    CONFIG_SHT3XD_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

	/* clear status register */
	if (sht3xd_write_command(drv_data, SHT3XD_CMD_CLEAR_STATUS) < 0) {
		SYS_LOG_DBG("Failed to clear status register!");
		return -EIO;
	}

	k_busy_wait(SHT3XD_CLEAR_STATUS_WAIT_USEC);

	/* set periodic measurement mode */
	if (sht3xd_write_command(drv_data,
		sht3xd_measure_cmd[SHT3XD_MPS_IDX][SHT3XD_REPEATABILITY_IDX])
		< 0) {
		SYS_LOG_DBG("Failed to set measurement mode!");
		return -EIO;
	}

	k_busy_wait(sht3xd_measure_wait[SHT3XD_REPEATABILITY_IDX]);

#ifdef CONFIG_SHT3XD_TRIGGER
	if (sht3xd_init_interrupt(dev) < 0) {
		SYS_LOG_DBG("Failed to initialize interrupt");
		return -EIO;
	}
#endif

	return 0;
}

struct sht3xd_data sht3xd_driver;

DEVICE_AND_API_INIT(sht3xd, CONFIG_SHT3XD_NAME, sht3xd_init, &sht3xd_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &sht3xd_driver_api);
