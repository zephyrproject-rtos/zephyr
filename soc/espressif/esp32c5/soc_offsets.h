/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV_ESP32C5_SOC_OFFSETS_H
#define SOC_RISCV_ESP32C5_SOC_OFFSETS_H

#ifdef CONFIG_RISCV_SOC_OFFSETS

#define GEN_SOC_OFFSET_SYMS()                                                                      \
	GEN_OFFSET_SYM(soc_esf_t, mcause);                                                         \
	GEN_OFFSET_SYM(soc_esf_t, mintthresh)

#endif /* CONFIG_RISCV_SOC_OFFSETS */

#endif /* SOC_RISCV_ESP32C5_SOC_OFFSETS_H */
