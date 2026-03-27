/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MS5637_MS5637_H_
#define ZEPHYR_DRIVERS_SENSOR_MS5637_MS5637_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

/* I2C commands (MS5637-02BA03 datasheet) */
#define MS5637_CMD_RESET    0x1EU
#define MS5637_CMD_READ_ADC 0x00U

/* Pressure conversion commands: base 0x40 + OSR offset */
#define MS5637_CMD_CONV_P_256  0x40U
#define MS5637_CMD_CONV_P_512  0x42U
#define MS5637_CMD_CONV_P_1024 0x44U
#define MS5637_CMD_CONV_P_2048 0x46U
#define MS5637_CMD_CONV_P_4096 0x48U
#define MS5637_CMD_CONV_P_8192 0x4AU

/* Temperature conversion commands: base 0x50 + OSR offset */
#define MS5637_CMD_CONV_T_256  0x50U
#define MS5637_CMD_CONV_T_512  0x52U
#define MS5637_CMD_CONV_T_1024 0x54U
#define MS5637_CMD_CONV_T_2048 0x56U
#define MS5637_CMD_CONV_T_4096 0x58U
#define MS5637_CMD_CONV_T_8192 0x5AU

/* PROM read commands: base 0xA0 + word_index * 2 */
#define MS5637_PROM_ADDR_BASE  0xA0U
#define MS5637_PROM_WORD_COUNT 8U

/* PROM coefficient indices */
#define MS5637_PROM_CRC_IDX 0U
#define MS5637_PROM_C1_IDX  1U /* Pressure sensitivity | SENS_T1 */
#define MS5637_PROM_C2_IDX  2U /* Pressure offset | OFF_T1 */
#define MS5637_PROM_C3_IDX  3U /* Temperature coefficient of pressure sensitivity | TCS */
#define MS5637_PROM_C4_IDX  4U /* Temperature coefficient of pressure offset | TCO */
#define MS5637_PROM_C5_IDX  5U /* Reference temperature | T_REF */
#define MS5637_PROM_C6_IDX  6U /* Temperature coefficient of temperature | TEMPSENS */

/* Reset recovery time per datasheet: 2.8 ms typical */
#define MS5637_RESET_TIME_MS 3U

/* ADC read byte count (24-bit result) */
#define MS5637_ADC_READ_LEN 3U

/* CRC-4 parameters (AN520) */
#define MS5637_CRC4_POLY        0x3000U
#define MS5637_CRC_NIBBLE_SHIFT 12U
#define MS5637_CRC_NIBBLE_MASK  0x000FU

/* Micro-degrees per centidegree for sensor_value_from_micro() */
#define MS5637_MICRO_PER_CENTIDEG 10000

/*
 * ADC conversion delays in ms per OSR level (datasheet Table 2).
 * Values are max conversion times rounded up.
 */
#define MS5637_CONV_DELAY_256  1U
#define MS5637_CONV_DELAY_512  2U
#define MS5637_CONV_DELAY_1024 3U
#define MS5637_CONV_DELAY_2048 5U
#define MS5637_CONV_DELAY_4096 9U
#define MS5637_CONV_DELAY_8192 17U

/* Default oversampling from Kconfig */
#if defined(CONFIG_MS5637_PRES_OVER_256X)
#define MS5637_PRES_OVER_DEFAULT 256
#elif defined(CONFIG_MS5637_PRES_OVER_512X)
#define MS5637_PRES_OVER_DEFAULT 512
#elif defined(CONFIG_MS5637_PRES_OVER_1024X)
#define MS5637_PRES_OVER_DEFAULT 1024
#elif defined(CONFIG_MS5637_PRES_OVER_4096X)
#define MS5637_PRES_OVER_DEFAULT 4096
#elif defined(CONFIG_MS5637_PRES_OVER_8192X)
#define MS5637_PRES_OVER_DEFAULT 8192
#else
#define MS5637_PRES_OVER_DEFAULT 2048
#endif

#if defined(CONFIG_MS5637_TEMP_OVER_256X)
#define MS5637_TEMP_OVER_DEFAULT 256
#elif defined(CONFIG_MS5637_TEMP_OVER_512X)
#define MS5637_TEMP_OVER_DEFAULT 512
#elif defined(CONFIG_MS5637_TEMP_OVER_1024X)
#define MS5637_TEMP_OVER_DEFAULT 1024
#elif defined(CONFIG_MS5637_TEMP_OVER_4096X)
#define MS5637_TEMP_OVER_DEFAULT 4096
#elif defined(CONFIG_MS5637_TEMP_OVER_8192X)
#define MS5637_TEMP_OVER_DEFAULT 8192
#else
#define MS5637_TEMP_OVER_DEFAULT 2048
#endif

struct ms5637_config {
	struct i2c_dt_spec bus;
};

struct ms5637_data {
	/* PROM calibration coefficients */
	uint16_t sens_t1;
	uint16_t off_t1;
	uint16_t tcs;
	uint16_t tco;
	uint16_t t_ref;
	uint16_t tempsens;

	/* Compensated measurement results */
	int32_t pressure;    /* mbar * 100 (i.e. Pascals) */
	int32_t temperature; /* centidegrees Celsius */

	/* Active conversion commands */
	uint8_t pressure_conv_cmd;
	uint8_t temperature_conv_cmd;

	/* Conversion delays in ms */
	uint8_t pressure_conv_delay;
	uint8_t temperature_conv_delay;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_MS5637_MS5637_H_ */
