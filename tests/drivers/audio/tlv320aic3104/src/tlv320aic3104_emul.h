/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

uint8_t tlv320aic3104_emul_last_val(uint8_t page, uint8_t addr);

void tlv320aic3104_emul_set_val(uint8_t page, uint8_t addr, uint8_t val);

void tlv320aic3104_emul_fail_write_at(uint8_t page, uint8_t addr);

void tlv320aic3104_emul_reset(void);
