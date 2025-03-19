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
#include <zephyr/sys/byteorder.h>

/* Address value has a read bit */
#define REG_SPI_READ_BIT BIT(7)

/* Registers */
/* Register Bank 0 */
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
#define REG_FIFO_COUNT_0			0x12
#define REG_FIFO_COUNT_1			0x13
#define REG_FIFO_DATA				0x14
#define REG_INT1_CONFIG0			0x16
#define REG_INT1_CONFIG1			0x17
#define REG_INT1_CONFIG2			0x18
#define REG_INT1_STATUS0			0x19
#define REG_INT1_STATUS1			0x1A
#define REG_ACCEL_CONFIG0			0x1B
#define REG_GYRO_CONFIG0			0x1C
#define REG_FIFO_CONFIG0			0x1D
#define REG_FIFO_CONFIG1_0			0x1E
#define REG_FIFO_CONFIG1_1			0x1F
#define REG_FIFO_CONFIG2			0x20
#define REG_FIFO_CONFIG3			0x21
#define REG_FIFO_CONFIG4			0x22
#define REG_DRIVE_CONFIG0			0x32
#define REG_WHO_AM_I				0x72
#define REG_IREG_ADDR_15_8			0x7C
#define REG_IREG_ADDR_7_0			0x7D
#define REG_IREG_DATA				0x7E
#define REG_MISC2				0x7F

/* User Bank IPREG_SYS1 - Gyro-related config settings */
#define REG_IPREG_SYS1_OFFSET			0xA400
#define REG_IPREG_SYS1_REG_172			0xAC

/* User Bank IPREG_SYS2 - Accel-related config settings */
#define REG_IPREG_SYS2_OFFSET			0xA500
#define REG_IPREG_SYS2_REG_131			0x83

/* Helper Macros for register manipulation */
#define REG_PWR_MGMT0_ACCEL_MODE(val)			((val) & BIT_MASK(2))
#define REG_PWR_MGMT0_GYRO_MODE(val)			(((val) & BIT_MASK(2)) << 2)

#define REG_ACCEL_CONFIG0_ODR(val)			((val) & BIT_MASK(4))
#define REG_ACCEL_CONFIG0_FS(val)			(((val) & BIT_MASK(3)) << 4)

#define REG_GYRO_CONFIG0_ODR(val)			((val) & BIT_MASK(4))
#define REG_GYRO_CONFIG0_FS(val)			(((val) & BIT_MASK(4)) << 4)

#define REG_DRIVE_CONFIG0_SPI_SLEW(val)			(((val) & BIT_MASK(2)) << 1)

#define REG_MISC2_SOFT_RST(val)				((val << 1) & BIT(1))

#define REG_IPREG_SYS1_REG_172_GYRO_LPFBW_SEL(val)	(val & BIT_MASK(3))

#define REG_IPREG_SYS2_REG_131_ACCEL_LPFBW_SEL(val)	(val & BIT_MASK(3))

#define REG_INT1_CONFIG0_STATUS_EN_DRDY(val)		(((val) & BIT_MASK(1)) << 2)
#define REG_INT1_CONFIG0_STATUS_EN_FIFO_THS(val)	(((val) & BIT_MASK(1)) << 1)
#define REG_INT1_CONFIG0_STATUS_EN_FIFO_FULL(val)	((val) & BIT_MASK(1))

#define REG_INT1_CONFIG2_EN_OPEN_DRAIN(val)		(((val) & BIT_MASK(1)) << 2)
#define REG_INT1_CONFIG2_EN_LATCH_MODE(val)		(((val) & BIT_MASK(1)) << 1)
#define REG_INT1_CONFIG2_EN_ACTIVE_HIGH(val)		((val) & BIT_MASK(1))

#define REG_INT1_STATUS0_DRDY(val)			(((val) & BIT_MASK(1)) << 2)
#define REG_INT1_STATUS0_FIFO_THS(val)			(((val) & BIT_MASK(1)) << 1)
#define REG_INT1_STATUS0_FIFO_FULL(val)			((val) & BIT_MASK(1))

#define REG_FIFO_CONFIG0_FIFO_MODE_BYPASS		0
#define REG_FIFO_CONFIG0_FIFO_MODE_STREAM		1
#define REG_FIFO_CONFIG0_FIFO_MODE_STOP_ON_FULL		2

#define REG_FIFO_CONFIG0_FIFO_DEPTH_2K			0x07
#define REG_FIFO_CONFIG0_FIFO_DEPTH_8K			0x1F

#define REG_FIFO_CONFIG0_FIFO_MODE(val)			(((val) & BIT_MASK(2)) << 6)
#define REG_FIFO_CONFIG0_FIFO_DEPTH(val)		((val) & BIT_MASK(6))

#define REG_FIFO_CONFIG1_0_FIFO_WM_THS(val)		((val) & BIT_MASK(8))
#define REG_FIFO_CONFIG1_1_FIFO_WM_THS(val)		(((val) >> 8) & BIT_MASK(8))

#define REG_FIFO_CONFIG2_FIFO_FLUSH(val)		(((val) & BIT_MASK(1)) << 7)
#define REG_FIFO_CONFIG2_FIFO_WM_GT_THS(val)		(((val) & BIT_MASK(1)) << 3)

#define REG_FIFO_CONFIG3_FIFO_HIRES_EN(val)		(((val) & BIT_MASK(1)) << 3)
#define REG_FIFO_CONFIG3_FIFO_GYRO_EN(val)		(((val) & BIT_MASK(1)) << 2)
#define REG_FIFO_CONFIG3_FIFO_ACCEL_EN(val)		(((val) & BIT_MASK(1)) << 1)
#define REG_FIFO_CONFIG3_FIFO_EN(val)			((val) & BIT_MASK(1))

/* Misc. Defines */
#define WHO_AM_I_ICM45686 0xE9

#define REG_IREG_PREPARE_WRITE_ARRAY(base, reg, val)	{((base) >> 8) & 0xFF, reg, val}

#define FIFO_HEADER_EXT_HEADER_EN(val)			(((val) & BIT_MASK(1)) << 7)
#define FIFO_HEADER_ACCEL_EN(val)			(((val) & BIT_MASK(1)) << 6)
#define FIFO_HEADER_GYRO_EN(val)			(((val) & BIT_MASK(1)) << 5)
#define FIFO_HEADER_HIRES_EN(val)			(((val) & BIT_MASK(1)) << 4)

#define FIFO_NO_DATA					0x8000
#define FIFO_COUNT_MAX_HIGH_RES				104

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM45686_REG_H_ */
