/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * Copyright (c) 2026 Arkadiusz Grzelka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ST_LSM6DSL_LSM6DSL_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_ST_LSM6DSL_LSM6DSL_EMUL_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>

/**
 * @brief Write one register of the emulated device.
 *
 * @param target Emulator instance.
 * @param reg Register address.
 * @param val Value to store.
 *
 * @retval 0 On success.
 * @retval -EIO If @p reg is outside the emulated register file.
 */
int lsm6dsl_emul_set_reg(const struct emul *target, uint8_t reg, uint8_t val);

/**
 * @brief Read one register of the emulated device.
 *
 * @param target Emulator instance.
 * @param reg Register address.
 * @param val Destination for the stored value.
 *
 * @retval 0 On success.
 * @retval -EIO If @p reg is outside the emulated register file.
 */
int lsm6dsl_emul_get_reg(const struct emul *target, uint8_t reg, uint8_t *val);

/**
 * @brief Set or clear every data-ready bit of STATUS_REG.
 *
 * @param target Emulator instance.
 * @param ready True to report accelerometer, gyroscope and temperature data as
 *        available, false to report none.
 */
void lsm6dsl_emul_set_data_ready(const struct emul *target, bool ready);

#endif /* ZEPHYR_DRIVERS_SENSOR_ST_LSM6DSL_LSM6DSL_EMUL_H_ */
