/*
 * Copyright (c) 2025 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Extra definitions required for CONFIG_RISCV_SOC_OFFSETS.
 */

#ifndef SOC_RISCV_EGIS_ET171_SOC_OFFSETS_H_
#define SOC_RISCV_EGIS_ET171_SOC_OFFSETS_H_

#include <csr_offsets.h>

#ifdef CONFIG_RISCV_SOC_OFFSETS

/* Egis ET171 specific registers. */
#define GEN_SOC_OFFSET_SYMS()			\
	GEN_CUSTOM_CSR_OFFSET_SYMS()

#endif /* CONFIG_RISCV_SOC_OFFSETS */

#endif /* SOC_RISCV_EGIS_ET171_SOC_OFFSETS_H_*/
