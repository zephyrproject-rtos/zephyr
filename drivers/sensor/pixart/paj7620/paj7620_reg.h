/*
 * Copyright (c) 2025 Paul Timke <ptimkec@live.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PAJ7620_REG_H_
#define ZEPHYR_DRIVERS_SENSOR_PAJ7620_REG_H_

#include <stdint.h>

/**
 * @file
 * @brief PAJ7620 Register addresses and values. Not all registers from the
 * data sheet are listed here, only the ones directly used by the driver.
 * For more information about the registers, refer to:
 * https://files.seeedstudio.com/wiki/Grove_Gesture_V_1.0/res/PAJ7620U2_DS_v1.5_05012022_Confidential.pdf
 */

/**
 * BANK 0 REGISTER ADDRESSES
 */

/* Chip / Version ID */
#define PAJ7620_REG_PART_ID_LSB 0x00
#define PAJ7620_REG_PART_ID_MSB 0x01

/** Register bank select */
#define PAJ7620_REG_BANK_SEL 0xEF

/** Interrupt Controls */
/* Note: If the corresponding bit is 1, the corresponding interrupt is enabled */
#define PAJ7620_REG_MCU_INT_CTRL 0x40 /* Configure auto clean and active high/low */
#define PAJ7620_REG_INT_1_EN     0x41 /* Enable int 1 interrupts */
#define PAJ7620_REG_INT_2_EN     0x42 /* Enable int 2 interrupts */
#define PAJ7620_REG_INT_FLAG_1   0x43 /* Gesture detection results */
#define PAJ7620_REG_INT_FLAG_2   0x44 /* Gesture detection results (wave and others) */

/**
 * BANK 0 REGISTER VALUES AND MASKS
 */
#define PAJ7620_VAL_PART_ID_LSB 0x20
#define PAJ7620_VAL_PART_ID_MSB 0x76

/** Register bank select values */
#define PAJ7620_VAL_BANK_SEL_BANK_0 0x00
#define PAJ7620_VAL_BANK_SEL_BANK_1 0x01

/** Interrupt controls masks and values */
#define PAJ7620_MASK_MCU_INT_FLAG_GCLR               BIT(1)
#define PAJ7620_VAL_MCU_INT_FLAG_AUTO_CLEAN_DISABLE  0x00
#define PAJ7620_VAL_MCU_INT_FLAG_AUTO_CLEAN_ENABLE   0x01

#define PAJ7620_MASK_MCU_INT_FLAG_INV                BIT(4)
#define PAJ7620_VAL_MCU_INT_FLAG_PIN_ACTIVE_LOW      0x00
#define PAJ7620_VAL_MCU_INT_FLAG_PIN_ACTIVE_HIGH     0x01

#define PAJ7620_MASK_ALL_GESTURE_INTS_ENABLE         0xFF
#define PAJ7620_MASK_ALL_GESTURE_INTS_DISABLE        0x00

/**
 * BANK 1 REGISTER ADDRESSES
 */
#define PAJ7620_REG_R_IDLE_TIME_LSB 0x65
#define PAJ7620_REG_R_IDLE_TIME_MSB 0x66

/**
 * INITIALIZATION ARRAYS
 * The following 'initial_register_array' is taken 'as is' from Section 8
 * (Firmware Guides) of the PAJ7620 datasheet v1.5.
 * It encodes pairs of register addresses and values for those registers
 * needed to initialize the sensor or change its operation mode
 *
 * Reference:
 * https://files.seeedstudio.com/wiki/Grove_Gesture_V_1.0/res/PAJ7620U2_DS_v1.5_05012022_Confidential.pdf
 */

static const uint8_t initial_register_array[][2] = {
	{0xEF, 0x00}, /* Select memory bank 0 */
	{0x41, 0xFF}, /* Enable gesture interrupts */
	{0x42, 0x01},
	{0x46, 0x2D},
	{0x47, 0x0F},
	{0x48, 0x80},
	{0x49, 0x00},
	{0x4A, 0x40},
	{0x4B, 0x00},
	{0x4C, 0x20},
	{0x4D, 0x00},
	{0x51, 0x10},
	{0x5C, 0x02},
	{0x5E, 0x10},
	{0x80, 0x41},
	{0x81, 0x44},
	{0x82, 0x0C},
	{0x83, 0x20},
	{0x84, 0x20},
	{0x85, 0x00},
	{0x86, 0x10},
	{0x87, 0x00},
	{0x8B, 0x01},
	{0x8D, 0x00},
	{0x90, 0x0C},
	{0x91, 0x0C},
	{0x93, 0x0D},
	{0x94, 0x0A},
	{0x95, 0x0A},
	{0x96, 0x0C},
	{0x97, 0x05},
	{0x9A, 0x14},
	{0x9C, 0x3F},
	{0x9F, 0xF9},
	{0xA0, 0x48},
	{0xA5, 0x19},
	{0xCC, 0x19},
	{0xCD, 0x0B},
	{0xCE, 0x13},
	{0xCF, 0x62},
	{0xD0, 0x21},
	{0xEF, 0x01}, /* Select memory bank 1 */
	{0x00, 0x1E},
	{0x01, 0x1E},
	{0x02, 0x0F},
	{0x03, 0x0F},
	{0x04, 0x02},
	{0x25, 0x01},
	{0x26, 0x00},
	{0x27, 0x39},
	{0x28, 0x7F},
	{0x29, 0x08},
	{0x30, 0x03},
	{0x3E, 0xFF},
	{0x5E, 0x3D},
	{0x65, 0xAC}, /* Set fps to 'normal' mode */
	{0x66, 0x00},
	{0x67, 0x97},
	{0x68, 0x01},
	{0x69, 0xCD},
	{0x6A, 0x01},
	{0x6B, 0xB0},
	{0x6C, 0x04},
	{0x6D, 0x2C},
	{0x6E, 0x01},
	{0x72, 0x01},
	{0x73, 0x35},
	{0x74, 0x00}, /* Set to gesture mode */
	{0x77, 0x01},
	{0xEF, 0x00}, /* Reselect memory bank 0 */
};

#endif /* ZEPHYR_DRIVERS_SENSOR_PAJ7620_REG_H_ */
