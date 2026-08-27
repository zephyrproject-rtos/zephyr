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
#define REG_WHO_AM_I     (REG_BANK0_OFFSET | 0x00)
#define REG_USER_CTRL    (REG_BANK0_OFFSET | 0x03)
#define REG_PWR_MGMT_1   (REG_BANK0_OFFSET | 0x06)
#define REG_PWR_MGMT_2   (REG_BANK0_OFFSET | 0x07)
#define REG_ACCEL_XOUT_H (REG_BANK0_OFFSET | 0x2D)
#define REG_GYRO_XOUT_H  (REG_BANK0_OFFSET | 0x33)
#define REG_TEMP_OUT_H   (REG_BANK0_OFFSET | 0x39)

/* Bank 2 */
#define REG_GYRO_SMPLRT_DIV (REG_BANK2_OFFSET | 0x00)
#define REG_GYRO_CONFIG_1   (REG_BANK2_OFFSET | 0x01)
#define REG_ACCEL_CONFIG    (REG_BANK2_OFFSET | 0x14)

/* WHO_AM_I */
#define WHO_AM_I_ICM20948 0xEA

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

/* Accelerometer, gyroscope and temperature are read in one burst */
#define ICM20948_DATA_LEN 14

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM20948_REG_H_ */
