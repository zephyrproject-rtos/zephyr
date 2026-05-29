/*
 * Copyright (c) 2025 Jonas Berg
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EMUL_INA228_H
#define EMUL_INA228_H

#include <stdint.h>

#define INA228_UINT20_MAX 0xFFFFF
#define INA228_INT20_MAX  0x7FFFF
#define INA228_UINT24_MAX 0xFFFFFF
#define INA228_UINT40_MAX 0xFFFFFFFFFF
#define INA228_INT40_MAX  0x7FFFFFFFFF

/**
 * @brief Prepare the contents of an emulated 16-bit register
 *
 * @param target        Emulator pointer
 * @param reg_addr      Register address
 * @param value         Value to set in the emulated register
 */
void ina228_emul_set_reg_16(const struct emul *target, uint8_t reg_addr, uint16_t value);

/**
 * @brief Prepare the contents of an emulated 24-bit register
 *
 * @param target        Emulator pointer
 * @param reg_addr      Register address
 * @param value         Value to set in the emulated register
 */
void ina228_emul_set_reg_24(const struct emul *target, uint8_t reg_addr, uint32_t value);

/**
 * @brief Prepare the contents of an emulated 40-bit register
 *
 * @param target        Emulator pointer
 * @param reg_addr      Register address
 * @param value         Value to set in the emulated register
 */
void ina228_emul_set_reg_40(const struct emul *target, uint8_t reg_addr, uint64_t value);

/**
 * @brief Read back the contents of an emulated 16-bit register
 *
 * @param target        Emulator pointer
 * @param reg_addr      Register address
 * @param value         Resulting value from the emulated register
 */
void ina228_emul_get_reg_16(const struct emul *target, uint8_t reg_addr, uint16_t *value);

/**
 * @brief Reset the emulated registers
 *
 * @param target        Emulator pointer
 */
void ina228_emul_reset(const struct emul *target);

#endif /* EMUL_INA228_H */
