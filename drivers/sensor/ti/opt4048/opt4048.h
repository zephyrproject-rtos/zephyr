/*
 * Copyright (c) 2026 Darshan Maru <darshanmaru1411@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file opt4048.h
 * @brief TI OPT4048 XYZ tristimulus color sensor driver.
 *
 * Defines register addresses, bit masks, fixed-point constants, driver
 * configuration and runtime data structures for the OPT4048 Zephyr
 * sensor driver. Register semantics follow the TI OPT4048 datasheet
 * (SBOSA84).
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TI_OPT4048_OPT4048_H_
#define ZEPHYR_DRIVERS_SENSOR_TI_OPT4048_OPT4048_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/opt4048.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>

/* ---- Register addresses ---- */
#define OPT4048_REG_CH0_MSB        0x00 /* X channel: exponent + result MSB */
#define OPT4048_REG_CH0_LSB        0x01 /* X channel: result LSB, counter, CRC */
#define OPT4048_REG_CH1_MSB        0x02 /* Y channel: exponent + result MSB */
#define OPT4048_REG_CH1_LSB        0x03 /* Y channel: result LSB, counter, CRC */
#define OPT4048_REG_CH2_MSB        0x04 /* Z channel: exponent + result MSB */
#define OPT4048_REG_CH2_LSB        0x05 /* Z channel: result LSB, counter, CRC */
#define OPT4048_REG_CH3_MSB        0x06 /* W channel: exponent + result MSB */
#define OPT4048_REG_CH3_LSB        0x07 /* W channel: result LSB, counter, CRC */
#define OPT4048_REG_THRESHOLD_LOW  0x08 /* Low threshold */
#define OPT4048_REG_THRESHOLD_HIGH 0x09 /* High threshold */
#define OPT4048_REG_CONFIG         0x0A /* Range, conversion time, mode, INT behavior */
#define OPT4048_REG_THRESHOLD_CFG  0x0B /* Threshold channel and INT pin configuration */
#define OPT4048_REG_STATUS         0x0C /* Status flags */
#define OPT4048_REG_DEVICE_ID      0x11 /* Device ID (read-only) */

/* ---- DEVICE_ID register (0x11): DIDL[13:12] : DIDH[11:0] ---- */
#define OPT4048_DEVICE_ID      0x0821 /* Expected device ID value */
#define OPT4048_DEVICE_ID_MASK 0x3FFF /* Valid device ID bits */

/* ---- CONFIG register bits (0x0A) ---- */
#define OPT4048_CFG_QWAKE_MASK    BIT(15)         /* Quick wake-up */
#define OPT4048_CFG_RANGE_MASK    GENMASK(13, 10) /* Full-scale range */
#define OPT4048_CFG_CONVTIME_MASK GENMASK(9, 6)   /* Conversion time */
#define OPT4048_CFG_MODE_MASK     GENMASK(5, 4)   /* Operating mode */
#define OPT4048_CFG_LATCH_MASK    BIT(3)          /* Interrupt latch */
#define OPT4048_CFG_INTPOL_MASK   BIT(2)          /* INT polarity */
#define OPT4048_CFG_FAULT_MASK    GENMASK(1, 0)   /* Fault count */

/* ---- THRESHOLD_CFG register bits (0x0B) ---- */
#define OPT4048_CFG2_DEFAULT     0x8011        /* Datasheet reset value */
#define OPT4048_CFG2_THCH_MASK   GENMASK(6, 5) /* Threshold channel select */
#define OPT4048_CFG2_INTDIR_MASK BIT(4)        /* INT pin direction */
#define OPT4048_CFG2_INTCFG_MASK GENMASK(3, 2) /* INT pin configuration */

/* ---- Result and threshold register fields ---- */
#define OPT4048_RESULT_EXP_SHIFT 12              /* Exponent in bits [15:12] of MSB register */
#define OPT4048_THRESH_EXP_MASK  GENMASK(15, 12) /* Threshold exponent */
#define OPT4048_THRESH_RES_MASK  GENMASK(11, 0)  /* Threshold result */

/* ---- STATUS register (0x0C) ---- */
#define OPT4048_STATUS_FLAGS_MASK 0x0F /* Flag bits [3:0]: L, H, conversion ready, overload */

/* ---- Upper result register ---- */
#define OPT4048_RESULT_MSB_MANT_MASK 0x0FFF /* Mantissa MSBs [11:0]; bits [15:12] = exponent */

