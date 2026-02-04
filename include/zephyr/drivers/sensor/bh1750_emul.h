/*
 * Copyright (c) 2026 Om Srivastava <srivastavaom97714@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief BH1750 sensor emulator API
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BH1750_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_BH1750_EMUL_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief BH1750 emulator internal state
 */
struct bh1750_emul_data {
	/**
	 * Indicates whether the emulator is powered on.
	 */
	bool powered;

	/**
	 * Measurement mode configured in the emulator.
	 */
	uint8_t mode;

	/**
	 * Raw sensor value returned by the emulator.
	 */
	uint16_t raw;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_BH1750_EMUL_H_ */
