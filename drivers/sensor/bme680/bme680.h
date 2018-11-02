/*
 * Copyright (c) 2018 Laird
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BME680_BME680_H_
#define ZEPHYR_DRIVERS_SENSOR_BME680_BME680_H_

#include <zephyr/types.h>
#include <device.h>

/* Coefficient's address */
#define BME680_COEFF_ADDR1			0x89
#define BME680_COEFF_ADDR2			0xe1

/* BME680 coefficients related defines */
#define BME680_COEFF_SIZE			41
#define BME680_COEFF_ADDR1_LEN			25
#define BME680_COEFF_ADDR2_LEN			16

/* BME680 chip ID register */
#define BME680_REG_ID				0xD0

/* BME680 unique chip identifier */
#define BME680_CHIP_ID				0x61

#if defined(CONFIG_BME680_TEMP_OVER_1X)
#define BME680_TEMP_OVER			0b00100000
#elif defined(CONFIG_BME680_TEMP_OVER_2X)
#define BME680_TEMP_OVER			0b01000000
#elif defined(CONFIG_BME680_TEMP_OVER_4X)
#define BME680_TEMP_OVER			0b01100000
#elif defined(CONFIG_BME680_TEMP_OVER_8X)
#define BME680_TEMP_OVER			0b10000000
#elif defined(CONFIG_BME680_TEMP_OVER_16X)
#define BME680_TEMP_OVER			0b10100000
#else
#define BME680_TEMP_OVER			0b00000000
#endif

#if defined(CONFIG_BME680_PRESS_OVER_1X)
#define BME680_PRESS_OVER			0b00100
#elif defined(CONFIG_BME680_PRESS_OVER_2X)
#define BME680_PRESS_OVER			0b01000
#elif defined(CONFIG_BME680_PRESS_OVER_4X)
#define BME680_PRESS_OVER			0b01100
#elif defined(CONFIG_BME680_PRESS_OVER_8X)
#define BME680_PRESS_OVER			0b10000
#elif defined(CONFIG_BME680_PRESS_OVER_16X)
#define BME680_PRESS_OVER			0b10100
#else
#define BME680_PRESS_OVER			0b00000
#endif

#if defined(CONFIG_BME680_HUMIDITY_OVER_1X)
#define BME680_HUMIDITY_OVER			0b001
#elif defined(CONFIG_BME680_HUMIDITY_OVER_2X)
#define BME680_HUMIDITY_OVER			0b010
#elif defined(CONFIG_BME680_HUMIDITY_OVER_4X)
#define BME680_HUMIDITY_OVER			0b011
#elif defined(CONFIG_BME680_HUMIDITY_OVER_8X)
#define BME680_HUMIDITY_OVER			0b100
#elif defined(CONFIG_BME680_HUMIDITY_OVER_16X)
#define BME680_HUMIDITY_OVER			0b101
#else
#define BME680_HUMIDITY_OVER			0b000
#endif

#if defined(CONFIG_BME680_FILTER_OFF)
#define BME680_FILTER				0b00000
#elif defined(CONFIG_BME680_FILTER_1)
#define BME680_FILTER				0b00100
#elif defined(CONFIG_BME680_FILTER_3)
#define BME680_FILTER				0b01000
#elif defined(CONFIG_BME680_FILTER_7)
#define BME680_FILTER				0b01100
#elif defined(CONFIG_BME680_FILTER_15)
#define BME680_FILTER				0b10000
#elif defined(CONFIG_BME680_FILTER_31)
#define BME680_FILTER				0b10100
#elif defined(CONFIG_BME680_FILTER_63)
#define BME680_FILTER				0b11000
#elif defined(CONFIG_BME680_FILTER_127)
#define BME680_FILTER				0b11100
#endif

/* I2C address */
#define BME680_I2C_ADDR				CONFIG_BME680_I2C_ADDR

/* Soft reset register */
#define BME680_SOFT_RESET_ADDR			0xe0

/* Soft reset command */
#define BME680_SOFT_RESET_CMD			0xb6
#define BME680_RESET_PERIOD			10

/* Array Index to Field data mapping for Calibration Data*/
#define BME680_T2_LSB_REG			1
#define BME680_T2_MSB_REG			2
#define BME680_T3_REG				3
#define BME680_P1_LSB_REG			5
#define BME680_P1_MSB_REG			6
#define BME680_P2_LSB_REG			7
#define BME680_P2_MSB_REG			8
#define BME680_P3_REG				9
#define BME680_P4_LSB_REG			11
#define BME680_P4_MSB_REG			12
#define BME680_P5_LSB_REG			13
#define BME680_P5_MSB_REG			14
#define BME680_P7_REG				15
#define BME680_P6_REG				16
#define BME680_P8_LSB_REG			19
#define BME680_P8_MSB_REG			20
#define BME680_P9_LSB_REG			21
#define BME680_P9_MSB_REG			22
#define BME680_P10_REG				23
#define BME680_H2_MSB_REG			25
#define BME680_H2_LSB_REG			26
#define BME680_H1_LSB_REG			26
#define BME680_H1_MSB_REG			27
#define BME680_H3_REG				28
#define BME680_H4_REG				29
#define BME680_H5_REG				30
#define BME680_H6_REG				31
#define BME680_H7_REG				32
#define BME680_T1_LSB_REG			33
#define BME680_T1_MSB_REG			34
#define BME680_GH2_LSB_REG			35
#define BME680_GH2_MSB_REG			36
#define BME680_GH1_REG				37
#define BME680_GH3_REG				38

