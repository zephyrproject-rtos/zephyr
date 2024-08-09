/*
 * Copyright (c) 2021 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Extra definitions required for CONFIG_RISCV_SOC_CONTEXT_SAVE.
 */

#ifndef SOC_RISCV_ANDES_V5_SOC_CONTEXT_H_
#define SOC_RISCV_ANDES_V5_SOC_CONTEXT_H_

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

/* Andes V5 specific registers. */
#if defined(CONFIG_SOC_ANDES_V5_PFT) && defined(CONFIG_SOC_ANDES_V5_HWDSP)
	#define SOC_ESF_MEMBERS				\
		unsigned long mxstatus;			\
		unsigned long ucode			\

	#define SOC_ESF_INIT				\
		0,					\
		0

#elif defined(CONFIG_SOC_ANDES_V5_PFT)
	#define SOC_ESF_MEMBERS				\
		unsigned long mxstatus

	#define SOC_ESF_INIT				\
		0

#elif defined(CONFIG_SOC_ANDES_V5_HWDSP)
	#define SOC_ESF_MEMBERS				\
		unsigned long ucode

	#define SOC_ESF_INIT				\
		0

#endif

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#endif /* SOC_RISCV_ANDES_V5_SOC_CONTEXT_H_ */
