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

#include <nanokernel.h>

#if CONFIG_HDC1008_I2C_ADDR_0
	#define HDC1008_I2C_ADDRESS	0x40
#elif CONFIG_HDC1008_I2C_ADDR_1
	#define HDC1008_I2C_ADDRESS	0x41
#elif CONFIG_HDC1008_I2C_ADDR_2
	#define HDC1008_I2C_ADDRESS	0x42
#elif CONFIG_HDC1008_I2C_ADDR_3
	#define HDC1008_I2C_ADDRESS	0x43
#endif

#define HDC1008_REG_TEMP	0x0
#define HDC1008_REG_HUMIDITY	0x1

struct hdc1008_data {
	struct device *i2c;
	struct device *gpio;
	uint16_t t_sample;
	uint16_t rh_sample;
	struct nano_sem data_sem;
};

#endif
