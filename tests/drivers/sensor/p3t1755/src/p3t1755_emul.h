/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_DRIVERS_SENSOR_P3T1755_EMUL_H_
#define ZEPHYR_TESTS_DRIVERS_SENSOR_P3T1755_EMUL_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>

void p3t1755_emul_reset(const struct emul *target);
void p3t1755_emul_set_temperature(const struct emul *target, int16_t sample);
uint8_t p3t1755_emul_get_config(const struct emul *target);
void p3t1755_emul_set_config(const struct emul *target, uint8_t config);
uint32_t p3t1755_emul_get_config_write_count(const struct emul *target);
void p3t1755_emul_set_read_failure(const struct emul *target, bool fail);
void p3t1755_emul_set_write_failure(const struct emul *target, bool fail);

#endif /* ZEPHYR_TESTS_DRIVERS_SENSOR_P3T1755_EMUL_H_ */
