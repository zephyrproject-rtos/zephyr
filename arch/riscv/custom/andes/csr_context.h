/*
 * Copyright (c) 2025 Andes Technology Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Extra definitions required for CONFIG_RISCV_SOC_CONTEXT_SAVE.
 */

#ifndef ZEPHYR_ARCH_RISCV_CUSTOM_ANDES_CSR_CONTEXT_H_
#define ZEPHYR_ARCH_RISCV_CUSTOM_ANDES_CSR_CONTEXT_H_

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

/* Andes V5 specific registers. */
#if defined(CONFIG_RISCV_CUSTOM_CSR_ANDES_PFT) && \
	defined(CONFIG_RISCV_CUSTOM_CSR_ANDES_HWDSP)
	#define CUSTOM_CSR_ESF_MEMBERS			\
		uint32_t mxstatus;			\
		uint32_t ucode				\

	#define CUSTOM_CSR_ESF_INIT			\
		0,					\
		0

#elif defined(CONFIG_RISCV_CUSTOM_CSR_ANDES_PFT)
	#define CUSTOM_CSR_ESF_MEMBERS			\
		uint32_t mxstatus

	#define CUSTOM_CSR_ESF_INIT			\
		0

#elif defined(CONFIG_RISCV_CUSTOM_CSR_ANDES_HWDSP)
	#define CUSTOM_CSR_ESF_MEMBERS			\
		uint32_t ucode

	#define CUSTOM_CSR_ESF_INIT			\
		0

#endif

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#endif /* ZEPHYR_ARCH_RISCV_CUSTOM_ANDES_CSR_CONTEXT_H_ */
