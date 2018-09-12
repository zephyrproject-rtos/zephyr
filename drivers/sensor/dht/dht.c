/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <gpio.h>
#include <misc/byteorder.h>
#include <misc/util.h>
#include <sensor.h>
#include <string.h>
#include <zephyr.h>

#include "dht.h"

/**
 * @brief Measure duration of signal send by sensor
 *
 * @param drv_data Pointer to the driver data structure
 * @param signal_val Value of signal being measured
 *
 * @return duration in usec of signal being measured,
 *         -1 if duration exceeds DHT_SIGNAL_MAX_WAIT_DURATION
 */
static s8_t dht_measure_signal_duration(struct dht_data *drv_data,
					  u32_t signal_val)
{
	u32_t val;
	u32_t elapsed_cycles;
	u32_t max_wait_cycles = (u32_t)(
		(u64_t)DHT_SIGNAL_MAX_WAIT_DURATION *
		(u64_t)sys_clock_hw_cycles_per_sec /
		(u64_t)USEC_PER_SEC
	);
	u32_t start_cycles = k_cycle_get_32();

	do {
		gpio_pin_read(drv_data->gpio, CONFIG_DHT_GPIO_PIN_NUM, &val);
		elapsed_cycles = k_cycle_get_32() - start_cycles;

		if (elapsed_cycles > max_wait_cycles) {
			return -1;
		}
	} while (val == signal_val);

	return (u64_t)elapsed_cycles *
	       (u64_t)USEC_PER_SEC /
	       (u64_t)sys_clock_hw_cycles_per_sec;
}

static int dht_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct dht_data *drv_data = dev->driver_data;
	int ret = 0;
	s8_t signal_duration[DHT_DATA_BITS_NUM];
	s8_t max_duration, min_duration, avg_duration;
	u8_t buf[5];
	unsigned int i, j;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* send start signal */
	gpio_pin_write(drv_data->gpio, CONFIG_DHT_GPIO_PIN_NUM, 0);

	k_busy_wait(DHT_START_SIGNAL_DURATION);

	gpio_pin_write(drv_data->gpio, CONFIG_DHT_GPIO_PIN_NUM, 1);

	/* switch to DIR_IN to read sensor signals */
	gpio_pin_configure(drv_data->gpio, CONFIG_DHT_GPIO_PIN_NUM,
			   GPIO_DIR_IN);

	/* wait for sensor response */
	if (dht_measure_signal_duration(drv_data, 1) == -1) {
		ret = -EIO;
		goto cleanup;
	}

	/* read sensor response */
	if (dht_measure_signal_duration(drv_data, 0) == -1) {
		ret = -EIO;
		goto cleanup;
	}

	/* wait for sensor data */
	if (dht_measure_signal_duration(drv_data, 1) == -1) {
		ret = -EIO;
		goto cleanup;
	}

	/* read sensor data */
	for (i = 0; i < DHT_DATA_BITS_NUM; i++) {
		/* LOW signal to indicate a new bit */
		if (dht_measure_signal_duration(drv_data, 0) == -1) {
			ret = -EIO;
			goto cleanup;
		}

		/* HIGH signal duration indicates bit value */
		signal_duration[i] = dht_measure_signal_duration(drv_data, 1);
		if (signal_duration[i] == -1) {
			ret = -EIO;
			goto cleanup;
		}
	}

	/*
	 * the datasheet says 20-40us HIGH signal duration for a 0 bit and
	 * 80us for a 1 bit; however, since dht_measure_signal_duration is
	 * not very precise, compute the threshold for deciding between a
	 * 0 bit and a 1 bit as the average between the minimum and maximum
	 * if the durations stored in signal_duration
	 */
	min_duration = signal_duration[0];
	max_duration = signal_duration[0];
	for (i = 1; i < DHT_DATA_BITS_NUM; i++) {
		if (min_duration > signal_duration[i]) {
			min_duration = signal_duration[i];
		}
		if (max_duration < signal_duration[i]) {
			max_duration = signal_duration[i];
		}
	}
	avg_duration = ((s16_t)min_duration + (s16_t)max_duration) / 2;

	/* store bits in buf */
	j = 0;
	(void)memset(buf, 0, sizeof(buf));
	for (i = 0; i < DHT_DATA_BITS_NUM; i++) {
		if (signal_duration[i] >= avg_duration) {
			buf[j] = (buf[j] << 1) | 1;
		} else {
			buf[j] = buf[j] << 1;
		}

		if (i % 8 == 7) {
			j++;
		}
	}

	/* verify checksum */
	if (((buf[0] + buf[1] + buf[2] + buf[3]) & 0xFF) != buf[4]) {
		SYS_LOG_DBG("Invalid checksum in fetched sample");
		ret = -EIO;
	} else {
		memcpy(drv_data->sample, buf, 4);
	}

