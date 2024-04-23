/*
 * Copyright (C) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV_NORDIC_NRF_COMMON_VPR_SOC_CONTEXT_H_
#define SOC_RISCV_NORDIC_NRF_COMMON_VPR_SOC_CONTEXT_H_

#define SOC_ESF_MEMBERS						\
	unsigned long minttresh;				\
	unsigned long sp_align;					\
	unsigned long padding0;					\
	unsigned long padding1;					\
	unsigned long padding2

#define SOC_ESF_INIT						\
	0,							\
	0,							\
	0,							\
	0

#endif /* SOC_RISCV_NORDIC_NRF_COMMON_VPR_SOC_CONTEXT_H_ */
