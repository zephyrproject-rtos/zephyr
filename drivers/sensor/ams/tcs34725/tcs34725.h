/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Darshan Maru <darshan.maru@dnkmail.in>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file tcs34725.h
 * @brief TCS34725 Color Light-to-Digital Converter driver.
 *
 * Defines register addresses, bit masks, driver configuration and runtime
 * data structures for the TCS34725 Zephyr sensor driver.
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TCS34725_TCS34725_H_
#define ZEPHYR_DRIVERS_SENSOR_TCS34725_TCS34725_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/tcs34725.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* ---- Command register control bits ---- */
#define TCS34725_CMD_BIT          0x80 /* Must be 1 to select COMMAND reg */
#define TCS34725_CMD_TYPE_REPEAT  0x00 /* Repeated byte protocol */
#define TCS34725_CMD_TYPE_AUTOINC 0x20 /* Auto-increment protocol */
#define TCS34725_CMD_TYPE_SPECIAL 0x60 /* Special function */
#define TCS34725_CMD_CLR_INT      0x66 /* CMD|SPECIAL|0x06: clear interrupt */

/* ---- Register addresses ---- */
#define TCS34725_REG_ENABLE  0x00 /* Enables states and interrupts */
#define TCS34725_REG_ATIME   0x01 /* RGBC integration time (ATIME) */
#define TCS34725_REG_WTIME   0x03 /* Wait time */
#define TCS34725_REG_AILTL   0x04 /* Clear interrupt low threshold, low byte */
#define TCS34725_REG_AILTH   0x05 /* Clear interrupt low threshold, high byte */
#define TCS34725_REG_AIHTL   0x06 /* Clear interrupt high threshold, low byte */
#define TCS34725_REG_AIHTH   0x07 /* Clear interrupt high threshold, high byte */
#define TCS34725_REG_PERS    0x0C /* Interrupt persistence filter (APERS) */
#define TCS34725_REG_CONFIG  0x0D /* Configuration register (WLONG) */
#define TCS34725_REG_CONTROL 0x0F /* Control register (AGAIN gain select) */
#define TCS34725_REG_ID      0x12 /* Device ID register (read-only) */
#define TCS34725_REG_STATUS  0x13 /* Device status register (read-only) */
#define TCS34725_REG_CDATAL  0x14 /* Clear data (LSB) */
#define TCS34725_REG_CDATAH  0x15 /* Clear data (MSB) */
#define TCS34725_REG_RDATAL  0x16 /* Red data (LSB) */
#define TCS34725_REG_RDATAH  0x17 /* Red data (MSB) */
#define TCS34725_REG_GDATAL  0x18 /* Green data (LSB) */
#define TCS34725_REG_GDATAH  0x19 /* Green data (MSB) */
#define TCS34725_REG_BDATAL  0x1A /* Blue data (LSB) */
#define TCS34725_REG_BDATAH  0x1B /* Blue data (MSB) */

/* ---- ENABLE register bits (0x00) ---- */
#define TCS34725_ENABLE_AIEN BIT(4) /* RGBC interrupt enable */
#define TCS34725_ENABLE_WEN  BIT(3) /* Wait enable */
#define TCS34725_ENABLE_AEN  BIT(1) /* RGBC enable */
#define TCS34725_ENABLE_PON  BIT(0) /* Power ON */

/* ---- CONFIG register bits (0x0D) ---- */
#define TCS34725_CONFIG_WLONG BIT(1) /* Wait Long: extends WTIME wait cycles by 12x */

/* ---- CONTROL register - AGAIN field (0x0F, bits 1:0) ---- */
#define TCS34725_AGAIN_1X  0x00 /* 1x gain */
#define TCS34725_AGAIN_4X  0x01 /* 4x gain */
#define TCS34725_AGAIN_16X 0x02 /* 16x gain */
#define TCS34725_AGAIN_60X 0x03 /* 60x gain */

/* ---- STATUS register bits (0x13) ---- */
#define TCS34725_STATUS_AINT   BIT(4) /* RGBC clear channel interrupt */
#define TCS34725_STATUS_AVALID BIT(0) /* RGBC valid: integration cycle complete */

/* ---- ID register values (0x12) ---- */
#define TCS34725_ID_34721_34725 0x44 /* Part ID for TCS34721 / TCS34725 */
#define TCS34725_ID_34723_34727 0x4D /* Part ID for TCS34723 / TCS34727 */

/* Power-up warmup delay required after PON before AEN (datasheet note) */
#define TCS34725_PON_WARMUP_MS 3

/* ------------------------------------------------------------------ */
/* DN40 Lux / CCT calculation constants (Q16.16 fixed-point)          */
/* Source: ams Application Note DN40-Rev 1.0                          */
/* Coefficients rounded to nearest representable Q16.16 value.        */
/* ------------------------------------------------------------------ */
#define TCS34725_FX_SHIFT 16
#define TCS34725_FX_ONE   (1 << TCS34725_FX_SHIFT)

#define TCS34725_DN40_DGF_FX        20316160  /* 310.0 */
#define TCS34725_DN40_R_COEF_FX     8918      /* 0.136 */
#define TCS34725_DN40_G_COEF_FX     65536     /* 1.000 */
#define TCS34725_DN40_B_COEF_FX     (-29098)  /* -0.444 */
#define TCS34725_DN40_CT_COEF_FX    249692160 /* 3810.0 */
#define TCS34725_DN40_CT_OFFSET_FX  91160576  /* 1391.0 */
#define TCS34725_DN40_ATIME_STEP_FX 157286    /* 2.4 ms per ATIME step */

/* Minimum clear-channel count required for a numerically stable result */
#define TCS34725_DN40_MIN_CLEAR 100

struct tcs34725_config {
	struct i2c_dt_spec i2c; /**< I2C bus specification from devicetree. */

	uint8_t gain;     /**< ADC gain setting (0=1x, 1=4x, 2=16x, 3=60x). */
	uint8_t atime;    /**< Integration time register value (ATIME). */
	uint8_t wtime;    /**< Wait time register value (WTIME). */
	bool wlong;       /**< Enable 12x wait time extension. */
	bool wait_enable; /**< Enable wait state between RGBC measurement cycles. */
};

struct tcs34725_data {
	uint16_t clear; /**< Raw 16-bit clear channel ADC count. */
	uint16_t red;   /**< Raw 16-bit red channel ADC count. */
	uint16_t green; /**< Raw 16-bit green channel ADC count. */
	uint16_t blue;  /**< Raw 16-bit blue channel ADC count. */
	uint16_t lux;   /**< Illuminance derived from DN40 algorithm, in lux. */
	/**
	 * Correlated color temperature derived from DN40 algorithm, in Kelvin.
	 */
	uint16_t color_temp;
	/**
	 * True if the last lux and color temperature calculation was
	 * numerically valid (not saturated, not too dim).
	 */
	bool lux_valid;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_TCS34725_TCS34725_H_ */
