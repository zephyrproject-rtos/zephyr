/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PNI_RM3100_REG_H_
#define ZEPHYR_DRIVERS_SENSOR_PNI_RM3100_REG_H_

/* Address value has a read bit */
#define REG_READ_BIT BIT(7)

/* RM3100 register addresses */
#define RM3100_REG_CMM         0x01 /* Continuous measurement mode */
#define RM3100_REG_CCX_MSB     0x04 /* Cycle Count X LSB */
#define RM3100_REG_CCX_LSB     0x05 /* Cycle Count X MSB */
#define RM3100_REG_CCY_MSB     0x06 /* Cycle Count Y LSB */
#define RM3100_REG_CCY_LSB     0x07 /* Cycle Count Y MSB */
#define RM3100_REG_CCZ_MSB     0x08 /* Cycle Count Z LSB */
#define RM3100_REG_CCZ_LSB     0x09 /* Cycle Count Z MSB */
#define RM3100_REG_TMRC        0x0B /* Continuous Mode Update Rate */
#define RM3100_REG_MX          0x24 /* Measurement results X (3 bytes) */
#define RM3100_REG_MY          0x27 /* Measurement results Y (3 bytes) */
#define RM3100_REG_MZ          0x2A /* Measurement results Z (3 bytes) */
#define RM3100_REG_STATUS      0x34 /* Status of DRDY */
#define RM3100_REG_REVID       0x36 /* Hardware revision ID */

/* Default values */
#define RM3100_REVID_VALUE     0x22   /* Expected REVID register value */

#define RM3100_CMM_ALL_AXIS 0x71

#define RM3100_CYCLE_COUNT_DEFAULT 0x00C8	/* Default Cycle Count value */
#define RM3100_CYCLE_COUNT_HIGH_ODR 0x0064	/* Cycle count value required for 600 Hz ODR */


#endif /* ZEPHYR_DRIVERS_SENSOR_PNI_RM3100_REG_H_ */
