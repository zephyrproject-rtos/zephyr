/*
 * Copyright (c) 2026, Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADT7420_ADT7420_EMUL_H
#define ZEPHYR_DRIVERS_SENSOR_ADT7420_ADT7420_EMUL_H

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>

#include "adt7420.h"

void adt7420_emul_set_reg(const struct emul *target, uint8_t reg_addr, const uint8_t *val);

void adt7420_emul_get_reg(const struct emul *target, uint8_t reg_addr, uint8_t *val);

void adt7420_emul_reset(const struct emul *target);

#endif /* ZEPHYR_DRIVERS_SENSOR_ADT7420_ADT7420_EMUL_H */
