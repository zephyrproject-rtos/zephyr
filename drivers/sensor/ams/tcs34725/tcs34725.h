/*
 * Copyright (c) 2026 Dotcom IoT LLP
 * SPDX-License-Identifier: Apache-2.0
 * Author: Mahendra Sondagar <mahendra@dotcom.co.in>
 * Author: Darshan Maru <darshan.maru@dnkmail.in>
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
#include <zephyr/drivers/gpio.h>
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
/* DN40 Lux / CCT calculation constants                                */
/* Source: ams Application Note DN40-Rev 1.0                          */
/* "Lux and CCT Calculations using ams Color Sensors" (2013-08-26)    */
/* Appendix I coefficients, TCS3472 row: DGF=310, R=0.136, G=1.000,   */
/* B=-0.444, CT_Coef=3810, CT_Offset=1391                             */
/* ------------------------------------------------------------------ */
#define TCS34725_DN40_DGF       310.0f
#define TCS34725_DN40_R_COEF    0.136f
#define TCS34725_DN40_G_COEF    1.000f
#define TCS34725_DN40_B_COEF    -0.444f
#define TCS34725_DN40_CT_COEF   3810.0f
#define TCS34725_DN40_CT_OFFSET 1391.0f

/* Minimum clear-channel count required for a numerically stable result */
#define TCS34725_DN40_MIN_CLEAR 100

/**
 * @brief TCS34725 driver compile-time configuration structure.
 *
 * Holds all configuration that is fixed at build time and derived from
 * the device tree node for this TCS34725 instance. Populated via
 * I2C_DT_SPEC_INST_GET() and the devicetree properties for gain,
 * integration time, and wait state behavior.
 *
 * This structure is stored in read-only memory (rodata).
 */
struct tcs34725_config {
	struct i2c_dt_spec i2c;

	uint8_t gain;     /* AGAIN: 0=1x 1=4x 2=16x 3=60x */
	uint8_t atime;    /* ATIME register value */
	uint8_t wtime;    /* WTIME register value */
	bool wlong;       /* 12x wait time extension */
	bool wait_enable; /* enable Wait state between RGBC cycles */
};

/**
 * @brief TCS34725 driver runtime data structure.
 *
 * Holds all mutable per-instance state for the TCS34725 driver. A separate
 * instance of this structure exists for each TCS34725 device node that is
 * enabled in the device tree.
 *
 * The clear/red/green/blue fields store the raw 16-bit RGBC ADC counts
 * populated by tcs34725_sample_fetch(), while lux and color_temp hold the
 * DN40-derived values. lux_valid indicates whether the last lux/CT
 * calculation was numerically valid (not saturated, not too dim).
 */
struct tcs34725_data {
	uint16_t clear;
	uint16_t red;
	uint16_t green;
	uint16_t blue;
	uint16_t lux;
	uint16_t color_temp;
	bool lux_valid;
};

/**
 * @brief Write a single byte to a TCS34725 register.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register address (5-bit register offset).
 * @param val Value to write to the register.
 *
 * @retval 0 on success.
 * @retval -errno Negative error code from the I2C API on failure.
 */
int tcs34725_reg_write(const struct device *dev, uint8_t reg, uint8_t val);

/**
 * @brief Read a single byte from a TCS34725 register.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register address (5-bit register offset).
 * @param[out] val Pointer to store the byte read from the register.
 *
 * @retval 0 on success.
 * @retval -errno Negative error code from the I2C API on failure.
 */
int tcs34725_reg_read(const struct device *dev, uint8_t reg, uint8_t *val);

/**
 * @brief Read a 16-bit (little-endian) value starting at a TCS34725 register.
 *
 * Uses auto-increment addressing to read two consecutive registers
 * (low byte then high byte) in a single I2C transaction.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Starting register address (5-bit register offset).
 * @param[out] val Pointer to store the assembled 16-bit value.
 *
 * @retval 0 on success.
 * @retval -errno Negative error code from the I2C API on failure.
 */
int tcs34725_reg_read16(const struct device *dev, uint8_t reg, uint16_t *val);

/**
 * @brief Power on and enable the TCS34725 RGBC ADC.
 *
 * Sets the PON bit, waits for the internal oscillator warm-up period,
 * then sets AEN (and WEN if wait time is configured in devicetree) to
 * start RGBC conversions.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 on success.
 * @retval -errno Negative error code on I2C failure.
 */
int tcs34725_enable(const struct device *dev);

/**
 * @brief Disable the TCS34725 (power down).
 *
 * Clears the ENABLE register, powering down the ADC and oscillator.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 on success.
 * @retval -errno Negative error code on I2C failure.
 */
int tcs34725_disable(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_TCS34725_TCS34725_H_ */
