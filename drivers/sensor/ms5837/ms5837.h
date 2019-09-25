/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SENSOR_MS5837_H__
#define __SENSOR_MS5837_H__

#include <zephyr/types.h>
#include <device.h>
#include <drivers/i2c.h>

#define MS5837_CMD_RESET 0x1E

#define MS5837_CMD_CONV_P_256  0x40
#define MS5837_CMD_CONV_P_512  0x42
#define MS5837_CMD_CONV_P_1024 0x44
#define MS5837_CMD_CONV_P_2048 0x46
#define MS5837_CMD_CONV_P_4096 0x48
#define MS5837_CMD_CONV_P_8192 0x4A

#define MS5837_CMD_CONV_T_256  0x50
#define MS5837_CMD_CONV_T_512  0x52
#define MS5837_CMD_CONV_T_1024 0x54
#define MS5837_CMD_CONV_T_2048 0x56
#define MS5837_CMD_CONV_T_4096 0x58
#define MS5837_CMD_CONV_T_8192 0x5A

#define MS5837_CMD_CONV_READ_ADC 0x00

#define MS5837_CMD_CONV_READ_CRC      0xA0
#define MS5837_CMD_CONV_READ_SENS_T1  0xA2
#define MS5837_CMD_CONV_READ_OFF_T1   0xA4
#define MS5837_CMD_CONV_READ_TCS      0xA6
#define MS5837_CMD_CONV_READ_TCO      0xA8
#define MS5837_CMD_CONV_READ_T_REF    0xAA
#define MS5837_CMD_CONV_READ_TEMPSENS 0xAC

#define MS5837_ADC_READ_DELAY_256  1
#define MS5837_ADC_READ_DELAY_512  2
#define MS5837_ADC_READ_DELAY_1024 3
#define MS5837_ADC_READ_DELAY_2048 5
#define MS5837_ADC_READ_DELAY_4086 10
#define MS5837_ADC_READ_DELAY_8129 20

struct ms5837_data {

	struct device *i2c_master;

	/* Calibration values */
	u16_t sens_t1;
	u16_t off_t1;
	u16_t tcs;
	u16_t tco;
	u16_t t_ref;
	u16_t tempsens;

	/* Measured values */
	s32_t pressure;
	s32_t temperature;

	/* Conversion commands */
	u8_t presure_conv_cmd;
	u8_t temperature_conv_cmd;

	/* Conversion delay in ms*/
	u8_t presure_conv_delay;
	u8_t temperature_conv_delay;

};

struct ms5837_config {
	const char *i2c_name;
	u8_t i2c_address;
};

#endif /* __SENSOR_MS5837_H__ */
