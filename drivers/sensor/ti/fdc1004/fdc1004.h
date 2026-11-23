/*
 * Copyright (c) 2026 Zephyr Project Developers
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TI FDC1004 — registers and channel definitions.
 */

#ifndef DRIVERS_SENSOR_FDC1004_H_
#define DRIVERS_SENSOR_FDC1004_H_

#include <zephyr/drivers/sensor.h>

/* All registers are 16-bit, big-endian */

/* Measurement result registers: MSB word (bits 23:8) */
#define FDC1004_REG_MEAS1_MSB   0x00
#define FDC1004_REG_MEAS2_MSB   0x02
#define FDC1004_REG_MEAS3_MSB   0x04
#define FDC1004_REG_MEAS4_MSB   0x06

/* Measurement result registers: LSB word (bits 7:0 in [15:8], reserved in [7:0]) */
#define FDC1004_REG_MEAS1_LSB   0x01
#define FDC1004_REG_MEAS2_LSB   0x03
#define FDC1004_REG_MEAS3_LSB   0x05
#define FDC1004_REG_MEAS4_LSB   0x07

/* Measurement configuration registers */
#define FDC1004_REG_CONF_MEAS1  0x08
#define FDC1004_REG_CONF_MEAS2  0x09
#define FDC1004_REG_CONF_MEAS3  0x0A
#define FDC1004_REG_CONF_MEAS4  0x0B

/* Main configuration / trigger register */
#define FDC1004_REG_FDC_CONF    0x0C

/* Identification registers */
#define FDC1004_REG_MANUFACTURER_ID 0xFE  /* expected: 0x5449 ("TI") */
#define FDC1004_REG_DEVICE_ID       0xFF  /* expected: 0x1004        */

/* FDC_CONF register (0x0C) bit fields */
#define FDC1004_CONF_RST_DEV       BIT(15)   /* 1 to reset all registers */
#define FDC1004_CONF_RATE_100SPS   (0x1 << 11)
#define FDC1004_CONF_RATE_200SPS   (0x2 << 11)
#define FDC1004_CONF_RATE_400SPS   (0x3 << 11)
#define FDC1004_CONF_REPEAT        BIT(8)    /* 0 = one-shot, 1 = continuous  */
#define FDC1004_CONF_MEAS1_EN      BIT(7)
#define FDC1004_CONF_MEAS2_EN      BIT(6)
#define FDC1004_CONF_MEAS3_EN      BIT(5)
#define FDC1004_CONF_MEAS4_EN      BIT(4)
#define FDC1004_CONF_DONE1         BIT(3)    /* Read-only: measurement done    */
#define FDC1004_CONF_DONE2         BIT(2)
#define FDC1004_CONF_DONE3         BIT(1)
#define FDC1004_CONF_DONE4         BIT(0)

/*
 * CONF_MEASx register (0x08..0x0B) bit fields
 *
 * [15:13] CHA — positive input: 0=CIN1, 1=CIN2, 2=CIN3, 3=CIN4, 4=CAPDAC
 * [12:10] CHB — negative input: 0=CIN1, 1=CIN2, 2=CIN3, 3=CIN4, 4=CAPDAC
 *               (CHB=4 → single-ended measurement against CAPDAC)
 * [ 9: 5] CAPDAC — offset in 3.125 pF steps (0..31)
 * [ 4: 0] reserved
 */
#define FDC1004_MEAS_CHA_SHIFT     13
#define FDC1004_MEAS_CHB_SHIFT     10
#define FDC1004_MEAS_CAPDAC_SHIFT  5
#define FDC1004_MEAS_CHB_CAPDAC    4   /* CHB value for single-ended mode */

/* Physical constants */
/* Full-scale range: ±15 pF (2^23 raw counts = 15 pF); 96.875 pF CAPDAC range */
#define FDC1004_FULL_SCALE_PF      15
/* CAPDAC step size in nano-picofarads (3.125 pF = 3125000 nPF) */
#define FDC1004_CAPDAC_STEP_NPF    3125000LL

/* Sensor channels */
enum fdc1004_channel {
	FDC1004_CHAN_CIN1 = SENSOR_CHAN_PRIV_START,
	FDC1004_CHAN_CIN2,
	FDC1004_CHAN_CIN3,
	FDC1004_CHAN_CIN4,
};

#endif /* DRIVERS_SENSOR_FDC1004_H_ */
