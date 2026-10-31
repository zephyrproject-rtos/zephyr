/*
 * Copyright (c) 2026 T3 Gemstone
 *	T3 Gemstone Developer Team <support@t3gemstone.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM20948_REG_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM20948_REG_H_

#include <zephyr/sys/util.h>

/* The most significant bit of the address selects a read transfer */
#define REG_SPI_READ_BIT BIT(7)

/*
 * Registers live in one of four user banks. They are encoded as 16 bit
 * values with the bank in the high byte and the address in the low byte,
 * so that a single constant carries everything the bus layer needs.
 */
#define REG_ADDRESS_MASK      GENMASK(7, 0)
#define REG_BANK_MASK         GENMASK(15, 8)
#define REG_BANK_OFFSET(bank) ((bank) << 8)
#define REG_BANK0_OFFSET      REG_BANK_OFFSET(0)
#define REG_BANK1_OFFSET      REG_BANK_OFFSET(1)
#define REG_BANK2_OFFSET      REG_BANK_OFFSET(2)
#define REG_BANK3_OFFSET      REG_BANK_OFFSET(3)

/* Bank select register, reachable from every bank */
#define REG_BANK_SEL   0x7F
#define MASK_USER_BANK GENMASK(5, 4)

/* Bank 0 */
#define REG_WHO_AM_I             (REG_BANK0_OFFSET | 0x00)
#define REG_USER_CTRL            (REG_BANK0_OFFSET | 0x03)
#define REG_LP_CONFIG            (REG_BANK0_OFFSET | 0x05)
#define REG_PWR_MGMT_1           (REG_BANK0_OFFSET | 0x06)
#define REG_PWR_MGMT_2           (REG_BANK0_OFFSET | 0x07)
#define REG_I2C_MST_STATUS       (REG_BANK0_OFFSET | 0x17)
#define REG_ACCEL_XOUT_H         (REG_BANK0_OFFSET | 0x2D)
#define REG_GYRO_XOUT_H          (REG_BANK0_OFFSET | 0x33)
#define REG_TEMP_OUT_H           (REG_BANK0_OFFSET | 0x39)
#define REG_EXT_SLV_SENS_DATA_00 (REG_BANK0_OFFSET | 0x3B)

/* Bank 2 */
#define REG_GYRO_SMPLRT_DIV (REG_BANK2_OFFSET | 0x00)
#define REG_GYRO_CONFIG_1   (REG_BANK2_OFFSET | 0x01)
#define REG_ACCEL_CONFIG    (REG_BANK2_OFFSET | 0x14)

/* Bank 3 */
#define REG_I2C_MST_ODR_CFG (REG_BANK3_OFFSET | 0x00)
#define REG_I2C_MST_CTRL    (REG_BANK3_OFFSET | 0x01)
#define REG_I2C_SLV0_ADDR   (REG_BANK3_OFFSET | 0x03)
#define REG_I2C_SLV0_REG    (REG_BANK3_OFFSET | 0x04)
#define REG_I2C_SLV0_CTRL   (REG_BANK3_OFFSET | 0x05)
#define REG_I2C_SLV4_ADDR   (REG_BANK3_OFFSET | 0x13)
#define REG_I2C_SLV4_REG    (REG_BANK3_OFFSET | 0x14)
#define REG_I2C_SLV4_CTRL   (REG_BANK3_OFFSET | 0x15)
#define REG_I2C_SLV4_DO     (REG_BANK3_OFFSET | 0x16)
#define REG_I2C_SLV4_DI     (REG_BANK3_OFFSET | 0x17)

/* WHO_AM_I */
#define WHO_AM_I_ICM20948 0xEA

/* USER_CTRL */
#define BIT_I2C_MST_EN  BIT(5)
#define BIT_I2C_IF_DIS  BIT(4)
#define BIT_I2C_MST_RST BIT(1)

/* LP_CONFIG */
#define BIT_I2C_MST_CYCLE BIT(6)

/* PWR_MGMT_1 */
#define BIT_DEVICE_RESET BIT(7)
#define BIT_SLEEP        BIT(6)
#define BIT_LP_EN        BIT(5)
#define BIT_TEMP_DIS     BIT(3)
#define MASK_CLKSEL      GENMASK(2, 0)
#define CLKSEL_AUTO      1

/* GYRO_CONFIG_1 */
#define MASK_GYRO_DLPFCFG GENMASK(5, 3)
#define MASK_GYRO_FS_SEL  GENMASK(2, 1)
#define BIT_GYRO_FCHOICE  BIT(0)

/* ACCEL_CONFIG */
#define MASK_ACCEL_DLPFCFG GENMASK(5, 3)
#define MASK_ACCEL_FS_SEL  GENMASK(2, 1)
#define BIT_ACCEL_FCHOICE  BIT(0)

/* I2C_MST_STATUS */
#define BIT_I2C_SLV4_DONE BIT(6)
#define BIT_I2C_SLV4_NACK BIT(4)
#define BIT_I2C_SLV0_NACK BIT(0)

/* I2C_MST_ODR_CFG, the cycle rate is the base rate halved once per step */
#define MASK_I2C_MST_ODR GENMASK(3, 0)
#define I2C_MST_ODR_MAX  0

/* I2C_MST_CTRL */
#define BIT_I2C_MST_P_NSR BIT(4)
#define MASK_I2C_MST_CLK  GENMASK(3, 0)
#define I2C_MST_CLK_345K  7

/* I2C_SLV0_ADDR, I2C_SLV0_CTRL and their SLV4 counterparts */
#define BIT_I2C_SLV_RNW   BIT(7)
#define BIT_I2C_SLV_EN    BIT(7)
#define MASK_I2C_SLV_LENG GENMASK(3, 0)

/* Accelerometer, gyroscope and temperature are read in one burst */
#define ICM20948_DATA_LEN 14

/* ST1, the three axes, a dummy byte and ST2 of the magnetometer follow it */
#define ICM20948_MAGN_LEN 9

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM20948_REG_H_ */
