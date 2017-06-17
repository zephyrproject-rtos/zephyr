/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel fatal error handler for ARM Cortex-M
 *
 * This module provides the _NanoFatalErrorHandler() routine for ARM Cortex-M.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <inttypes.h>

#include <kernel.h>
#include <kernel_structs.h>
#include <misc/printk.h>


/**
 *
 * @brief Kernel fatal error handler
 *
 * This routine is called when fatal error conditions are detected by software
 * and is responsible only for reporting the error. Once reported, it then
 * invokes the user provided routine _SysFatalErrorHandler() which is
 * responsible for implementing the error handling policy.
 *
 * The caller is expected to always provide a usable ESF. In the event that the
 * fatal error does not have a hardware generated ESF, the caller should either
 * create its own or use a pointer to the global default ESF <_default_esf>.
 *
 * Unlike other arches, this function may return if _SysFatalErrorHandler
 * determines that only the current thread should be aborted and the CPU
 * was in handler mode. PendSV will be asserted in this case and the current
 * thread taken off the run queue. Leaving the exception will immediately
 * trigger a context switch.
 *
 * @param reason the reason that the handler was called
 * @param pEsf pointer to the exception stack frame
 */
void _NanoFatalErrorHandler(unsigned int reason,
					  const NANO_ESF *pEsf)
{
	switch (reason) {
	case _NANO_ERR_INVALID_TASK_EXIT:
		printk("***** Invalid Exit Software Error! *****\n");
		break;

#if defined(CONFIG_STACK_CANARIES) || defined(CONFIG_STACK_SENTINEL)
	case _NANO_ERR_STACK_CHK_FAIL:
		printk("***** Stack Check Fail! *****\n");
		break;
#endif /* CONFIG_STACK_CANARIES */

	case _NANO_ERR_ALLOCATION_FAIL:
		printk("**** Kernel Allocation Failure! ****\n");
		break;

	case _NANO_ERR_KERNEL_OOPS:
		printk("***** Kernel OOPS! *****\n");
		break;

	case _NANO_ERR_KERNEL_PANIC:
		printk("***** Kernel Panic! *****\n");
		break;

	default:
		printk("**** Unknown Fatal Error %d! ****\n", reason);
		break;
	}
	printk("Current thread ID = %p\n"
	       "Faulting instruction address = 0x%x\n",
	       k_current_get(), pEsf->pc);

	/*
	 * Now that the error has been reported, call the user implemented
	 * policy
	 * to respond to the error.  The decisions as to what responses are
	 * appropriate to the various errors are something the customer must
	 * decide.
	 */

	_SysFatalErrorHandler(reason, pEsf);
}

void _do_kernel_oops(const NANO_ESF *esf)
{
	_NanoFatalErrorHandler(esf->r0, esf);
}
