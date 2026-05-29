/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Extra definitions required for CONFIG_RISCV_SOC_OFFSETS.
 */

#ifndef SOC_RISCV32_OPENISA_RV32M1_SOC_OFFSETS_H_
#define SOC_RISCV32_OPENISA_RV32M1_SOC_OFFSETS_H_

#include <fsl_device_registers.h>

#ifdef CONFIG_SOC_OPENISA_RV32M1_RI5CY

#include <csr_offsets.h>

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
/*
 * Ensure offset macros are available in <offsets.h>.
 *
 * Also create a macro which contains the value of &EVENT0->INTPTPENDCLEAR,
 * for use in assembly.
 */
#define GEN_SOC_OFFSET_SYMS()					\
	GEN_CUSTOM_CSR_OFFSET_SYMS();				\
	GEN_ABSOLUTE_SYM(__EVENT_INTPTPENDCLEAR,		\
			 (uint32_t)&EVENT0->INTPTPENDCLEAR)
#else

#define GEN_SOC_OFFSET_SYMS()					\
	GEN_ABSOLUTE_SYM(__EVENT_INTPTPENDCLEAR,		\
			 (uint32_t)&EVENT0->INTPTPENDCLEAR)

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#endif /* CONFIG_SOC_OPENISA_RV32M1_RI5CY */

#ifdef CONFIG_SOC_OPENISA_RV32M1_ZERO_RISCY

#define GEN_SOC_OFFSET_SYMS()					\
	GEN_ABSOLUTE_SYM(__EVENT_INTPTPENDCLEAR,		\
			 (uint32_t)&EVENT1->INTPTPENDCLEAR)

#endif /* CONFIG_SOC_OPENISA_RV32M1_ZERO_RISCY */

#endif /* SOC_RISCV32_OPENISA_RV32M1_SOC_OFFSETS_H_ */
