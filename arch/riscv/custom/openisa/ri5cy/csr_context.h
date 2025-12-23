/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Extra definitions required for CONFIG_RISCV_SOC_CONTEXT_SAVE.
 */

#ifndef ZEPHYR_ARCH_RISCV_CUSTOM_OPENISA_RI5CY_CSR_CONTEXT_H_
#define ZEPHYR_ARCH_RISCV_CUSTOM_OPENISA_RI5CY_CSR_CONTEXT_H_

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

/* Extra state for RI5CY hardware loop registers. */
#define CUSTOM_CSR_ESF_MEMBERS					\
	uint32_t lpstart0;					\
	uint32_t lpend0;					\
	uint32_t lpcount0;					\
	uint32_t lpstart1;					\
	uint32_t lpend1;					\
	uint32_t lpcount1

/* Initial saved state. */
#define CUSTOM_CSR_ESF_INIT					\
	0,							\
	0,							\
	0,							\
	0,							\
	0,							\
	0

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#endif /* ZEPHYR_ARCH_RISCV_CUSTOM_OPENISA_RI5CY_CSR_CONTEXT_H_ */
