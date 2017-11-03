/*
 * Copyright (c) 2017 Tidy Jiang <tidyjiang@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <i2c.h>
#include <init.h>
#include <misc/__assert.h>
#include <misc/byteorder.h>
#include <sensor.h>
#include <string.h>
#include <crc16.h>
#include <logging/sys_log.h>

/* AM2320 I2C Address, 7 bits */
#define AM2320_I2C_ADDR         0x5C

/**
 * Note: In order to wake up AM2320, we need to send START +
 * ADDRESS(0xB8) signal to it. But I can't find a way to send
 * this signal, so I use API i2c_write() with a arbitrary
 * value (here is 0xB8) to do so.
 */
#define AM2320_WAKEUP_VALUE     0xB8

/* AM2320 Function Code, Read/Write */
#define AM2320_FUNCCODE_READ    0x03
#define AM2320_FUNCCODE_WRITE   0X10

/* Register's Start address and Length to fetch data. */
#define AM2320_FETCH_START_ADDR 0X00
#define AM2320_FETCH_LEN        0X04

struct am2320_data {
	struct device *i2c;
	s16_t temp;
	s16_t humidity;
};

static u16_t am2320_crc16(u8_t *ptr, unsigned int len)
{
	u16_t   crc = 0xFFFF;
	u8_t    i;

	while (len--) {
		crc ^= *ptr++;

		for (i = 0; i < 8; i++) {
			if (crc & 0x01) {
				crc >>= 1;
				crc ^= 0xA001;
			} else {
				crc >>= 1;
			}
		}
	}

	return crc;
}

static int am2320_channel_get(struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct am2320_data *drv_data = dev->driver_data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_TEMP || SENSOR_CHAN_HUMIDITY);

	if (chan == SENSOR_CHAN_TEMP) {
		val->val1 = drv_data->temp / 10;
		val->val2 = drv_data->temp % 10 * 100000;
	} else { /* SENSOR_CHAN_HUMIDITY */
		val->val1 = drv_data->humidity * 100;
		val->val2 = 0;
	}

	return 0;
}

static int am2320_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct am2320_data *drv_data = dev->driver_data;
	u8_t   buf[8];
	u16_t  crc;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/**
	 * Step 1: Wake up AM2320.
	 * We do not check the return value, as AM2320 will not
	 * response ACK signal.
	 */
	buf[0] = AM2320_WAKEUP_VALUE;
	i2c_write(drv_data->i2c, buf, 1, AM2320_I2C_ADDR);

	/* Step 2: Send Read Command to AM2320. */
	buf[0] = AM2320_FUNCCODE_READ;
	buf[1] = AM2320_FETCH_START_ADDR;
	buf[2] = AM2320_FETCH_LEN;
	if (i2c_write(drv_data->i2c, buf, 3, AM2320_I2C_ADDR)) {
		SYS_LOG_ERR("Failed to fetch data sample.");
		return -EIO;
	}

	/* Step: 3: Read data from AM2320. */
	memset(buf, 0, sizeof(buf));
	if (i2c_read(drv_data->i2c, buf, 8, AM2320_I2C_ADDR)) {
		SYS_LOG_ERR("Failed to fetch data sample.");
		return -EIO;
	}

	/* Step 4: verify data.  */
	crc = (buf[7] << 8) + buf[6];
	if (crc != am2320_crc16(buf, 6)) {
		SYS_LOG_ERR("Failed to fetch data sample.");
		return -EIO;
	}

	/* Step 5: store data to driver structure. */
	drv_data->temp     = (buf[4] << 8) + buf[5];
	drv_data->humidity = (buf[2] << 8) + buf[3];

	return 0;
}

static const struct sensor_driver_api am2320_driver_api = {
	.sample_fetch = am2320_sample_fetch,
	.channel_get  = am2320_channel_get,
};

int am2320_init(struct device *dev)
{
	struct am2320_data *drv_data = dev->driver_data;

	drv_data->i2c = device_get_binding(CONFIG_AM2320_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		SYS_LOG_ERR("Could not get pointer to %s device.",
			    CONFIG_AM2320_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

	dev->driver_api = &am2320_driver_api;

	return 0;
}

struct am2320_data am2320_driver;

DEVICE_INIT(am2320, CONFIG_AM2320_NAME, am2320_init, &am2320_driver,
	    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);
