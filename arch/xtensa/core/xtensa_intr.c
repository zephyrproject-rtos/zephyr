/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Xtensa-specific interrupt and exception functions for RTOS ports.
 * Also see xtensa_intr_asm.S.
 */

#include <stdlib.h>

#include <xtensa/config/core.h>
#include "xtensa_rtos.h"
#include "xtensa_api.h"
#include <kernel_structs.h>
#include <sw_isr_table.h>

#if XCHAL_HAVE_EXCEPTIONS
static void unhandled_exception_trampoline(XtExcFrame *frame)
{
	FatalErrorHandler();
	CODE_UNREACHABLE;
}

typedef void (*xt_exc_handler)(XtExcFrame *);

xt_exc_handler _xt_exception_table[XCHAL_EXCCAUSE_NUM] __aligned(4) = {
	[0 ... (XCHAL_EXCCAUSE_NUM - 1)] = unhandled_exception_trampoline
};
#endif

#if defined(CONFIG_SW_ISR_TABLE) && defined(XCHAL_HAVE_INTERRUPTS)
void _irq_spurious(void *arg)
{
	ReservedInterruptHandler((unsigned int)arg);
	CODE_UNREACHABLE;
}
#endif
