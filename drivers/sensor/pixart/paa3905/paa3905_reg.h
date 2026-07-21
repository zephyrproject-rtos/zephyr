/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PAA3905_REG_H_
#define ZEPHYR_DRIVERS_SENSOR_PAA3905_REG_H_

#define REG_SPI_READ_BIT			0
#define REG_SPI_WRITE_BIT			BIT(7)

/* Registers */
#define REG_PRODUCT_ID				0x00
#define REG_MOTION				0x02
#define REG_DELTA_X_L				0x03
#define REG_DELTA_X_H				0x04
#define REG_DELTA_Y_L				0x05
#define REG_DELTA_Y_H				0x06
#define REG_BURST_READ				0x16
#define REG_POWER_UP_RESET			0x3A
#define REG_RESOLUTION				0x4E

/* Misc. Defines */
#define PRODUCT_ID				0xA2
#define POWER_UP_RESET_VAL			0x5A

#define REG_MOTION_DETECTED(val)		((val) & BIT(7))
#define REG_MOTION_CHALLENGING_COND(val)	((val) & BIT(0))

#define REG_OBSERVATION_MODE(val)		(((val) & (BIT(7) | BIT(6))) >> 6)
#define REG_OBSERVATION_CHIP_OK(val)		(((val) & BIT_MASK(6)) == 0x3F)

#define OBSERVATION_MODE_BRIGHT			0
#define OBSERVATION_MODE_LOW_LIGHT		1
#define OBSERVATION_MODE_SUPER_LOW_LIGHT	2

/* Data validation boundaries */
#define SQUAL_MIN_BRIGHT			0x19
#define SHUTTER_MAX_BRIGHT			0x00FF80

#define SQUAL_MIN_LOW_LIGHT			0x46
#define SHUTTER_MAX_LOW_LIGHT			0x00FF80

#define SQUAL_MIN_SUPER_LOW_LIGHT		0x55
#define SHUTTER_MAX_SUPER_LOW_LIGHT		0x025998


#endif /* ZEPHYR_DRIVERS_SENSOR_PAA3905_REG_H_ */
