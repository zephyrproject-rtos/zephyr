/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_DHT_DHT_H_
#define ZEPHYR_DRIVERS_SENSOR_DHT_DHT_H_

#include <zephyr/device.h>

#define DHT_START_SIGNAL_DURATION		18000
#define DHT_SIGNAL_MAX_WAIT_DURATION		100
#define DHT_DATA_BITS_NUM			40

struct dht_data {
	const struct device *gpio;
	uint8_t sample[4];
};

struct dht_config {
	const char *ctrl;
	gpio_dt_flags_t flags;
	gpio_pin_t pin;
};

#endif
