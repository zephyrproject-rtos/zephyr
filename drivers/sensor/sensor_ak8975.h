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

#ifndef __SENSOR_AK8975__
#define __SENSOR_AK8975__

#include <device.h>

#define SYS_LOG_DOMAIN "AK8975"
#define SYS_LOG_LEVEL CONFIG_AK8975_SYS_LOG_LEVEL
#include <misc/sys_log.h>

#define AK8975_REG_CHIP_ID		0x00
#define AK8975_CHIP_ID			0x48

#define AK8975_REG_DATA_START		0x03

#define AK8975_REG_CNTL			0x0A
#define AK8975_MODE_MEASURE		0x01
#define AK8975_MODE_FUSE_ACCESS		0x0F

#define AK8975_REG_ADJ_DATA_START	0x10

#define AK8975_MEASURE_TIME_US		9000
#define AK8975_MICRO_GAUSS_PER_BIT	3000

struct ak8975_data {
	struct device *i2c;

	int16_t x_sample;
	int16_t y_sample;
	int16_t z_sample;

	uint8_t x_adj;
	uint8_t y_adj;
	uint8_t z_adj;
};

#endif /* __SENSOR_AK8975__ */
