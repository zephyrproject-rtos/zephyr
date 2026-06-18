/*
 * Copyright (c) 2026 Junseo Pyun
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/emul.h>
#include <stdint.h>

#define NUM_REGS 16

#ifndef ZEPHYR_DRIVERS_SENSOR_TMP11X_TMP11X_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_TMP11X_TMP11X_EMUL_H_

void tmp11x_emul_write_reg(const struct emul *target, uint8_t reg, uint16_t val);

#endif
