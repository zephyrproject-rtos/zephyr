/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Extra definitions required for CONFIG_RISCV_SOC_CONTEXT_SAVE.
 */

#ifndef SOC_RISCV32_OPENISA_RV32M1_SOC_CONTEXT_H_
#define SOC_RISCV32_OPENISA_RV32M1_SOC_CONTEXT_H_

#ifdef CONFIG_SOC_OPENISA_RV32M1_RI5CY

/* Extra state for RI5CY hardware loop registers. */
#define SOC_ESF_MEMBERS					\
	u32_t lpstart0;					\
	u32_t lpend0;						\
	u32_t lpcount0;					\
	u32_t lpstart1;					\
	u32_t lpend1;						\
	u32_t lpcount1

/* Initial saved state. */
#define SOC_ESF_INIT						\
	0xdeadbaad,						\
	0xdeadbaad,						\
	0xdeadbaad,						\
	0xdeadbaad,						\
	0xdeadbaad,						\
	0xdeadbaad

/*
 * Ensure offset macros are available in <offsets.h> for the above.
 *
 * Also create a macro which contains the value of &EVENT0->INTPTPENDCLEAR,
 * for use in assembly.
 */
#define GEN_SOC_OFFSET_SYMS()					\
	GEN_OFFSET_SYM(soc_esf_t, lpstart0);			\
	GEN_OFFSET_SYM(soc_esf_t, lpend0);			\
	GEN_OFFSET_SYM(soc_esf_t, lpcount0);			\
	GEN_OFFSET_SYM(soc_esf_t, lpstart1);			\
	GEN_OFFSET_SYM(soc_esf_t, lpend1);			\
	GEN_OFFSET_SYM(soc_esf_t, lpcount1);			\
	GEN_ABSOLUTE_SYM(__EVENT0_INTPTPENDCLEAR,		\
			 (uint32_t)&EVENT0->INTPTPENDCLEAR)

#endif /* CONFIG_SOC_OPENISA_RV32M1_RI5CY */

#endif /* SOC_RISCV32_OPENISA_RV32M1_SOC_CONTEXT_H_ */
