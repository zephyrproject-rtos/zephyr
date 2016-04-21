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

#ifndef __SENSOR_BMP280_H__
#define __SENSOR_BMP280_H__

#include <stdint.h>
#include <device.h>

#define BMP280_REG_PRESS_MSB		0xF7
#define BMP280_REG_COMP_START		0x88
#define BMP280_REG_ID			0xD0
#define BMP280_REG_CONFIG		0xF5
#define BMP280_REG_CTRL_MEAS		0xF4

#define BMP280_CHIP_ID			0x58
#define BMP280_MODE_NORMAL		0x03
#define BMP280_SPI_3W_DISABLE		0x00

#if defined CONFIG_BMP280_TEMP_OVER_1X
#define BMP280_TEMP_OVER		(1 << 5)
#elif defined CONFIG_BMP280_TEMP_OVER_2X
#define BMP280_TEMP_OVER		(2 << 5)
#elif defined CONFIG_BMP280_TEMP_OVER_4X
#define BMP280_TEMP_OVER		(3 << 5)
#elif defined CONFIG_BMP280_TEMP_OVER_8X
#define BMP280_TEMP_OVER		(4 << 5)
#elif defined CONFIG_BMP280_TEMP_OVER_16X
#define BMP280_TEMP_OVER		(5 << 5)
#endif

#if defined CONFIG_BMP280_PRESS_OVER_1X
#define BMP280_PRESS_OVER		(1 << 2)
#elif defined CONFIG_BMP280_PRESS_OVER_2X
#define BMP280_PRESS_OVER		(2 << 2)
#elif defined CONFIG_BMP280_PRESS_OVER_4X
#define BMP280_PRESS_OVER		(3 << 2)
#elif defined CONFIG_BMP280_PRESS_OVER_8X
#define BMP280_PRESS_OVER		(4 << 2)
#elif defined CONFIG_BMP280_PRESS_OVER_16X
#define BMP280_PRESS_OVER		(5 << 2)
#endif

#if defined CONFIG_BMP280_STANDBY_05MS
#define BMP280_STANDBY			0
#elif defined CONFIG_BMP280_STANDBY_62MS
#define BMP280_STANDBY			(1 << 5)
#elif defined CONFIG_BMP280_STANDBY_125MS
#define BMP280_STANDBY			(2 << 5)
#elif defined CONFIG_BMP280_STANDBY_250MS
#define BMP280_STANDBY			(3 << 5)
#elif defined CONFIG_BMP280_STANDBY_500MS
#define BMP280_STANDBY			(4 << 5)
#elif defined CONFIG_BMP280_STANDBY_1000MS
#define BMP280_STANDBY			(5 << 5)
#elif defined CONFIG_BMP280_STANDBY_2000MS
#define BMP280_STANDBY			(6 << 5)
#elif defined CONFIG_BMP280_STANDBY_4000MS
#define BMP280_STANDBY			(7 << 5)
#endif

#if defined CONFIG_BMP280_FILTER_OFF
#define BMP280_FILTER			0
#elif defined CONFIG_BMP280_FILTER_2
#define BMP280_FILTER			(1 << 2)
#elif defined CONFIG_BMP280_FILTER_4
#define BMP280_FILTER			(2 << 2)
#elif defined CONFIG_BMP280_FILTER_8
#define BMP280_FILTER			(3 << 2)
#elif defined CONFIG_BMP280_FILTER_16
#define BMP280_FILTER			(4 << 2)
#endif

#define BMP280_CTRL_MEAS_VAL		(BMP280_PRESS_OVER |	\
					 BMP280_TEMP_OVER |	\
					 BMP280_MODE_NORMAL)
#define BMP280_CONFIG_VAL		(BMP280_STANDBY |	\
					 BMP280_FILTER |	\
					 BMP280_SPI_3W_DISABLE)

struct bmp280_data {
	struct device *i2c_master;
	uint16_t i2c_slave_addr;

	/* Compensation parameters. */
	uint16_t dig_t1;
	int16_t dig_t2;
	int16_t dig_t3;
	uint16_t dig_p1;
	int16_t dig_p2;
	int16_t dig_p3;
	int16_t dig_p4;
	int16_t dig_p5;
	int16_t dig_p6;
	int16_t dig_p7;
	int16_t dig_p8;
	int16_t dig_p9;

	/* Compensated values. */
	int32_t comp_temp;
	uint32_t comp_press;

	/* Carryover between temperature and pressure compensation. */
	int32_t t_fine;
};

#define SYS_LOG_DOMAIN "BMP280"
#define SYS_LOG_LEVEL CONFIG_BMP280_SYS_LOG_LEVEL
#include <misc/sys_log.h>
#endif /* __SENSOR_BMP280_H__ */
