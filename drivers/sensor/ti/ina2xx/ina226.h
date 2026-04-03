/*
 * Copyright 2024 NXP
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA2XX_INA226_H_
#define ZEPHYR_DRIVERS_SENSOR_INA2XX_INA226_H_

#include "ina2xx_common.h"

/* Device register addresses. */
#define INA226_REG_CONFIG			0x00
#define INA226_REG_SHUNT_VOLT		0x01
#define INA226_REG_BUS_VOLT			0x02
#define INA226_REG_POWER			0x03
#define INA226_REG_CURRENT			0x04
#define INA226_REG_CALIB			0x05
#define INA226_REG_MASK				0x06
#define INA226_REG_ALERT			0x07
#define INA226_REG_MANUFACTURER_ID	0xFE
#define INA226_REG_DEVICE_ID		0xFF

#endif /* ZEPHYR_DRIVERS_SENSOR_INA2XX_INA226_H_ */
