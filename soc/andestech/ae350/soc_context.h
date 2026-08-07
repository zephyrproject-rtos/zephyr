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

#include <csr_context.h>

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

/* Andes V5 specific registers. */

#define SOC_ESF_MEMBERS				\
	CUSTOM_CSR_ESF_MEMBERS

#define SOC_ESF_INIT				\
	CUSTOM_CSR_ESF_INIT

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#endif /* SOC_RISCV_ANDES_V5_SOC_CONTEXT_H_ */
