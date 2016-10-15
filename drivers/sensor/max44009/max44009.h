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

#ifndef _SENSOR_MAX44009
#define _SENSOR_MAX44009

#include <misc/util.h>

#define SYS_LOG_DOMAIN "MAX44009"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <misc/sys_log.h>

#define MAX44009_I2C_ADDRESS	CONFIG_MAX44009_I2C_ADDR

#define MAX44009_SAMPLING_CONTROL_BIT	BIT(7)
#define MAX44009_CONTINUOUS_SAMPLING	BIT(7)
#define MAX44009_SAMPLE_EXPONENT_SHIFT	12
#define MAX44009_MANTISSA_HIGH_NIBBLE_MASK	0xf00
#define MAX44009_MANTISSA_LOW_NIBBLE_MASK	0xf

#define MAX44009_REG_CONFIG		0x02
#define MAX44009_REG_LUX_HIGH_BYTE	0x03
#define MAX44009_REG_LUX_LOW_BYTE	0x04

struct max44009_data {
	struct device *i2c;
	uint16_t sample;
};

#endif /* _SENSOR_MAX44009_ */
