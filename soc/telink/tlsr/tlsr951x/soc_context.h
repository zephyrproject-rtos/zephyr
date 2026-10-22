/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV_TELINK_B91_SOC_CONTEXT_H
#define SOC_RISCV_TELINK_B91_SOC_CONTEXT_H

#include <csr_context.h>

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

/* Telink B91 specific registers. */
#define SOC_ESF_MEMBERS	\
	CUSTOM_CSR_ESF_MEMBERS

#define SOC_ESF_INIT \
	CUSTOM_CSR_ESF_INIT

#endif  /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#endif  /* SOC_RISCV_TELINK_B91_SOC_CONTEXT_H */
