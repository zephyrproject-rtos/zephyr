/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SENSOR_MS5837_H__
#define __SENSOR_MS5837_H__

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

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

enum ms5837_type {
	MS5837_02BA01 = 0x00,
	MS5837_02BA21 = 0x15,
	MS5837_30BA26 = 0x1A
};

typedef void (*ms5837_compensate_func)(const struct device *dev,
				       const int32_t adc_temperature,
				       const int32_t adc_pressure);

struct ms5837_data {
	/* Calibration values */
	uint16_t factory;
	uint16_t sens_t1;
	uint16_t off_t1;
	uint16_t tcs;
	uint16_t tco;
	uint16_t t_ref;
	uint16_t tempsens;

	/* Measured values */
	int32_t pressure;
	int32_t temperature;

	/* Conversion commands */
	uint8_t presure_conv_cmd;
	uint8_t temperature_conv_cmd;

	/* Conversion delay in ms*/
	uint8_t presure_conv_delay;
	uint8_t temperature_conv_delay;

	ms5837_compensate_func comp_func;
};

struct ms5837_config {
	struct i2c_dt_spec i2c;
};

#endif /* __SENSOR_MS5837_H__ */
