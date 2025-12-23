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

#include <csr_context.h>

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

#ifdef CONFIG_SOC_OPENISA_RV32M1_RI5CY

/* Extra state for RI5CY hardware loop registers. */
#define SOC_ESF_MEMBERS					\
	CUSTOM_CSR_ESF_MEMBERS

/* Initial saved state. */
#define SOC_ESF_INIT					\
	CUSTOM_CSR_ESF_INIT

#endif /* CONFIG_SOC_OPENISA_RV32M1_RI5CY */

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#endif /* SOC_RISCV32_OPENISA_RV32M1_SOC_CONTEXT_H_ */
