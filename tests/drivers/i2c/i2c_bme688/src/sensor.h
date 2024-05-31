/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENSOR_H
#define SENSOR_H

#include <zephyr/kernel.h>

#define SLEEP_TIME_MS 250
#define MEASUREMENT_CYCLES 3
#define MAX_BURST_READ_SIZE 10

/* General */
#define CHIP_ID_REGISTER_ADDRESS 0xD0
#define VARIANT_ID_REGISTER_ADDRESS 0xF0
#define CONF_REGISTER_ADDRESS 0x75
#define CTRL_MEAS_REGISTER_ADDRESS 0x74
#define CTRL_HUM_REGISTER_ADDRESS 0x72
#define RESET_REGISTER_ADDRESS 0xE0
#define SLEEP_MODE 0x0
#define FORCED_MODE 0x1
#define CTRL_MEAS_MODE_BIT_MSK 1 << 1 | 1
#define CTRL_MEAS_MODE_BIT_SHIFT 0
#define RESET_DEVICE 0xB6
#define SENSOR_MEMORY_SIZE_IN_BYTES 255

/* Calibration coeffcients */
#define TEMP_PAR_T1_REGISTER_ADDRESS_LSB 0xE9
#define TEMP_PAR_T1_REGISTER_ADDRESS_MSB 0xEA
#define TEMP_PAR_T2_REGISTER_ADDRESS_LSB 0x8A
#define TEMP_PAR_T2_REGISTER_ADDRESS_MSB 0x8B
#define TEMP_PAR_T3_REGISTER_ADDRESS 0x8C

#define HUMI_PAR_REGISTERS_START_ADDRESS 0xE1
#define HUMI_PAR_REGISTERS_COUNT 8
#define HUMI_PAR_H1_LSB_BUF_POSITION 1
#define HUMI_PAR_H1_MSB_BUF_POSITION 2
#define HUMI_PAR_H2_LSB_BUF_POSITION 1
#define HUMI_PAR_H2_MSB_BUF_POSITION 0
#define HUMI_PAR_H3_BUF_POSITION 3
#define HUMI_PAR_H4_BUF_POSITION 4
#define HUMI_PAR_H5_BUF_POSITION 5
#define HUMI_PAR_H6_BUF_POSITION 6
#define HUMI_PAR_H7_BUF_POSITION 7

#define HUMI_PAR_H1_REGISTER_ADDRESS_LSB 0xE2
#define HUMI_PAR_H1_LSB_BIT_MASK 0xFF
#define HUMI_PAR_H1_REGISTER_ADDRESS_MSB 0xE3
#define HUMI_PAR_H2_REGISTER_ADDRESS_LSB 0xE2
#define HUMI_PAR_H2_REGISTER_ADDRESS_MSB 0xE1
#define HUMI_PAR_H3_REGISTER_ADDRESS 0xE4
#define HUMI_PAR_H4_REGISTER_ADDRESS 0xE5
#define HUMI_PAR_H5_REGISTER_ADDRESS 0xE6
#define HUMI_PAR_H6_REGISTER_ADDRESS 0xE7
#define HUMI_PAR_H7_REGISTER_ADDRESS 0xE8

#define PRES_PAR_P1_REGISTER_ADDRESS_LSB 0x8E
#define PRES_PAR_P1_REGISTER_ADDRESS_MSB 0x8F
#define PRES_PAR_P2_REGISTER_ADDRESS_LSB 0x90
#define PRES_PAR_P2_REGISTER_ADDRESS_MSB 0x91
#define PRES_PAR_P3_REGISTER_ADDRESS 0x92
#define PRES_PAR_P4_REGISTER_ADDRESS_LSB 0x94
#define PRES_PAR_P4_REGISTER_ADDRESS_MSB 0x95
#define PRES_PAR_P5_REGISTER_ADDRESS_LSB 0x96
#define PRES_PAR_P5_REGISTER_ADDRESS_MSB 0x97
#define PRES_PAR_P6_REGISTER_ADDRESS 0x99
#define PRES_PAR_P7_REGISTER_ADDRESS 0x98
#define PRES_PAR_P8_REGISTER_ADDRESS_LSB 0x9C
#define PRES_PAR_P8_REGISTER_ADDRESS_MSB 0x9D
#define PRES_PAR_P9_REGISTER_ADDRESS_LSB 0x9E
#define PRES_PAR_P9_REGISTER_ADDRESS_MSB 0x9F
#define PRES_PAR_P10_REGISTER_ADDRESS 0xA0

