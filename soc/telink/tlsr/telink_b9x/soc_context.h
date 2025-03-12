/*
 * Copyright (c) 2021-2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV_TELINK_B9X_SOC_CONTEXT_H
#define SOC_RISCV_TELINK_B9X_SOC_CONTEXT_H

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

/* Telink B91 specific registers. */
#if defined(CONFIG_TELINK_B9X_PFT_ARCH) && defined(__riscv_dsp)

#define SOC_ESF_MEMBERS                                                                            \
	uint32_t mxstatus;                                                                         \
	uint32_t ucode

#define SOC_ESF_INIT 0, 0

#elif defined(CONFIG_TELINK_B9X_PFT_ARCH)

#define SOC_ESF_MEMBERS uint32_t mxstatus

#define SOC_ESF_INIT 0

#elif defined(__riscv_dsp)

#define SOC_ESF_MEMBERS uint32_t ucode

#define SOC_ESF_INIT 0

#endif

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#endif /* SOC_RISCV_TELINK_B9X_SOC_CONTEXT_H */
