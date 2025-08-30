/*
 * Copyright (c) 2024 Baumer Electric AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Assembler-hooks specific to RISC-V Core Local Interrupt Controller
 */

#include <zephyr/arch/cpu.h>
#include "clic.h"


/* register-wide load/store based on lw/sw (XLEN = 32) */

.macro lr, rd, mem
lw \rd, \mem
.endm

.macro sr, rs, mem
sw \rs, \mem
.endm


GTEXT(__soc_handle_irq)
/*
 * In an CLIC, pending interrupts don't have to be cleared by hand.
 * In vectored mode, interrupts are cleared automatically.
 * In non-vectored mode, interrupts are cleared when writing the mnxti register (done in
 * __soc_handle_all_irqs).
 * Thus this function can directly return.
 */
SECTION_FUNC(exception.other, __soc_handle_irq)
	ret

#ifdef CONFIG_TRACING
/* imports */
GTEXT(sys_trace_isr_enter)
GTEXT(sys_trace_isr_exit)
#endif
