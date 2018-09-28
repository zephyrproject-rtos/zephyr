/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_DHT_DHT_H_
#define ZEPHYR_DRIVERS_SENSOR_DHT_DHT_H_

#include <device.h>

#define SYS_LOG_DOMAIN "DHT"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#define DHT_START_SIGNAL_DURATION		18000
#define DHT_SIGNAL_MAX_WAIT_DURATION		100
#define DHT_DATA_BITS_NUM			40

struct dht_data {
	struct device *gpio;
	u8_t sample[4];
};

#endif
