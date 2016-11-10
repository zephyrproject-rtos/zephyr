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

#ifndef _SENSOR_HDC1008
#define _SENSOR_HDC1008

#include <kernel.h>

#define HDC1008_I2C_ADDRESS	0x40

#define HDC1008_REG_TEMP	0x0
#define HDC1008_REG_HUMIDITY	0x1
#define HDC1000_MANUFID         0xFE
#define HDC1000_DEVICEID        0xFF

struct hdc1008_data {
	struct device *i2c;
	struct device *gpio;
	struct gpio_callback gpio_cb;
	uint16_t t_sample;
	uint16_t rh_sample;
	struct k_sem data_sem;
};

#define SYS_LOG_DOMAIN "HDC1008"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <misc/sys_log.h>
#endif
