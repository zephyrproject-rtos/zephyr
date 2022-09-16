/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aosong_dht

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/sensor.h>
#include <string.h>
#include <zephyr/zephyr.h>
#include <zephyr/logging/log.h>

#include "dht.h"

LOG_MODULE_REGISTER(DHT, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Measure duration of signal send by sensor
 *
 * @param drv_data Pointer to the driver data structure
 * @param active Whether current signal is active
 *
 * @return duration in usec of signal being measured,
 *         -1 if duration exceeds DHT_SIGNAL_MAX_WAIT_DURATION
 */
static int8_t dht_measure_signal_duration(const struct device *dev,
	       	                   bool active)
{
	struct dht_data *drv_data = dev->data;
	const struct dht_config *cfg = dev->config;
	uint32_t elapsed_cycles;
	uint32_t max_wait_cycles = (uint32_t)(
		(uint64_t)DHT_SIGNAL_MAX_WAIT_DURATION *
		(uint64_t)sys_clock_hw_cycles_per_sec() /
		(uint64_t)USEC_PER_SEC
	);
	uint32_t start_cycles = k_cycle_get_32();
	int rc;

	do {
		rc = gpio_pin_get(drv_data->gpio, cfg->pin);
		elapsed_cycles = k_cycle_get_32() - start_cycles;

		if ((rc < 0)
		    || (elapsed_cycles > max_wait_cycles)) {
			return -1;
		}
	} while ((bool)rc == active);

	return (uint64_t)elapsed_cycles *
	       (uint64_t)USEC_PER_SEC /
	       (uint64_t)sys_clock_hw_cycles_per_sec();
}

static int dht_sample_fetch(const struct device *dev,
			    enum sensor_channel chan)
{
	struct dht_data *drv_data = dev->data;
	const struct dht_config *cfg = dev->config;
	int ret = 0;
	int8_t signal_duration[DHT_DATA_BITS_NUM];
	int8_t max_duration, min_duration, avg_duration;
	uint8_t buf[5];
	unsigned int i, j;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* assert to send start signal */
	gpio_pin_set(drv_data->gpio, cfg->pin, true);

	k_busy_wait(DHT_START_SIGNAL_DURATION);

	gpio_pin_set(drv_data->gpio, cfg->pin, false);

	/* switch to DIR_IN to read sensor signals */
	gpio_pin_configure(drv_data->gpio, cfg->pin,
			   GPIO_INPUT | cfg->flags);

	/* wait for sensor active response */
	if (dht_measure_signal_duration(dev, false) == -1) {
		ret = -EIO;
		goto cleanup;
	}

	/* read sensor response */
	if (dht_measure_signal_duration(dev, true) == -1) {
		ret = -EIO;
		goto cleanup;
	}

	/* wait for sensor data start */
	if (dht_measure_signal_duration(dev, false) == -1) {
		ret = -EIO;
		goto cleanup;
	}

	/* read sensor data */
	for (i = 0U; i < DHT_DATA_BITS_NUM; i++) {
		/* Active signal to indicate a new bit */
		if (dht_measure_signal_duration(dev, true) == -1) {
			ret = -EIO;
			goto cleanup;
		}

		/* Inactive signal duration indicates bit value */
		signal_duration[i] = dht_measure_signal_duration(dev, false);
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
	for (i = 1U; i < DHT_DATA_BITS_NUM; i++) {
		if (min_duration > signal_duration[i]) {
			min_duration = signal_duration[i];
		}
		if (max_duration < signal_duration[i]) {
			max_duration = signal_duration[i];
		}
	}
	avg_duration = ((int16_t)min_duration + (int16_t)max_duration) / 2;

	/* store bits in buf */
	j = 0U;
	(void)memset(buf, 0, sizeof(buf));
	for (i = 0U; i < DHT_DATA_BITS_NUM; i++) {
		if (signal_duration[i] >= avg_duration) {
			buf[j] = (buf[j] << 1) | 1;
		} else {
			buf[j] = buf[j] << 1;
		}

		if (i % 8 == 7U) {
			j++;
		}
	}

	/* verify checksum */
	if (((buf[0] + buf[1] + buf[2] + buf[3]) & 0xFF) != buf[4]) {
		LOG_DBG("Invalid checksum in fetched sample");
		ret = -EIO;
	} else {
		memcpy(drv_data->sample, buf, 4);
	}

cleanup:
	/* Switch to output inactive until next fetch. */
	gpio_pin_configure(drv_data->gpio, cfg->pin,
			   GPIO_OUTPUT_INACTIVE | cfg->flags);

	return ret;
}

static int dht_channel_get(const struct device *dev,
			   enum sensor_channel chan,
			   struct sensor_value *val)
{
	struct dht_data *drv_data = dev->data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_AMBIENT_TEMP
			|| chan == SENSOR_CHAN_HUMIDITY);

	/* see data calculation example from datasheet */
	if (IS_ENABLED(DT_INST_PROP(0, dht22))) {
		/*
		 * use both integral and decimal data bytes; resulted
		 * 16bit data has a resolution of 0.1 units
		 */
		int16_t raw_val, sign;

		if (chan == SENSOR_CHAN_HUMIDITY) {
			raw_val = (drv_data->sample[0] << 8)
				+ drv_data->sample[1];
			val->val1 = raw_val / 10;
			val->val2 = (raw_val % 10) * 100000;
		} else { /* chan == SENSOR_CHAN_AMBIENT_TEMP */
			raw_val = (drv_data->sample[2] << 8)
				+ drv_data->sample[3];

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
	} else {
		/* use only integral data byte */
		if (chan == SENSOR_CHAN_HUMIDITY) {
			val->val1 = drv_data->sample[0];
			val->val2 = 0;
		} else { /* chan == SENSOR_CHAN_AMBIENT_TEMP */
			val->val1 = drv_data->sample[2];
			val->val2 = 0;
		}
	}

	return 0;
}

static const struct sensor_driver_api dht_api = {
	.sample_fetch = &dht_sample_fetch,
	.channel_get = &dht_channel_get,
};

static int dht_init(const struct device *dev)
{
	int rc = 0;
	struct dht_data *drv_data = dev->data;
	const struct dht_config *cfg = dev->config;

	drv_data->gpio = device_get_binding(cfg->ctrl);
	if (drv_data->gpio == NULL) {
		LOG_ERR("Failed to get GPIO device %s.", cfg->ctrl);
		return -EINVAL;
	}

	rc = gpio_pin_configure(drv_data->gpio, cfg->pin,
				GPIO_OUTPUT_INACTIVE | cfg->flags);

	return rc;
}

static struct dht_data dht_data;
static const struct dht_config dht_config = {
	.ctrl = DT_INST_GPIO_LABEL(0, dio_gpios),
	.flags = DT_INST_GPIO_FLAGS(0, dio_gpios),
	.pin = DT_INST_GPIO_PIN(0, dio_gpios),
};

DEVICE_DT_INST_DEFINE(0, &dht_init, NULL,
		    &dht_data, &dht_config,
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &dht_api);
