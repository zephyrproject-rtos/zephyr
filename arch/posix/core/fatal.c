/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <kernel_structs.h>
#include <misc/printk.h>
#include <inttypes.h>
#include <logging/log_ctrl.h>
#include "posix_soc_if.h"

const NANO_ESF _default_esf = {
	0xdeadbaad
};

/**
 *
 * @brief Kernel fatal error handler
 *
 * This routine is called when a fatal error condition is detected
 *
 * The caller is expected to always provide a usable ESF.  In the event that the
 * fatal error does not have a hardware generated ESF, the caller should either
 * create its own or call _Fault instead.
 *
 * @param reason the reason that the handler was called
 * @param pEsf pointer to the exception stack frame
 *
 * @return This function does not return.
 */
FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
		const NANO_ESF *esf)
{
	LOG_PANIC();

#ifdef CONFIG_PRINTK
	switch (reason) {
	case _NANO_ERR_CPU_EXCEPTION:
	case _NANO_ERR_SPURIOUS_INT:
		break;

	case _NANO_ERR_INVALID_TASK_EXIT:
		printk("***** Invalid Exit Software Error! *****\n");
		break;


	case _NANO_ERR_ALLOCATION_FAIL:
		printk("**** Kernel Allocation Failure! ****\n");
		break;

	case _NANO_ERR_KERNEL_OOPS:
		printk("***** Kernel OOPS! *****\n");
		break;

	case _NANO_ERR_KERNEL_PANIC:
		printk("***** Kernel Panic! *****\n");
		break;

#ifdef CONFIG_STACK_SENTINEL
	case _NANO_ERR_STACK_CHK_FAIL:
		printk("***** Stack overflow *****\n");
		break;
#endif
	default:
		printk("**** Unknown Fatal Error %u! ****\n", reason);
		break;
	}

#endif

	void _SysFatalErrorHandler(unsigned int reason,
			const NANO_ESF *pEsf);
	_SysFatalErrorHandler(reason, esf);
}


/**
 *
 * @brief Fatal error handler
 *
 * This routine implements the corrective action to be taken when the system
 * detects a fatal error.
 *
 * This sample implementation attempts to abort the current thread and allow
 * the system to continue executing, which may permit the system to continue
 * functioning with degraded capabilities.
 *
 * System designers may wish to enhance or substitute this sample
 * implementation to take other actions, such as logging error (or debug)
 * information to a persistent repository and/or rebooting the system.
 *
 * @param reason the fatal error reason
 * @param pEsf pointer to exception stack frame
 *
 * @return N/A
 */
FUNC_NORETURN __weak void _SysFatalErrorHandler(unsigned int reason,
		const NANO_ESF *pEsf)
{
	ARG_UNUSED(pEsf);

#ifdef CONFIG_STACK_SENTINEL
	if (reason == _NANO_ERR_STACK_CHK_FAIL) {
		goto hang_system;
	}
#endif
	if (reason == _NANO_ERR_KERNEL_PANIC) {
		goto hang_system;
	}
	if (k_is_in_isr() || _is_thread_essential()) {
		posix_print_error_and_exit(
			"Fatal fault in %s! Stopping...\n",
			k_is_in_isr() ? "ISR" : "essential thread");
	}
	printk("Fatal fault in thread %p! Aborting.\n", _current);
	k_thread_abort(_current);

hang_system:

	posix_print_error_and_exit(
		"Stopped in _SysFatalErrorHandler()\n");
	CODE_UNREACHABLE;
}
