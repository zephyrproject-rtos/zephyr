/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV_TELINK_B91_SOC_OFFSETS_H
#define SOC_RISCV_TELINK_B91_SOC_OFFSETS_H

#include <csr_offsets.h>

#ifdef CONFIG_RISCV_SOC_OFFSETS

/* Telink B91 specific registers. */
#define GEN_SOC_OFFSET_SYMS() \
	GEN_CUSTOM_CSR_OFFSET_SYMS()

#endif  /* CONFIG_RISCV_SOC_OFFSETS */

#endif  /* SOC_RISCV_TELINK_B91_SOC_OFFSETS_H*/
