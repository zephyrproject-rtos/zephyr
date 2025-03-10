/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM45686_REG_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM45686_REG_H_

#include <zephyr/sys/util.h>

/* Address value has a read bit */
#define REG_SPI_READ_BIT BIT(7)

/* Registers */
#define REG_ACCEL_DATA_X1_UI			0x00
#define REG_ACCEL_DATA_X0_UI			0x01
#define REG_ACCEL_DATA_Y1_UI			0x02
#define REG_ACCEL_DATA_Y0_UI			0x03
#define REG_ACCEL_DATA_Z1_UI			0x04
#define REG_ACCEL_DATA_Z0_UI			0x05
#define REG_GYRO_DATA_X1_UI			0x06
#define REG_GYRO_DATA_X0_UI			0x07
#define REG_GYRO_DATA_Y1_UI			0x08
#define REG_GYRO_DATA_Y0_UI			0x09
#define REG_GYRO_DATA_Z1_UI			0x0A
#define REG_GYRO_DATA_Z0_UI			0x0B
#define REG_TEMP_DATA1_UI			0x0C
#define REG_TEMP_DATA0_UI			0x0D
#define REG_PWR_MGMT0				0x10
#define REG_INT1_CONFIG0			0x16
#define REG_INT1_CONFIG1			0x17
#define REG_INT1_CONFIG2			0x18
#define REG_INT1_STATUS0			0x19
#define REG_INT1_STATUS1			0x1A
#define REG_ACCEL_CONFIG0			0x1B
#define REG_GYRO_CONFIG0			0x1C
#define REG_DRIVE_CONFIG0			0x32
#define REG_WHO_AM_I				0x72
#define REG_MISC2				0x7F

/* Helper Macros for register manipulation */
#define REG_PWR_MGMT0_ACCEL_MODE(val)			((val) & BIT_MASK(2))
#define REG_PWR_MGMT0_GYRO_MODE(val)			(((val) & BIT_MASK(2)) << 2)

#define REG_ACCEL_CONFIG0_ODR(val)			((val) & BIT_MASK(4))
#define REG_ACCEL_CONFIG0_FS(val)			(((val) & BIT_MASK(3)) << 4)

#define REG_GYRO_CONFIG0_ODR(val)			((val) & BIT_MASK(4))
#define REG_GYRO_CONFIG0_FS(val)			(((val) & BIT_MASK(4)) << 4)

#define REG_DRIVE_CONFIG0_SPI_SLEW(val)			(((val) & BIT_MASK(2)) << 1)

#define REG_MISC2_SOFT_RST(val)				((val << 1) & BIT(1))

#define REG_INT1_CONFIG0_STATUS_EN_DRDY(val)		(((val) & BIT_MASK(1)) << 2)
#define REG_INT1_CONFIG0_STATUS_EN_FIFO_THS(val)	(((val) & BIT_MASK(1)) << 1)
#define REG_INT1_CONFIG0_STATUS_EN_FIFO_FULL(val)	((val) & BIT_MASK(1))

#define REG_INT1_CONFIG2_EN_OPEN_DRAIN(val)		(((val) & BIT_MASK(1)) << 2)
#define REG_INT1_CONFIG2_EN_LATCH_MODE(val)		(((val) & BIT_MASK(1)) << 1)
#define REG_INT1_CONFIG2_EN_ACTIVE_HIGH(val)		((val) & BIT_MASK(1))

#define REG_INT1_STATUS0_DRDY(val)			(((val) & BIT_MASK(1)) << 2)
#define REG_INT1_STATUS0_FIFO_THS(val)			(((val) & BIT_MASK(1)) << 1)
#define REG_INT1_STATUS0_FIFO_FULL(val)			((val) & BIT_MASK(1))

/* Misc. Defines */
#define WHO_AM_I_ICM45686 0xE9

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM45686_REG_H_ */