cleanup:
	/* switch to DIR_OUT and leave pin to HIGH until next fetch */
	gpio_pin_configure(drv_data->gpio, CONFIG_DHT_GPIO_PIN_NUM,
			   GPIO_DIR_OUT);
	gpio_pin_write(drv_data->gpio, CONFIG_DHT_GPIO_PIN_NUM, 1);

	return ret;
}

static int dht_channel_get(struct device *dev,
			   enum sensor_channel chan,
			   struct sensor_value *val)
{
	struct dht_data *drv_data = dev->driver_data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_AMBIENT_TEMP || chan == SENSOR_CHAN_HUMIDITY);

	/* see data calculation example from datasheet */
#if defined(CONFIG_DHT_CHIP_DHT11)
	/* use only integral data byte */
	if (chan == SENSOR_CHAN_HUMIDITY) {
		val->val1 = drv_data->sample[0];
		val->val2 = 0;
	} else { /* chan == SENSOR_CHAN_AMBIENT_TEMP */
		val->val1 = drv_data->sample[2];
		val->val2 = 0;
	}
#elif defined(CONFIG_DHT_CHIP_DHT22)
	/*
	 * use both integral and decimal data bytes; resulted 16bit data has
	 * a resolution of 0.1 units
	 */
	s16_t raw_val, sign;

	if (chan == SENSOR_CHAN_HUMIDITY) {
		raw_val = (drv_data->sample[0] << 8) | drv_data->sample[1];
		val->val1 = raw_val / 10;
		val->val2 = (raw_val % 10) * 100000;
	} else { /* chan == SENSOR_CHAN_AMBIENT_TEMP */
		raw_val = (drv_data->sample[2] << 8) | drv_data->sample[3];

		sign = raw_val & 0x8000;
		raw_val = raw_val & ~0x8000;

		val->val1 = raw_val / 10;
		val->val2 = (raw_val % 10) * 100000;

		/* handle negative value */
		if (sign) {
			val->val1 = -val->val1;
			val->val2 = -val->val2;
		}
	}
#endif

	return 0;
}

static const struct sensor_driver_api dht_api = {
	.sample_fetch = &dht_sample_fetch,
	.channel_get = &dht_channel_get,
};

static int dht_init(struct device *dev)
{
	struct dht_data *drv_data = dev->driver_data;

	drv_data->gpio = device_get_binding(CONFIG_DHT_GPIO_DEV_NAME);
	if (drv_data->gpio == NULL) {
		SYS_LOG_ERR("Failed to get GPIO device.");
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, CONFIG_DHT_GPIO_PIN_NUM,
			   GPIO_DIR_OUT);

	gpio_pin_write(drv_data->gpio, CONFIG_DHT_GPIO_PIN_NUM, 1);

	return 0;
}

struct dht_data dht_data;

DEVICE_AND_API_INIT(dht_dev, CONFIG_DHT_NAME, &dht_init, &dht_data,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &dht_api);
