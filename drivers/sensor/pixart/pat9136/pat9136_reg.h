/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PAT9136_REG_H_
#define ZEPHYR_DRIVERS_SENSOR_PAT9136_REG_H_

#define REG_SPI_READ_BIT			0
#define REG_SPI_WRITE_BIT			BIT(7)

/* Registers */
#define REG_PRODUCT_ID				0x00
#define REG_MOTION				0x02
#define REG_OBSERVATION				0x15
#define REG_BURST_READ				0x16
#define REG_POWER_UP_RESET			0x3A
#define REG_RESOLUTION_SET			0x47
#define REG_RESOLUTION_X_LOWER			0x48
#define REG_RESOLUTION_X_UPPER			0x49
#define REG_RESOLUTION_Y_LOWER			0x4A
#define REG_RESOLUTION_Y_UPPER			0x4B

/* Misc. Defines */
#define PRODUCT_ID				0x4F
#define POWER_UP_RESET_VAL			0x5A

#define REG_MOTION_DETECTED(val)		((val) & BIT(7))

#define REG_OBSERVATION_READ_IS_VALID(val)	(((val) == 0xB7) || ((val) == 0xBF))

#endif /* ZEPHYR_DRIVERS_SENSOR_PAT9136_REG_H_ */
