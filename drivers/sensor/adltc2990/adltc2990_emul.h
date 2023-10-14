/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADLTC2990_ADLTC2990_EMUL_H
#define ZEPHYR_DRIVERS_SENSOR_ADLTC2990_ADLTC2990_EMUL_H

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>

void adltc2990_emul_set_reg(const struct emul *target, uint8_t reg_addr, const uint8_t *val);

void adltc2990_emul_get_reg(const struct emul *target, uint8_t reg_addr, uint8_t *val);

void adltc2990_emul_reset(const struct emul *target);

#endif /* ZEPHYR_DRIVERS_SENSOR_ADLTC2990_ADLTC2990_EMUL_H */
