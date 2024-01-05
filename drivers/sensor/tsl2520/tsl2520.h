/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TSL2520_TSL2520_H_
#define ZEPHYR_DRIVERS_SENSOR_TSL2520_TSL2520_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>

#define TSL2520_MOD_CHANNEL_CTRL 0x40
#define TSL2520_MEAS_MODE0       0x81
#define TSL2520_MEAS_MODE1       0x82
#define TSL2520_SAMPLE_TIME0     0x83
#define TSL2520_ALS_NR_SAMPLES0  0x85

/* ALS_NR_SAMPLES(0xAD): Reserved: 7:3; 2:0: ALS_NR_SAMPLES[10:8]*/
#define TSL2520_ALS_NR_SAMPLES1 0x86

#define TSL2520_REG_WTIME 0x89
#define TSL2520_REG_AILT0 0x8A
#define TSL2520_REG_AILT1 0x8B
#define TSL2520_REG_AILT2 0x8C
#define TSL2520_REG_AIHT0 0x8D
#define TSL2520_REG_AIHT1 0x8E
#define TSL2520_REG_AIHT2 0x8F

#define TSL2520_REG_REVID 0x91
#define TSL2520_REG_ID    0x92

#define TSL2520_REG_STATUS         0x93
#define TSL2520_REG_ALS_STATUS     0x94
#define TSL2520_REG_ALS_DATA0_LOW  0x95
#define TSL2520_REG_ALS_DATA0_HIGH 0x96
#define TSL2520_REG_ALS_DATA1_LOW  0x97
#define TSL2520_REG_ALS_DATA1_HIGH 0x98
#define TSL2520_REG_ALS_STATUS2    0x9B
#define TSL2520_REG_STATUS2        0x9D
#define TSL2520_REG_STATUS3        0x9E
#define TSL2520_REG_STATUS4        0x9F
#define TSL2520_REG_STATUS5        0xA0

#define TSL2520_CFG0 0xA1
#define TSL2520_CFG2 0xA3
#define TSL2520_CFG3 0xA4
#define TSL2520_CFG4 0xA5
#define TSL2520_CFG5 0xA6
#define TSL2520_CFG6 0xA7
#define TSL2520_CFG7 0xA8
#define TSL2520_CFG8 0xA9

#define TSL2520_AGC_NR_SAMPLES_LOW  0xAC
/* AGC_NR_SAMPLES(0xAD): Reserved: 7:3; 2:0: AGC_NR_SAMPLES[10:8]*/
#define TSL2520_AGC_NR_SAMPLES_HIGH 0xAD

/* MOD_TRIGGER_TIMING(0xAE): Reserved: 7:3; */
#define TSL2520_MOD_TRIGGER_TIMING 0xAE

#define TSL2520_REG_CONTROL 0xB1

/* INTENAB(0xBA: 0x00): MIEN:7 | Reserved:6:4 | AIEN:3 | Reserved:2:1 | SIEN:0*/
#define TSL2520_REG_INTENAB     0xBA
#define TSL2520_REG_SIEN        0xBB
#define TSL2520_MOD_COMP_CFG1   0xCE
#define TSL2520_MEAS_SEQR_ALS_1 0xD0

#define TSL2520_MEAS_SEQR_APERS_AND_VSYNC_WAIT 0xD1
#define TSL2520_MEAS_SEQR_RESIDUAL_0           0xD2
#define TSL2520_MEAS_SEQR_RESIDUAL_1_AND_WAIT  0xD3
#define TSL2520_MEAS_SEQR_STEP0_MOD_GAINX_L    0xD4
#define TSL2520_MEAS_SEQR_STEP1_MOD_GAINX_L    0xD6
#define TSL2520_MEAS_SEQR_STEP2_MOD_GAINX_L    0xD8
#define TSL2520_MEAS_SEQR_STEP3_MOD_GAINX_L    0xDA

#define TSL2520_MEAS_SEQR_STEP0_MOD_PHDX_SMUX_L 0xDC
#define TSL2520_MEAS_SEQR_STEP0_MOD_PHDX_SMUX_H 0xDD

#define TSL2520_MEAS_SEQR_STEP1_MOD_PHDX_SMUX_L 0xDE
#define TSL2520_MEAS_SEQR_STEP1_MOD_PHDX_SMUX_H 0xDF

#define TSL2520_MEAS_SEQR_STEP2_MOD_PHDX_SMUX_L 0xE0
#define TSL2520_MEAS_SEQR_STEP2_MOD_PHDX_SMUX_H 0xE1

#define TSL2520_MEAS_SEQR_STEP3_MOD_PHDX_SMUX_L 0xE2
#define TSL2520_MEAS_SEQR_STEP3_MOD_PHDX_SMUX_H 0xE3

#define TSL2520_MOD_CALIB_CFG0 0xE4
#define TSL2520_MOD_CALIB_CFG2 0xE6

/* ENABLE(0x80: 0x00): Reserved:7:2 | AEN:1 | PON:0 */
#define TSL2520_ENABLE_ADDR    0x80
#define TSL2520_ENABLE_MASK    GENMASK(1, 0)
#define TSL2520_ENABLE_AEN_PON GENMASK(1, 0)
#define TSL2520_ENABLE_DISABLE (0)

struct tsl2520_config {
	const struct i2c_dt_spec i2c_spec;
	struct gpio_dt_spec int_gpio;
	const uint8_t sample_time;
	const uint16_t als_nr_samples;
	const uint16_t agc_nr_samples;
	const uint16_t als_gains;
};

struct tsl2520_data {
	struct gpio_callback gpio_cb;
	const struct device *dev;
	uint16_t als_data0;
	uint16_t als_data1;
};

#endif
