/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fatal fault handling
 *
 * This module implements the routines necessary for handling fatal faults on
 * ARCv2 CPUs.
 */

#include <kernel_structs.h>
#include <offsets_short.h>
#include <toolchain.h>
#include <arch/cpu.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>

LOG_MODULE_REGISTER(fatal);

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
 * @return This function does not return.
 */
void _NanoFatalErrorHandler(unsigned int reason, const NANO_ESF *pEsf)
{
	LOG_PANIC();

	switch (reason) {
	case _NANO_ERR_HW_EXCEPTION:
		break;

#if defined(CONFIG_STACK_CANARIES) || defined(CONFIG_ARC_STACK_CHECKING) \
	|| defined(CONFIG_STACK_SENTINEL)
	case _NANO_ERR_STACK_CHK_FAIL:
		LOG_ERR("***** Stack Check Fail! *****");
		break;
#endif

	case _NANO_ERR_ALLOCATION_FAIL:
		LOG_ERR("**** Kernel Allocation Failure! ****");
		break;

	case _NANO_ERR_KERNEL_OOPS:
		LOG_ERR("***** Kernel OOPS! *****");
		break;

	case _NANO_ERR_KERNEL_PANIC:
		LOG_ERR("***** Kernel Panic! *****");
		break;

	default:
		LOG_ERR("**** Unknown Fatal Error %d! ****", reason);
		break;
	}

	LOG_ERR("Current thread ID = %p",  k_current_get());

	if (reason == _NANO_ERR_HW_EXCEPTION) {
		LOG_ERR("Faulting instruction address = 0x%lx",
		_arc_v2_aux_reg_read(_ARC_V2_ERET));
	}

	/*
	 * Now that the error has been reported, call the user implemented
	 * policy
	 * to respond to the error.  The decisions as to what responses are
	 * appropriate to the various errors are something the customer must
	 * decide.
	 */

	_SysFatalErrorHandler(reason, pEsf);
}

FUNC_NORETURN void _arch_syscall_oops(void *ssf_ptr)
{
	LOG_PANIC();

	_SysFatalErrorHandler(_NANO_ERR_KERNEL_OOPS, ssf_ptr);
	CODE_UNREACHABLE;
}
