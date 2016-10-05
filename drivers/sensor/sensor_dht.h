/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _SENSOR_DHT
#define _SENSOR_DHT

#include <device.h>

#define SYS_LOG_DOMAIN "DHT"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <misc/sys_log.h>

#define DHT_START_SIGNAL_DURATION		18000
#define DHT_SIGNAL_MAX_WAIT_DURATION		100
#define DHT_DATA_BITS_NUM			40

struct dht_data {
	struct device *gpio;
	uint8_t sample[4];
};

#endif
