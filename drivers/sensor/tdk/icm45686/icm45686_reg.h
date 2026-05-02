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
#define REG_READ_BIT BIT(7)

/* Helper Macros for register manipulation */
#define REG_PWR_MGMT0_ACCEL_MODE(val) ((val) & BIT_MASK(2))
#define REG_PWR_MGMT0_GYRO_MODE(val)  (((val) & BIT_MASK(2)) << 2)

#define REG_ACCEL_CONFIG0_ODR(val) ((val) & BIT_MASK(4))
#define REG_ACCEL_CONFIG0_FS(val)  (((val) & BIT_MASK(3)) << 4)

#define REG_GYRO_CONFIG0_ODR(val) ((val) & BIT_MASK(4))
#define REG_GYRO_CONFIG0_FS(val)  (((val) & BIT_MASK(4)) << 4)

#define REG_DRIVE_CONFIG0_SPI_SLEW(val) (((val) & BIT_MASK(2)) << 1)
#define REG_DRIVE_CONFIG1_I3C_SLEW(val) (((val) & BIT_MASK(3)) | (((val) & BIT_MASK(3)) << 3))
#define REG_MISC2_SOFT_RST(val)         ((val << 1) & BIT(1))

#define REG_IPREG_SYS1_REG_172_GYRO_LPFBW_SEL(val) (val & BIT_MASK(3))

#define REG_IPREG_SYS2_REG_131_ACCEL_LPFBW_SEL(val) (val & BIT_MASK(3))

#define REG_INT1_CONFIG0_STATUS_EN_DRDY(val)      (((val) & BIT_MASK(1)) << 2)
#define REG_INT1_CONFIG0_STATUS_EN_FIFO_THS(val)  (((val) & BIT_MASK(1)) << 1)
#define REG_INT1_CONFIG0_STATUS_EN_FIFO_FULL(val) ((val) & BIT_MASK(1))

#define REG_INT1_CONFIG2_EN_OPEN_DRAIN(val)  (((val) & BIT_MASK(1)) << 2)
#define REG_INT1_CONFIG2_EN_LATCH_MODE(val)  (((val) & BIT_MASK(1)) << 1)
#define REG_INT1_CONFIG2_EN_ACTIVE_HIGH(val) ((val) & BIT_MASK(1))

#define REG_INT1_STATUS0_DRDY(val)      (((val) & BIT_MASK(1)) << 2)
#define REG_INT1_STATUS0_FIFO_THS(val)  (((val) & BIT_MASK(1)) << 1)
#define REG_INT1_STATUS0_FIFO_FULL(val) ((val) & BIT_MASK(1))

#define REG_FIFO_CONFIG0_FIFO_MODE_BYPASS       0
#define REG_FIFO_CONFIG0_FIFO_MODE_STREAM       1
#define REG_FIFO_CONFIG0_FIFO_MODE_STOP_ON_FULL 2

#define REG_FIFO_CONFIG0_FIFO_DEPTH_2K 0x07
#define REG_FIFO_CONFIG0_FIFO_DEPTH_8K 0x1F

#define REG_FIFO_CONFIG0_FIFO_MODE(val)  (((val) & BIT_MASK(2)) << 6)
#define REG_FIFO_CONFIG0_FIFO_DEPTH(val) ((val) & BIT_MASK(6))

#define REG_FIFO_CONFIG1_0_FIFO_WM_THS(val) ((val) & BIT_MASK(8))
#define REG_FIFO_CONFIG1_1_FIFO_WM_THS(val) (((val) >> 8) & BIT_MASK(8))

#define REG_FIFO_CONFIG2_FIFO_FLUSH(val)     (((val) & BIT_MASK(1)) << 7)
#define REG_FIFO_CONFIG2_FIFO_WM_GT_THS(val) (((val) & BIT_MASK(1)) << 3)

#define REG_FIFO_CONFIG3_FIFO_HIRES_EN(val) (((val) & BIT_MASK(1)) << 3)
#define REG_FIFO_CONFIG3_FIFO_GYRO_EN(val)  (((val) & BIT_MASK(1)) << 2)
#define REG_FIFO_CONFIG3_FIFO_ACCEL_EN(val) (((val) & BIT_MASK(1)) << 1)
#define REG_FIFO_CONFIG3_FIFO_EN(val)       ((val) & BIT_MASK(1))

/* Misc. Defines */
#define WHO_AM_I_ICM45686 0xE9

#define REG_IREG_PREPARE_WRITE_ARRAY(base, reg, val)                                               \
	{                                                                                          \
		((base) >> 8) & 0xFF, reg, val                                                     \
	}

#define FIFO_HEADER_EXT_HEADER_EN(val) (((val) & BIT_MASK(1)) << 7)
#define FIFO_HEADER_ACCEL_EN(val)      (((val) & BIT_MASK(1)) << 6)
#define FIFO_HEADER_GYRO_EN(val)       (((val) & BIT_MASK(1)) << 5)
#define FIFO_HEADER_HIRES_EN(val)      (((val) & BIT_MASK(1)) << 4)

#define FIFO_NO_DATA            0x8000
#define FIFO_COUNT_MAX_HIGH_RES 104

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM45686_REG_H_ */
