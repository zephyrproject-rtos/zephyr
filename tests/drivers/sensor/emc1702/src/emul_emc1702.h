/*
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EMUL_EMC1702_H
#define EMUL_EMC1702_H

#include <stdint.h>

/**
 * @brief Prepare the contents of an emulated 8-bit register
 *
 * @param target        Emulator pointer
 * @param reg_addr      Register address
 * @param value         Value to set in the emulated register
 */
void emc1702_emul_set_reg_8(const struct emul *target, uint8_t reg_addr, uint16_t value);

/**
 * @brief Reset the emulated registers
 *
 * @param target        Emulator pointer
 */
void emc1702_emul_reset(const struct emul *target);

#endif /* EMUL_EMC1702_H */
