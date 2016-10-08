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

#ifndef _SENSOR_TH02
#define _SENSOR_TH02

#include <device.h>
#include <misc/util.h>

#define TH02_I2C_DEV_ID         0x40
#define TH02_REG_STATUS         0x00
#define TH02_REG_DATA_H         0x01
#define TH02_REG_DATA_L         0x02
#define TH02_REG_CONFIG         0x03
#define TH02_REG_ID             0x11

#define TH02_STATUS_RDY_MASK    0x01

#define TH02_CMD_MEASURE_HUMI   0x01
#define TH02_CMD_MEASURE_TEMP   0x11

#define TH02_WR_REG_MODE        0xC0
#define TH02_RD_REG_MODE        0x80

struct th02_data {
	struct device *i2c;
	uint16_t t_sample;
	uint16_t rh_sample;
};

#define SYS_LOG_DOMAIN "TH02"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <misc/sys_log.h>
#endif /* _SENSOR_TH02_ */