/* Other coefficient's address */
#define BME680_ADDR_RES_HEAT_VAL_ADDR		0x00
#define BME680_ADDR_RES_HEAT_RANGE_ADDR		0x02
#define BME680_ADDR_RANGE_SW_ERR_ADDR		0x04
#define BME680_ADDR_SENS_CONF_START		0x5A
#define BME680_ADDR_GAS_CONF_START		0x64

/* Mask definitions */
#define BME680_GAS_MEAS_MSK			0x30
#define BME680_NBCONV_MSK			0X0F
#define BME680_FILTER_MSK			0X1C
#define BME680_OST_MSK				0XE0
#define BME680_OSP_MSK				0X1C
#define BME680_OSH_MSK				0X07
#define BME680_HCTRL_MSK			0x08
#define BME680_RUN_GAS_MSK			0x10
#define BME680_MODE_MSK				0x03
#define BME680_RHRANGE_MSK			0x30
#define BME680_RSERROR_MSK			0xf0
#define BME680_NEW_DATA_MSK			0x80
#define BME680_GAS_INDEX_MSK			0x0f
#define BME680_GAS_RANGE_MSK			0x0f
#define BME680_GASM_VALID_MSK			0x20
#define BME680_HEAT_STAB_MSK			0x10
#define BME680_MEM_PAGE_MSK			0x10
#define	BME680_BIT_H1_DATA_MSK			0x0F

/* Field settings */
#define BME680_FIELD0_ADDR			0x1d

/* BME680 field_x related defines */
#define BME680_FIELD_LENGTH			15
#define BME680_FIELD_ADDR_OFFSET		17

/* BME680 pressure calculation macros */
#define BME680_MAX_OVERFLOW_VAL			0x40000000

/* Sensor configuration registers */
#define BME680_CONF_HEAT_CTRL_ADDR		0x70
#define BME680_CONF_ODR_RUN_GAS_NBC_ADDR	0x71
#define BME680_CONF_OS_H_ADDR			0x72
#define BME680_CONF_T_P_MODE_ADDR		0x74
#define BME680_CONF_ODR_FILT_ADDR		0x75
#define BME680_MEM_PAGE_ADDR			0xf3

/* Gas configuration register */
#define BME680_GAS_R_LSB			0x2b

/* Gas reading valid mask */
#define BME680_GAS_VALID_MASK			0b100000

/* Heater control settings */
#define BME680_DISABLE_HEATER			0x08

/* Power mode settings */
#define BME680_SLEEP_MODE			0
#define BME680_FORCED_MODE			1
#define BME680_CONTINUOUS_MODE			3

/* BME680 General config */
#define BME680_POLL_PERIOD_MS			10
#define BME680_POLL_TIMEOUT_CHECK		10
#define BME680_TIMEOUT_ERROR_CODE		-ENXIO

/* Ambient humidity shift value for compensation */
#define BME680_HUM_REG_SHIFT_VAL		4

/* Heater settings */
#define BME680_RES_HEAT0_ADDR			0x5a
#define BME680_GAS_WAIT0_ADDR			0x64

struct BME680_data {
#ifdef CONFIG_BME680_DEV_TYPE_I2C
	struct device *i2c_master;
	u16_t i2c_slave_addr;
#else
#error "BME680 device is undefined, enable I2C interface"
#endif
	/* Compensation parameters. */
	u16_t par_h1;
	u16_t par_h2;
	s8_t par_h3;
	s8_t par_h4;
	s8_t par_h5;
	u8_t par_h6;
	s8_t par_h7;
	s8_t par_gh1;
	s16_t par_gh2;
	s8_t par_gh3;
	u16_t par_t1;
	s16_t par_t2;
	s8_t par_t3;
	u16_t par_p1;
	s16_t par_p2;
	s8_t par_p3;
	s16_t par_p4;
	s16_t par_p5;
	s8_t par_p6;
	s8_t par_p7;
	s16_t par_p8;
	s16_t par_p9;
	u8_t par_p10;

	/* Variable to store t_fine size */
	s32_t t_fine;

	/* Heater resistance range */
	u8_t res_heat_range;

	/* Heater resistance value */
	s8_t res_heat_val;

	/* Error range */
	s8_t range_sw_err;

	/* Compensated values */
	s16_t comp_temp;
	u32_t comp_press;
	u32_t comp_humidity;
	u32_t comp_gas;

	/* Power mode of sensor */
	u8_t power_mode;

	/* Chip identification */
	u8_t chip_id;

	/* Variable to store nb conversion */
	u8_t nb_conv;

	/* Variable to store heater control */
	u8_t heatr_ctrl;

	/* Run gas enable value */
	u8_t run_gas;

	/* Heater temperature value */
	u16_t heatr_temp;

	/* Duration profile value */
	u16_t heatr_dur;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_BME680_BME680_H_ */
