/* Bosch BMP280 pressure sensor
 *
 * Copyright (c) 2026 NBuiLe
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet: BST-BMP280-DS001-10 | Revision 1.13 | November 2014
 *
 */
#ifndef ZEPHYR_DRIVER_SENSORS_BMP280_H
#define ZEPHYR_DRIVER_SENSORS_BMP280_H
/* Connecting SDO to GND results in slave address 1110110 (0x76); connection it to */
/* VDDIO results in slave address 1110111 (0x77) */
#define BMP280_I2C_ADDR 0x77U

#define BMP280_CHIPID          0x58U /* The chip ID of the BMP280 is 0x58 */
#define BMP280_SOFTRESET_VALUE 0xB6U /* Softreset value */

#define BMP280_SLEEPMODE        0x00U /* Sleep mode */
#define BMP280_FORCEDMODE       0x01U /* Forced mode */
#define BMP280_NORMALMODE       0x03U /* Normal mode */
#define BMP280_STATUS_MEASURING 0x08U
#define BMP280_STATUS_IM_UPDATE 0x01U

#define BMP280_STANDBYTIME_0 0x00U /* 0.5ms */
#define BMP280_STANDBYTIME_1 0x01U /* 62.5ms */
#define BMP280_STANDBYTIME_2 0x02U /* 125ms */
#define BMP280_STANDBYTIME_3 0x03U /* 250ms */
#define BMP280_STANDBYTIME_4 0x04U /* 500ms */
#define BMP280_STANDBYTIME_5 0x05U /* 1000ms */
#define BMP280_STANDBYTIME_6 0x06U /* 2000ms */
#define BMP280_STANDBYTIME_7 0x07U /* 4000ms */

#define BMP280_ULTRA_LOW_POWER       0x01U /* x1 16 bit / 2.62 Pa / 0.0050 °C */
#define BMP280_LOW_POWER             0x02U /* x2 17 bit / 1.31 Pa / 0.0025 °C */
#define BMP280_STANDARD_RESOLUTION   0x03U /* x4 18 bit / 0.66 Pa / 0.0012 °C */
#define BMP280_HIGH_RESOLUTION       0x04U /* x8 19 bit / 0.33 Pa / 0.0006 °C */
#define BMP280_ULTRA_HIGH_RESOLUTION 0x05U /* x16 20 bit / 0.16 Pa / 0.0003 °C */

#define BMP280_TEMP_XLSB  0xFCU
#define BMP280_TEMP_LSB   0xFBU
#define BMP280_TEMP_MSB   0xFAU
#define BMP280_PRESS_XLSB 0xF9U
#define BMP280_PRESS_LSB  0xF8U
#define BMP280_PRESS_MSB  0xF7U
#define BMP280_CONFIG     0xF5U
#define BMP280_REG_CALIB0 0x88U
#define BMP280_CTRL_MEAS  0xF4U
#define BMP280_STATUS     0xF3U
#define BMP280_RESET      0xE0U
#define BMP280_ID         0xD0U

/* calibration Registers structure */
struct bmp280_calib_data {
	uint16_t dig_T1;
	int16_t dig_T2;
	int16_t dig_T3;
	uint16_t dig_P1;
	int16_t dig_P2;
	int16_t dig_P3;
	int16_t dig_P4;
	int16_t dig_P5;
	int16_t dig_P6;
	int16_t dig_P7;
	int16_t dig_P8;
	int16_t dig_P9;
};

struct bmp280_config {
	const struct i2c_dt_spec i2c;
};

struct bmp280_comp_sample {
	/* Compensated values. */
	uint32_t comp_press; /* Pa */
	int32_t comp_temp;   /* 0.01 degree Celsius */
};

struct bmp280_data {
	/* 3.3.2 Temperature measurement */
	uint8_t osrs_t;
	/* 3.3.1 Pressure measurement */
	uint8_t osrs_p;
	/* Controls the power mode of the device. See chapter 3.6 for details. */
	uint8_t mode;
	/* Controls inactive duration tstandby in normal mode. See chapter 3.6.3 for details. */
	uint8_t t_sb;
	/* Controls the time constant of the IIR filter. See chapter 3.3.3 for details. */
	uint8_t filter;
	/* Enables 3-wire SPI interface when set to ‘1’. See chapter 5.3 for details. */
	uint8_t spi3w_en;
	struct bmp280_calib_data calib;
	struct bmp280_comp_sample sample;
};

#endif /* ZEPHYR_DRIVER_SENSORS_BMP280_H */
