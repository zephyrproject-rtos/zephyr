/*
 * Copyright (c) 2026 Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MFD_EMUL_BQ25620_H_
#define ZEPHYR_DRIVERS_MFD_EMUL_BQ25620_H_

#include <stdint.h>

#include <zephyr/drivers/emul.h>

/**
 * @brief Set a register of the emulated BQ25620.
 *
 * Unlike an I2C write, this also sets read-only status and flag registers.
 *
 * @param target Emulator instance.
 * @param reg Register address.
 * @param val Register value.
 */
void emul_bq25620_set_reg(const struct emul *target, uint8_t reg, uint8_t val);

/**
 * @brief Get a register of the emulated BQ25620.
 *
 * Unlike an I2C read, this does not clear flag registers.
 *
 * @param target Emulator instance.
 * @param reg Register address.
 *
 * @return Register value.
 */
uint8_t emul_bq25620_get_reg(const struct emul *target, uint8_t reg);

#endif /* ZEPHYR_DRIVERS_MFD_EMUL_BQ25620_H_ */
