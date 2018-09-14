/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TH02_TH02_H_
#define ZEPHYR_DRIVERS_SENSOR_TH02_TH02_H_

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
	u16_t t_sample;
	u16_t rh_sample;
};

#define SYS_LOG_DOMAIN "TH02"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>
#endif /* _SENSOR_TH02_ */
