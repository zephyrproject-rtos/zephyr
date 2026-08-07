/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Extra definitions required for CONFIG_RISCV_SOC_OFFSETS.
 */

#ifndef ZEPHYR_ARCH_RISCV_CUSTOM_OPENISA_RI5CY_CSR_OFFSETS_H_
#define ZEPHYR_ARCH_RISCV_CUSTOM_OPENISA_RI5CY_CSR_OFFSETS_H_

#ifdef CONFIG_RISCV_SOC_OFFSETS

#define GEN_CUSTOM_CSR_OFFSET_SYMS()				\
	GEN_OFFSET_SYM(soc_esf_t, lpstart0);			\
	GEN_OFFSET_SYM(soc_esf_t, lpend0);			\
	GEN_OFFSET_SYM(soc_esf_t, lpcount0);			\
	GEN_OFFSET_SYM(soc_esf_t, lpstart1);			\
	GEN_OFFSET_SYM(soc_esf_t, lpend1);			\
	GEN_OFFSET_SYM(soc_esf_t, lpcount1)

#endif /* CONFIG_RISCV_SOC_OFFSETS */

#endif /* ZEPHYR_ARCH_RISCV_CUSTOM_OPENISA_RI5CY_CSR_OFFSETS_H_ */
