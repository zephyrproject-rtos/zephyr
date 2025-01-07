/*
 * Copyright (C) 2025 Andrew Featherstone
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

#include "hardware/platform_defs.h"
#include "hardware/regs/rvcsr.h"

/* imports */
#ifdef CONFIG_TRACING
GTEXT(sys_trace_isr_enter)
GTEXT(sys_trace_isr_exit)
#endif

/* exports */
GTEXT(__soc_handle_all_irqs)

/*
 * This function services and clears all pending interrupts for an CLIC in non-vectored mode.
 */
SECTION_FUNC(exception.other, __soc_handle_all_irqs)
	/* Get the highest-priority external interrupt. */
	csrr a0, RVCSR_MEINEXT_OFFSET
	bltz a0, irq_done

	addi sp, sp, -4
	sw ra, 0(sp)
irq_loop:

#ifdef CONFIG_TRACING_ISR
	call sys_trace_isr_enter
#endif
	/*
	 * Call corresponding registered function in _sw_isr_table.
	 * Value from MEINEXT is index left shifted by 2, so shift
	 * by one to get offset into 2-word wide table.
	 */
	la t0, _sw_isr_table
	slli a0, a0, (1)
	add t0, t0, a0

	/* Load argument in a0 register */
	lw a0, 0(t0)

	/* Load ISR function address in register t1 */
	lw t1, 4(t0)

	/* Call ISR function */
	jalr ra, t1, 0

#ifdef CONFIG_TRACING_ISR
	call sys_trace_isr_exit
#endif

	/* Get the next highest interrupt, and process that, or continue to done. */
	csrr a0, RVCSR_MEINEXT_OFFSET
	bgez a0, irq_loop

	lw ra, 0(sp)
	addi sp, sp, 4
irq_done:
	ret
