/*
 * Copyright (c) 2022 Badgerd Technologies B.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver is developed to be used with Zephyr. And it only supports i2c interface.
 *
 * Author: Talha Can Havadar <havadartalha@gmail.com>
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMP581_USER_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMP581_USER_H_

#include <zephyr/drivers/sensor.h>

#define BMP5_SEA_LEVEL_PRESSURE_PA 101325

/* ODR settings */
#define BMP5_ODR_240_HZ	  0x00
#define BMP5_ODR_218_5_HZ 0x01
#define BMP5_ODR_199_1_HZ 0x02
#define BMP5_ODR_179_2_HZ 0x03
#define BMP5_ODR_160_HZ	  0x04
#define BMP5_ODR_149_3_HZ 0x05
#define BMP5_ODR_140_HZ	  0x06
#define BMP5_ODR_129_8_HZ 0x07
#define BMP5_ODR_120_HZ	  0x08
#define BMP5_ODR_110_1_HZ 0x09
#define BMP5_ODR_100_2_HZ 0x0A
#define BMP5_ODR_89_6_HZ  0x0B
#define BMP5_ODR_80_HZ	  0x0C
#define BMP5_ODR_70_HZ	  0x0D
#define BMP5_ODR_60_HZ	  0x0E
#define BMP5_ODR_50_HZ	  0x0F
#define BMP5_ODR_45_HZ	  0x10
#define BMP5_ODR_40_HZ	  0x11
#define BMP5_ODR_35_HZ	  0x12
#define BMP5_ODR_30_HZ	  0x13
#define BMP5_ODR_25_HZ	  0x14
#define BMP5_ODR_20_HZ	  0x15
#define BMP5_ODR_15_HZ	  0x16
#define BMP5_ODR_10_HZ	  0x17
#define BMP5_ODR_05_HZ	  0x18
#define BMP5_ODR_04_HZ	  0x19
#define BMP5_ODR_03_HZ	  0x1A
#define BMP5_ODR_02_HZ	  0x1B
#define BMP5_ODR_01_HZ	  0x1C
#define BMP5_ODR_0_5_HZ	  0x1D
#define BMP5_ODR_0_250_HZ 0x1E
#define BMP5_ODR_0_125_HZ 0x1F

/* Oversampling for temperature and pressure */
#define BMP5_OVERSAMPLING_1X   0x00
#define BMP5_OVERSAMPLING_2X   0x01
#define BMP5_OVERSAMPLING_4X   0x02
#define BMP5_OVERSAMPLING_8X   0x03
#define BMP5_OVERSAMPLING_16X  0x04
#define BMP5_OVERSAMPLING_32X  0x05
#define BMP5_OVERSAMPLING_64X  0x06
#define BMP5_OVERSAMPLING_128X 0x07

/* IIR filter for temperature and pressure */
#define BMP5_IIR_FILTER_BYPASS	  0x00
#define BMP5_IIR_FILTER_COEFF_1	  0x01
#define BMP5_IIR_FILTER_COEFF_3	  0x02
#define BMP5_IIR_FILTER_COEFF_7	  0x03
#define BMP5_IIR_FILTER_COEFF_15  0x04
#define BMP5_IIR_FILTER_COEFF_31  0x05
#define BMP5_IIR_FILTER_COEFF_63  0x06
#define BMP5_IIR_FILTER_COEFF_127 0x07

/* Custom ATTR values */

/* This is used to enable IIR config,
 * keep in mind that disabling IIR back in runtime is not
 * supported yet
 */
#define BMP5_ATTR_IIR_CONFIG (SENSOR_ATTR_PRIV_START + 1u)
#define BMP5_ATTR_POWER_MODE (SENSOR_ATTR_PRIV_START + 2u)

enum bmp5_powermode {
	/*! Standby powermode */
	BMP5_POWERMODE_STANDBY,
	/*! Normal powermode */
	BMP5_POWERMODE_NORMAL,
	/*! Forced powermode */
	BMP5_POWERMODE_FORCED,
	/*! Continuous powermode */
	BMP5_POWERMODE_CONTINUOUS,
	/*! Deep standby powermode */
	BMP5_POWERMODE_DEEP_STANDBY
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMP581_USER_H_ */
