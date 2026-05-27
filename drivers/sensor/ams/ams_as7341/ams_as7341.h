/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Dotcom IoT LLP
 * Author: Mahendra Sondagar <mahendra@dotcom.co.in>
 * Author: Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 */
/**
 * @file ams_as7341.h
 * @brief AS7341 Spectral Sensor Module driver.
 *
 * Defines register addresses, bit masks, driver configuration and runtime
 * data structures for the AS7341 Zephyr sensor driver.
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_AS7341_AS7341_H_
#define ZEPHYR_DRIVERS_SENSOR_AS7341_AS7341_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>

#define AS7341_I2C_ADDR 0x39 /* I2C address */

#define AS7341_REG_ID 0x92 /* Device ID register */

/* Bank 0 Registers */
#define AS7341_REG_ASTATUS_0 0x60 /* Auxiliary status (bank 0: saturation, flags) */

#define AS7341_REG_CH0_DATA_L_0 0x61 /* Channel 0 data (LSB, bank 0) */
#define AS7341_REG_CH0_DATA_H_0 0x62 /* Channel 0 data (MSB, bank 0) */

#define AS7341_REG_ITIME_L 0x63 /* Integration time (LSB) */
#define AS7341_REG_ITIME_M 0x64 /* Integration time (mid byte) */
#define AS7341_REG_ITIME_H 0x65 /* Integration time (MSB) */

#define AS7341_REG_CH1_DATA_L_0 0x66 /* Channel 1 data (LSB, bank 0) */
#define AS7341_REG_CH1_DATA_H_0 0x67 /* Channel 1 data (MSB, bank 0) */

#define AS7341_REG_CH2_DATA_L_0 0x68 /* Channel 2 data (LSB, bank 0) */
#define AS7341_REG_CH2_DATA_H_0 0x69 /* Channel 2 data (MSB, bank 0) */

#define AS7341_REG_CH3_DATA_L_0 0x6A /* Channel 3 data (LSB, bank 0) */
#define AS7341_REG_CH3_DATA_H_0 0x6B /* Channel 3 data (MSB, bank 0) */

#define AS7341_REG_CH4_DATA_L_0 0x6C /* Channel 4 data (LSB, bank 0) */
#define AS7341_REG_CH4_DATA_H_0 0x6D /* Channel 4 data (MSB, bank 0) */

#define AS7341_REG_CH5_DATA_L_0 0x6E /* Channel 5 data (LSB, bank 0) */
#define AS7341_REG_CH5_DATA_H_0 0x6F /* Channel 5 data (MSB, bank 0) */

#define AS7341_REG_CONFIG  0x70 /* LED control and measurement mode configuration */
#define AS7341_REG_STATUS0 0x71 /* Status register */
#define AS7341_REG_EDGE	   0x72 /* Interrupt edge configuration */

#define AS7341_REG_GPIO 0x73 /* GPIO control register */
#define AS7341_REG_LED	0x74 /* LED control register */

#define AS7341_REG_ENABLE    0x80   /* Enable register */
#define AS7341_ENABLE_PON    BIT(0) /* Power ON */
#define AS7341_ENABLE_SP_EN  BIT(1) /* Spectral measurement enable */
#define AS7341_ENABLE_SMUXEN BIT(4) /* SMUX enable */
#define AS7341_ENABLE_FDEN   BIT(6) /* Flicker detection enable */

#define AS7341_REG_ATIME 0x81 /* Integration time */
#define AS7341_REG_WTIME 0x83 /* Wait time */

#define AS7341_REG_SP_TH_L_LSB 0x84 /* Spectral threshold low (LSB) */
#define AS7341_REG_SP_TH_L_MSB 0x85 /* Spectral threshold low (MSB) */

#define AS7341_REG_SP_TH_H_LSB 0x86 /* Spectral threshold high (LSB) */
#define AS7341_REG_SP_TH_H_MSB 0x87 /* Spectral threshold high (MSB) */

#define AS7341_REG_AUXID 0x90 /* Auxiliary ID register */
#define AS7341_REG_REVID 0x91 /* Revision ID register */

#define AS7341_REG_STATUS1 0x93 /* Interrupt Status register */

/* Bank 1 Registers */
#define AS7341_REG_ASTATUS_1 0x94 /* Auxiliary status (bank 1: gain status) */

#define AS7341_REG_CH0_DATA_L_1 0x95 /* Channel 0 data (LSB, bank 1) */
#define AS7341_REG_CH0_DATA_H_1 0x96 /* Channel 0 data (MSB, bank 1) */

#define AS7341_REG_CH1_DATA_L_1 0x97 /* Channel 1 data (LSB, bank 1) */
#define AS7341_REG_CH1_DATA_H_1 0x98 /* Channel 1 data (MSB, bank 1) */

#define AS7341_REG_CH2_DATA_L_1 0x99 /* Channel 2 data (LSB, bank 1) */
#define AS7341_REG_CH2_DATA_H_1 0x9A /* Channel 2 data (MSB, bank 1) */

#define AS7341_REG_CH3_DATA_L_1 0x9B /* Channel 3 data (LSB, bank 1) */
#define AS7341_REG_CH3_DATA_H_1 0x9C /* Channel 3 data (MSB, bank 1) */

#define AS7341_REG_CH4_DATA_L_1 0x9D /* Channel 4 data (LSB, bank 1) */
#define AS7341_REG_CH4_DATA_H_1 0x9E /* Channel 4 data (MSB, bank 1) */

#define AS7341_REG_CH5_DATA_L_1 0x9F /* Channel 5 data (LSB, bank 1) */
#define AS7341_REG_CH5_DATA_H_1 0xA0 /* Channel 5 data (MSB, bank 1) */

#define AS7341_REG_STATUS2 0xA3 /* Status register 2 */
#define AS7341_REG_STATUS3 0xA4 /* Status register 3 */
#define AS7341_REG_STATUS5 0xA6 /* Status register 5 */
#define AS7341_REG_STATUS6 0xA7 /* Status register 6 */

#define AS7341_REG_CFG0	      0xA9   /* Configuration register 0 */
#define AS7341_CFG0_REG_BANK  BIT(4) /* Register bank select */
#define AS7341_CFG0_LOW_POWER BIT(0) /* Low power mode */
#define AS7341_CFG0_WLONG     BIT(2) /* Wait long enable */

#define AS7341_REG_CFG1 0xAA /* Configuration register 1 */
#define AS7341_REG_CFG3 0xAC /* Configuration register 3 */
#define AS7341_REG_CFG6 0xAF /* Configuration register 6 */
#define AS7341_REG_CFG8 0xB1 /* Configuration register 8 */

#define AS7341_REG_CFG9	 0xB2 /* Configuration register 9 */
#define AS7341_REG_CFG10 0xB3 /* Configuration register 10 */
#define AS7341_REG_CFG12 0xB5 /* Configuration register 12 */

#define AS7341_REG_PERS 0xBD /* Interrupt persistence register */

#define AS7341_REG_GPIO2 0xBE /* GPIO control register 2 */

#define AS7341_REG_ASTEP_L 0xCA /* Integration step (LSB) */
#define AS7341_REG_ASTEP_H 0xCB /* Integration step (MSB) */

#define AS7341_REG_AGC_GAIN_MAX 0xCF /* AGC gain maximum register */

#define AS7341_REG_AZ_CONFIG 0xD6 /* Auto-zero configuration */

#define AS7341_REG_INTENAB  0xF9 /* Interrupt enable register */
#define AS7341_INTENAB_MASK BIT(3)
#define AS7341_INTENAB_CONF BIT(3)

#define AS7341_REG_CONTROL 0xFA /* Control register */

#define AS7341_REG_FIFO_MAP 0xFC /* FIFO channel mapping */
#define AS7341_REG_FIFO_LVL 0xFD /* FIFO level */

#define AS7341_REG_FDATA_L 0xFE /* FIFO data (LSB) */
#define AS7341_REG_FDATA_H 0xFF /* FIFO data (MSB) */

#define AS7341_SPECTRAL_INT_HIGH_MSK BIT(5)
#define AS7341_SPECTRAL_INT_LOW_MSK  BIT(4)

/* SMUX commands */
#define AS7341_SMUX_CMD_ROM_INIT 0 /* ROM code initialization */
#define AS7341_SMUX_CMD_READ	 1 /* Read SMUX config to RAM */
#define AS7341_SMUX_CMD_WRITE	 2 /* Write SMUX config from RAM to SMUX chain */

/* Flicker Detection Registers */
#define AS7341_REG_FD_CFG0   0xD7 /* Flicker detection configuration */
#define AS7341_REG_FD_TIME1  0xD8 /* Flicker detection time LSB */
#define AS7341_REG_FD_TIME2  0xDA /* Flicker detection time MSB + gain */
#define AS7341_REG_FD_STATUS 0xDB /* Flicker detection status */

#define AS7341_FD_STATUS_MEAS_VALID    BIT(5)
#define AS7341_FD_STATUS_SATURATION    BIT(4)
#define AS7341_FD_STATUS_120HZ_VALID   BIT(3)
#define AS7341_FD_STATUS_100HZ_VALID   BIT(2)
#define AS7341_FD_STATUS_120HZ_FLICKER BIT(1)
#define AS7341_FD_STATUS_100HZ_FLICKER BIT(0)

/* In your register defines section */
#define AS7341_CONFIG_FLICKER_MODE BIT(3)

/* SMUX command bits in CFG6 */
#define AS7341_CFG6_SMUX_CMD_MASK 0x18 /* bits [4:3] */
#define AS7341_SMUX_CMD_SHIFT	  3
#define AS7341_SMUX_CMD_MASK	  0x03 /* 2-bit command code */

/* SMUX load command */
#define AS7341_SMUX_LOAD_CMD 0x10

/* CFG0 register configuration (bank select and mode) */
#define AS7341_CFG0_NORMAL_BANK0 0x20 /* Bank 0, normal operation */

/* LED brightness */
#define AS7341_LED_BRIGHTNESS_DEFAULT 0x80

/* Default integration time (ATIME) – 100 corresponds to moderate light */
#define AS7341_ATIME_DEFAULT 100

/* ASTEP value for 999 (typical integration steps) */
#define AS7341_ASTEP_999_LSB 0xE7
#define AS7341_ASTEP_999_MSB 0x03

/**
 * @brief Driver-specific sensor channel identifiers for the AS7341.
 *
 * Extends the Zephyr sensor_channel enum starting from SENSOR_CHAN_PRIV_START
 * to avoid conflicts with the standard Zephyr sensor channels defined in
 * <zephyr/drivers/sensor.h>.
 *
 * Each entry maps to one of the AS7341's eight narrowband spectral filters
 * or to the broadband clear and near-infrared (NIR) channels. Because the
 * sensor has more channels than ADC inputs, the channels are split across
 * two SMUX measurement passes:
 *
 * Pass 1 (F1–F4, CLEAR_0, NIR_0) – low spectral channels:
 *   - SENSOR_CHAN_AS7341_415NM_F1  – F1 filter, 415 nm (violet)
 *   - SENSOR_CHAN_AS7341_445NM_F2  – F2 filter, 445 nm (violet/blue)
 *   - SENSOR_CHAN_AS7341_480NM_F3  – F3 filter, 480 nm (blue)
 *   - SENSOR_CHAN_AS7341_515NM_F4  – F4 filter, 515 nm (cyan/green)
 *   - SENSOR_CHAN_AS7341_CLEAR_0   – clear channel (unfiltered, pass 1)
 *   - SENSOR_CHAN_AS7341_NIR_0     – near-infrared channel (pass 1)
 *
 * Pass 2 (F5–F8, CLEAR, NIR) – high spectral channels:
 *   - SENSOR_CHAN_AS7341_555NM_F5  – F5 filter, 555 nm (green)
 *   - SENSOR_CHAN_AS7341_590NM_F6  – F6 filter, 590 nm (yellow)
 *   - SENSOR_CHAN_AS7341_630NM_F7  – F7 filter, 630 nm (orange/red)
 *   - SENSOR_CHAN_AS7341_680NM_F8  – F8 filter, 680 nm (red)
 *   - SENSOR_CHAN_AS7341_CLEAR     – clear channel (unfiltered, pass 2)
 *   - SENSOR_CHAN_AS7341_NIR       – near-infrared channel (pass 2)
 *
 * Use these identifiers with sensor_channel_get() after a successful
 * sensor_sample_fetch() call.
 */
enum sensor_channel_as7341 {
	SENSOR_CHAN_AS7341_415NM_F1 = SENSOR_CHAN_PRIV_START,
	SENSOR_CHAN_AS7341_445NM_F2,
	SENSOR_CHAN_AS7341_480NM_F3,
	SENSOR_CHAN_AS7341_515NM_F4,
	SENSOR_CHAN_AS7341_CLEAR_0,
	SENSOR_CHAN_AS7341_NIR_0,
	SENSOR_CHAN_AS7341_555NM_F5,
	SENSOR_CHAN_AS7341_590NM_F6,
	SENSOR_CHAN_AS7341_630NM_F7,
	SENSOR_CHAN_AS7341_680NM_F8,
	SENSOR_CHAN_AS7341_CLEAR,
	SENSOR_CHAN_AS7341_NIR,
	SENSOR_CHAN_AS7341_FLICKER,
};

/**
 * @brief Internal ADC channel identifiers for the AS7341.
 *
 * The AS7341 has six internal ADC channels (0–5). Each channel is connected
 * to a photodiode input via the SMUX multiplexer. The SMUX routing table
 * determines which spectral filter is connected to which ADC channel for
 * each measurement pass.
 *
 * These identifiers are used internally by the driver and are not exposed
 * through the Zephyr sensor API.
 */
enum as7341_adc_channel {
	AS7341_ADC_CHANNEL_0,
	AS7341_ADC_CHANNEL_1,
	AS7341_ADC_CHANNEL_2,
	AS7341_ADC_CHANNEL_3,
	AS7341_ADC_CHANNEL_4,
	AS7341_ADC_CHANNEL_5,
};

/**
 * @brief AS7341 flicker detection status values.
 *
 * These values are read from the AS7341_REG_FD_STATUS register and indicate
 * the detected flicker frequency or an unknown pattern.
 */
enum as7341_fd_status {
	AS7341_FD_STATUS_UNKNOWN = 44, /* 0x2C */
	AS7341_FD_STATUS_100HZ = 45,   /* 0x2D */
	AS7341_FD_STATUS_120HZ = 46,   /* 0x2E */
};

/**
 * @brief AS7341 ADC gain settings.
 *
 * These values are written to the AS7341_REG_CFG1 register to set the
 * analog gain for spectral measurements. Higher gain increases sensitivity
 * but also amplifies noise.
 */
enum as7341_gain {
	AS7341_GAIN_0_5X = 0x00,
	AS7341_GAIN_1X = 0x01,
	AS7341_GAIN_2X = 0x02,
	AS7341_GAIN_4X = 0x03,
	AS7341_GAIN_8X = 0x04,
	AS7341_GAIN_16X = 0x05,
	AS7341_GAIN_32X = 0x06,
	AS7341_GAIN_64X = 0x07,
	AS7341_GAIN_128X = 0x08,
	AS7341_GAIN_256X = 0x09,
	AS7341_GAIN_512X = 0x0A,
};

/**
 * @brief AS7341 chip identification values.
 *
 * These values are read from the AS7341_REG_ID register to identify the
 * sensor revision. The driver expects either the main value (0x09) or the
 * alternative value (0x24).
 */
enum as7341_chip_id {
	AS7341_CHIP_ID_VAL = 0x09,
	AS7341_CHIP_ID_ALT = 0x24,
};

/**
 * @brief Detect flicker frequency from AS7341 sensor.
 *
 * Configures sensor for flicker detection, measures, and decodes FD_STATUS
 * into Hz (0 = none, 1 = unknown, 100, or 120).
 *
 * @param dev AS7341 device instance.
 * @param flicker_hz Pointer to store detected frequency in Hz.
 *
 * @return 0 on success, negative errno on failure.
 */
int as7341_detect_flicker(const struct device *dev, uint16_t *flicker_hz);

/**
 * @brief AS7341 driver compile-time configuration structure.
 *
 * Holds all configuration that is fixed at build time and derived from
 * the device tree node for this AS7341 instance. Populated by the
 * AS7341_DEFINE() macro via I2C_DT_SPEC_INST_GET().
 *
 * This structure is stored in read-only memory (rodata).
 */
struct as7341_config {
	struct i2c_dt_spec i2c;
};

/**
 * @brief AS7341 driver runtime data structure.
 *
 * Holds all mutable per-instance state for the AS7341 driver. A separate
 * instance of this structure exists for each AS7341 device node that is
 * enabled in the device tree.
 *
 * The ch_data array stores raw 16-bit ADC counts populated by
 * as7341_sample_fetch(). The layout is:
 *
 * | Index | Channel              | Pass |
 * |-------|----------------------|------|
 * |  0    | F1 – 415 nm          | 1    |
 * |  1    | F2 – 445 nm          | 1    |
 * |  2    | F3 – 480 nm          | 1    |
 * |  3    | F4 – 515 nm          | 1    |
 * |  4    | Clear (CLEAR_0)      | 1    |
 * |  5    | NIR   (NIR_0)        | 1    |
 * |  6    | F5 – 555 nm          | 2    |
 * |  7    | F6 – 590 nm          | 2    |
 * |  8    | F7 – 630 nm          | 2    |
 * |  9    | F8 – 680 nm          | 2    |
 * | 10    | Clear (CLEAR)        | 2    |
 * | 11    | NIR   (NIR)          | 2    |
 */
struct as7341_data {
	uint16_t ch_data[12]; /* F1–F4, CLEAR_0, NIR_0, F5–F8, CLEAR, NIR  */
	uint16_t flicker_hz;
	uint8_t gain;
	uint16_t atime;
	uint16_t astep;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_AS7341_AS7341_H_ */
