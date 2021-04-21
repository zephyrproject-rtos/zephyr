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
	uint32_t lpstart0;					\
	uint32_t lpend0;						\
	uint32_t lpcount0;					\
	uint32_t lpstart1;					\
	uint32_t lpend1;						\
	uint32_t lpcount1

/* Initial saved state. */
#define SOC_ESF_INIT						\
	0,						\
	0,						\
	0,						\
	0,						\
	0,						\
	0

#endif /* CONFIG_SOC_OPENISA_RV32M1_RI5CY */

#endif /* SOC_RISCV32_OPENISA_RV32M1_SOC_CONTEXT_H_ */