/* ---- Channel and burst-read sizes ---- */
/** Number of measurement channels (X, Y, Z, W). */
#define OPT4048_NUM_CHANNELS 4

/** Number of bytes of the burst read covering all four channels. */
#define OPT4048_RAW_LEN (4 * OPT4048_NUM_CHANNELS)

/** Highest exponent reported by the device (ADC codes are below 2^28). */
#define OPT4048_MAX_EXPONENT 8

/* ---- Q31 output shifts ---- */
/**
 * A decoded value v is encoded as v / 2^shift in struct sensor_q31_data.
 */
#define OPT4048_SHIFT_RAW 28 /* ADC codes are below 2^28 */
#define OPT4048_SHIFT_LUX 20 /* Up to about 577 klux */
#define OPT4048_SHIFT_XYZ 20 /* Scaled XYZ, up to about 1 M */
#define OPT4048_SHIFT_CIE 1  /* Chromaticity 0 to 1 */
#define OPT4048_SHIFT_CCT 17 /* Kelvin, below 2^17 */

/* ---- Timing ---- */
/** Extra delay after a one-shot trigger: standby wake-up, auto-range reset, I2C. */
#define OPT4048_ONESHOT_MARGIN_US 4000U

/* ------------------------------------------------------------------ */
/* Fixed-point calculation constants                                  */
/* Source: TI OPT4048 datasheet (SBOSA84)                             */
/* The driver does not use floating point, so it has no FPU           */
/* requirement.                                                       */
/* ------------------------------------------------------------------ */

/** Micro units: intermediate results of the value calculation. */
#define OPT4048_MICRO 1000000LL

/** Illuminance per CH1 ADC code in micro-lux (datasheet eq. 9: 2.15 mlux/code). */
#define OPT4048_LUX_PER_CODE_UL 2150LL

/** Conversion matrix coefficients are stored multiplied by 1e12; divide by this for micro. */
#define OPT4048_MATRIX_TO_MICRO 1000000LL

/** Q16.16 fixed-point used by the color temperature calculation. */
#define OPT4048_FX_SHIFT 16
#define OPT4048_FX_ONE   (1LL << OPT4048_FX_SHIFT)

/* ---- Color temperature ---- */
#define OPT4048_CCT_XE_UL    332000LL                   /* 0.3320 in micro units */
#define OPT4048_CCT_YE_UL    185800LL                   /* 0.1858 in micro units */
#define OPT4048_CCT_N_MAX_FX (10LL << OPT4048_FX_SHIFT) /* |n| limit, keeps n^3 in 64 bits */
#define OPT4048_CCT_A3       437LL                      /* n^3 coefficient */
#define OPT4048_CCT_A2       3601LL                     /* n^2 coefficient */
#define OPT4048_CCT_A1       6861LL                     /* n coefficient */
#define OPT4048_CCT_A0       5517LL                     /* Constant term */
#define OPT4048_CCT_MAX      (1LL << 17)                /* Kelvin, upper bound of valid result */

/**
 * @brief Driver configuration, constant per instance (from devicetree).
 */
struct opt4048_config {
	struct i2c_dt_spec i2c; /**< I2C bus specification from devicetree. */

	uint8_t range;     /**< Full-scale range (see enum opt4048_range). */
	uint8_t conv_time; /**< Conversion time (see enum opt4048_conversion_time). */
	uint8_t mode;      /**< Operating mode (see enum opt4048_mode). */
};

/**
 * @brief Encoded sample written into the buffer of a sensor_read() request.
 */
struct opt4048_encoded_data {
	uint64_t timestamp;                 /**< Sample time in nanoseconds. */
	uint32_t adc[OPT4048_NUM_CHANNELS]; /**< Linearised ADC codes of CH0..CH3. */
};

/**
 * @brief Driver runtime data, one per instance.
 */
struct opt4048_data {
	struct k_mutex lock; /**< Protects the registers and cached values. */

	uint16_t cfg;  /**< Cached CONFIG register (0x0A). */
	uint16_t cfg2; /**< Cached THRESHOLD_CFG register (0x0B). */

	uint32_t adc[OPT4048_NUM_CHANNELS]; /**< Last fetched linearised ADC codes. */
};

#endif /* ZEPHYR_DRIVERS_SENSOR_TI_OPT4048_OPT4048_H_ */
