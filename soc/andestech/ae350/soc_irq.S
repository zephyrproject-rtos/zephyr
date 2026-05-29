/*
 * Copyright (c) 2021 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/offsets.h>
#include <zephyr/toolchain.h>
#include <csr_irq.inc>

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

/* Exports */
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)

SECTION_FUNC(exception.other, __soc_save_context)

	__custom_csr_save_context a0, t0

	ret

SECTION_FUNC(exception.other, __soc_restore_context)

	__custom_csr_restore_context a0, t0

	ret

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */
