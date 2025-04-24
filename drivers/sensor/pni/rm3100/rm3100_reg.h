/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PNI_RM3100_REG_H_
#define ZEPHYR_DRIVERS_SENSOR_PNI_RM3100_REG_H_

/* RM3100 register addresses */
#define RM3100_REG_CMM         0x01 /* Continuous measurement mode */
#define RM3100_REG_MX          0x24 /* Measurement results X (3 bytes) */
#define RM3100_REG_MY          0x27 /* Measurement results Y (3 bytes) */
#define RM3100_REG_MZ          0x2A /* Measurement results Z (3 bytes) */
#define RM3100_REG_STATUS      0x34 /* Status of DRDY */
#define RM3100_REG_REVID       0x36 /* Hardware revision ID */

/* Default values */
#define RM3100_REVID_VALUE     0x22   /* Expected REVID register value */

#define RM3100_CMM_ALL_AXIS 0x71

#endif /* ZEPHYR_DRIVERS_SENSOR_PNI_RM3100_REG_H_ */
