/*
 * Copyright (c) 2018 Foundries.io Ltd
 * Copyright (c) 2026 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Extra definitions required for CONFIG_RISCV_SOC_OFFSETS.
 */

#ifndef SOC_ADI_MAX32_RV32_SOC_OFFSETS_H_
#define SOC_ADI_MAX32_RV32_SOC_OFFSETS_H_

#ifdef CONFIG_SOC_FAMILY_MAX32_RV32
/*
 * Ensure offset macros are available in <offsets.h>.
 *
 * Also create a macro which contains the value of &EVENT0->INTPTPENDCLEAR,
 * for use in assembly.
 */
#define GEN_SOC_OFFSET_SYMS()                                                                      \
	GEN_OFFSET_SYM(soc_esf_t, lpstart0);                                                       \
	GEN_OFFSET_SYM(soc_esf_t, lpend0);                                                         \
	GEN_OFFSET_SYM(soc_esf_t, lpcount0);                                                       \
	GEN_OFFSET_SYM(soc_esf_t, lpstart1);                                                       \
	GEN_OFFSET_SYM(soc_esf_t, lpend1);                                                         \
	GEN_OFFSET_SYM(soc_esf_t, lpcount1);

#endif /* CONFIG_SOC_FAMILY_MAX32_RV32 */

#endif /* SOC_ADI_MAX32_RV32_SOC_OFFSETS_H_ */
