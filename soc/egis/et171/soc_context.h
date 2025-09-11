/*
 * Copyright (c) 2021 Andes Technology Corporation
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_RISCV_EGIS_ET171_SOC_CONTEXT_H__
#define __SOC_RISCV_EGIS_ET171_SOC_CONTEXT_H__

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

/* Egis et171 specific registers. */
#if defined(CONFIG_SOC_EGIS_ET171_PFT) && defined(CONFIG_SOC_EGIS_ET171_HWDSP)
	#define SOC_ESF_MEMBERS \
		uint32_t mxstatus;  \
		uint32_t ucode

	#define SOC_ESF_INIT \
		0,				 \
		0

#elif defined(CONFIG_SOC_EGIS_ET171_PFT)
	#define SOC_ESF_MEMBERS \
		uint32_t mxstatus

	#define SOC_ESF_INIT \
		0

#elif defined(CONFIG_SOC_EGIS_ET171_HWDSP)
	#define SOC_ESF_MEMBERS \
		uint32_t ucode

	#define SOC_ESF_INIT \
		0

#endif

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#endif /* __SOC_RISCV_EGIS_ET171_SOC_CONTEXT_H__ */
