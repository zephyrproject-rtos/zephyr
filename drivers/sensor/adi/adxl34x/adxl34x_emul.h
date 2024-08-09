/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL34X_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL34X_EMUL_H_

#include <stdint.h>

#include "adxl34x_reg.h"

/**
 * @brief Virtual registry of the adxl34x used in emulation mode
 * @struct adxl34x_emul_data
 */
struct adxl34x_emul_data {
	uint8_t reg[ADXL34X_REG_MAX + 1];
};

/**
 * @brief Virtual (static) configuration used in emulation mode
 * @struct adxl34x_emul_config
 */
struct adxl34x_emul_config {
	uint16_t addr; /**< Address of emulator */
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL34X_EMUL_H_ */
