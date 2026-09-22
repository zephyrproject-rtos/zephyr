/*
 * Copyright (c) 2025 Andes Technology Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Extra definitions required for CONFIG_RISCV_SOC_OFFSETS.
 */

#ifndef ZEPHYR_ARCH_RISCV_CUSTOM_ANDES_CSR_OFFSETS_H_
#define ZEPHYR_ARCH_RISCV_CUSTOM_ANDES_CSR_OFFSETS_H_

#ifdef CONFIG_RISCV_SOC_OFFSETS

/* Andes V5 specific registers. */
#if defined(CONFIG_RISCV_CUSTOM_CSR_ANDES_PFT) && \
	defined(CONFIG_RISCV_CUSTOM_CSR_ANDES_HWDSP)
	#define GEN_CUSTOM_CSR_OFFSET_SYMS()		\
		GEN_OFFSET_SYM(soc_esf_t, mxstatus);	\
		GEN_OFFSET_SYM(soc_esf_t, ucode)

#elif defined(CONFIG_RISCV_CUSTOM_CSR_ANDES_PFT)
	#define GEN_CUSTOM_CSR_OFFSET_SYMS()		\
		GEN_OFFSET_SYM(soc_esf_t, mxstatus)

#elif defined(CONFIG_RISCV_CUSTOM_CSR_ANDES_HWDSP)
	#define GEN_CUSTOM_CSR_OFFSET_SYMS()		\
		GEN_OFFSET_SYM(soc_esf_t, ucode)

#endif

#endif /* CONFIG_RISCV_SOC_OFFSETS */

#endif /* ZEPHYR_ARCH_RISCV_CUSTOM_ANDES_CSR_OFFSETS_H_ */
