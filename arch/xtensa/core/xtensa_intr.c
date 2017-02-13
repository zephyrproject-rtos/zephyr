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

#if XCHAL_HAVE_EXCEPTIONS

/* Handler table is in xtensa_intr_asm.S */

extern xt_exc_handler _xt_exception_table[XCHAL_EXCCAUSE_NUM];


/*
 * Default handler for unhandled exceptions.
 */
void xt_unhandled_exception(XtExcFrame *frame)
{
	FatalErrorHandler();
	CODE_UNREACHABLE;
}


/*
 * This function registers a handler for the specified exception.
 * The function returns the address of the previous handler.
 * On error, it returns 0.
 */
xt_exc_handler _xt_set_exception_handler(int n, xt_exc_handler f)
{
	xt_exc_handler old;

	if (n < 0 || n >= XCHAL_EXCCAUSE_NUM)
		return 0;       /* invalid exception number */

	old = _xt_exception_table[n];

	if (f) {
		_xt_exception_table[n] = f;
	} else {
		_xt_exception_table[n] = &xt_unhandled_exception;
	}

	return ((old == &xt_unhandled_exception) ? 0 : old);
}

#endif

#if XCHAL_HAVE_INTERRUPTS
/*
 * Default handler for unhandled interrupts.
 */
void xt_unhandled_interrupt(void *arg)
{
	ReservedInterruptHandler((unsigned int)arg);
	CODE_UNREACHABLE;
}
#endif /* XCHAL_HAVE_INTERRUPTS */
