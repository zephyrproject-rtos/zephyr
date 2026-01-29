/*
 * Copyright (c) 2026 Om Srivastava <srivastavaom97714@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BH1750_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_BH1750_EMUL_H_

#include <stdint.h>
#include <stdbool.h>

struct bh1750_emul_data {
	bool powered;
	uint8_t mode;
	uint16_t raw;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_BH1750_EMUL_H_ */
