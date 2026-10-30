/*
 * Copyright (c) 2026 Federico Borgo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FUEL_GAUGE_LTC3337_EMUL_LTC3337_H_
#define ZEPHYR_DRIVERS_FUEL_GAUGE_LTC3337_EMUL_LTC3337_H_

#include <stdint.h>

#include <zephyr/drivers/emul.h>

/**
 * @brief Set the content of an emulated register, including the read only ones.
 *
 * @param target Emulator of the LTC3337
 * @param reg Register sub-address
 * @param val 16-bit register value
 */
void ltc3337_emul_set_reg(const struct emul *target, uint8_t reg, uint16_t val);

/**
 * @brief Get the content of an emulated register, including the write only ones.
 *
 * @param target Emulator of the LTC3337
 * @param reg Register sub-address
 *
 * @return 16-bit register value
 */
uint16_t ltc3337_emul_get_reg(const struct emul *target, uint8_t reg);

#endif /* ZEPHYR_DRIVERS_FUEL_GAUGE_LTC3337_EMUL_LTC3337_H_ */