/* IIR filter */
#define IIR_FILER_ORDER_BIT_MASK 1 << 4 | 1 << 3 | 1 << 2
#define IIR_FILER_ORDER_BIT_SHIFT 2
#define IIR_FILER_COEFF_3 0x2

/* Temperature measurement */
#define TEMPERATURE_OVERSAMPLING_2X 0x2
#define TEMP_OVERSAMPLING_BIT_MSK 1 << 7 | 1 << 6 | 1 << 5
#define TEMP_OVERSAMPLING_BIT_SHIFT 5

#define TEMP_ADC_DATA_MSB_0 0x22
#define TEMP_ADC_DATA_LSB_0 0x23
#define TEMP_ADC_DATA_XLSB_0 0x24

/* Pressure measurement */
#define PRESSURE_OVERSAMPLING_16X 0x5
#define PRES_OVERSAMPLING_BIT_MSK 1 << 4 | 1 << 3 | 1 << 2
#define PRES_OVERSAMPLING_BIT_SHIFT 2

#define PRES_ADC_DATA_MSB_0 0x1F
#define PRES_ADC_DATA_LSB_0 0x20
#define PRES_ADC_DATA_XLSB_0 0x21

/* Humidity measurement */
#define HUMIDITY_OVERSAMPLING_1X 0x1
#define HUMIDITY_OVERSAMPLING_BIT_MSK 1 << 2 | 1 << 1 | 1
#define HUMIDITY_OVERSAMPLING_BIT_SHIFT 0

#define HUM_ADC_DATA_MSB_0 0x25
#define HUM_ADC_DATA_LSB_0 0x26

/* Measurement status */
#define MEAS_STATUS_0_REG_ADDRESS 0x1D
#define MEAS_STATUS_1_REG_ADDRESS 0x2E
#define MEAS_STATUS_2_REG_ADDRESS 0x3F
#define MEASUREMENT_IN_PROGRESS_BIT_MASK 1 << 5
#define MEASUREMENT_NEW_DATA_BIT_MASK 1 << 7

struct calibration_coeffs {
	/* Temperature */
	uint16_t par_t1;
	uint16_t par_t2;
	uint8_t par_t3;

	/* Humidity */
	uint16_t par_h1;
	uint16_t par_h2;
	uint8_t par_h3;
	uint8_t par_h4;
	uint8_t par_h5;
	uint8_t par_h6;
	uint8_t par_h7;

	/* Pressure */
	uint16_t par_p1;
	int16_t par_p2;
	int8_t par_p3;
	int16_t par_p4;
	int16_t par_p5;
	int8_t par_p6;
	int8_t par_p7;
	int16_t par_p8;
	int16_t par_p9;
	uint8_t par_p10;
};

/* Calculate the compensated temperature
 * Note: 't_fine' is required for other measurements
 */
int32_t calculate_temperature(uint32_t adc_temp, int32_t *t_fine,
			      struct calibration_coeffs *cal_coeffs);

/* Calculate the compensated pressure
 * Note: temperature must be calculated first
 * to obtain the 't_fine'
 */
uint32_t calculate_pressure(uint32_t pres_adc, int32_t t_fine,
			    struct calibration_coeffs *cal_coeffs);

/* Calculate the relative humidity
 * Note: temperature must be calculated first
 * to obtain the 't_fine'
 */
uint32_t calculate_humidity(uint16_t hum_adc, int32_t t_fine,
			    struct calibration_coeffs *cal_coeffs);

#endif
