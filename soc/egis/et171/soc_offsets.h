/*
 * Copyright (c) 2021 Andes Technology Corporation
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_RISCV_EGIS_ET171_SOC_OFFSETS_H__
#define __SOC_RISCV_EGIS_ET171_SOC_OFFSETS_H__

#ifdef CONFIG_RISCV_SOC_OFFSETS

/* Egis et171 specific registers. */
#if defined(CONFIG_SOC_EGIS_ET171_PFT) && defined(CONFIG_SOC_EGIS_ET171_HWDSP)
	#define GEN_SOC_OFFSET_SYMS()			\
		GEN_OFFSET_SYM(soc_esf_t, mxstatus);	\
		GEN_OFFSET_SYM(soc_esf_t, ucode)

#elif defined(CONFIG_SOC_EGIS_ET171_PFT)
	#define GEN_SOC_OFFSET_SYMS()			\
		GEN_OFFSET_SYM(soc_esf_t, mxstatus)

#elif defined(CONFIG_SOC_EGIS_ET171_HWDSP)
	#define GEN_SOC_OFFSET_SYMS()			\
		GEN_OFFSET_SYM(soc_esf_t, ucode)

#endif

#endif /* CONFIG_RISCV_SOC_OFFSETS */

#endif /* __SOC_RISCV_EGIS_ET171_SOC_OFFSETS_H__*/
