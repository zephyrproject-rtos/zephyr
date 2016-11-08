/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#define PR_EXC(...) printk(__VA_ARGS__)
#else
#define PR_EXC(...)
#endif /* CONFIG_PRINTK */

const NANO_ESF _default_esf = {
	0xdeaddead, /* placeholder */
};

/**
 *
 * @brief Nanokernel fatal error handler
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
FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
							const NANO_ESF *pEsf)
{
	switch (reason) {
	case _NANO_ERR_INVALID_TASK_EXIT:
		PR_EXC("***** Invalid Exit Software Error! *****\n");
		break;

#if defined(CONFIG_STACK_CANARIES)
	case _NANO_ERR_STACK_CHK_FAIL:
		PR_EXC("***** Stack Check Fail! *****\n");
		break;
#endif

	case _NANO_ERR_ALLOCATION_FAIL:
		PR_EXC("**** Kernel Allocation Failure! ****\n");
		break;

	default:
		PR_EXC("**** Unknown Fatal Error %d! ****\n", reason);
		break;
	}
	PR_EXC("Current thread ID = %p\n"
	       "Faulting instruction address = 0x%lx\n",
	       k_current_get(),
	       _arc_v2_aux_reg_read(_ARC_V2_ERET));

	/*
	 * Now that the error has been reported, call the user implemented
	 * policy
	 * to respond to the error.  The decisions as to what responses are
	 * appropriate to the various errors are something the customer must
	 * decide.
	 */

	_SysFatalErrorHandler(reason, pEsf);

	for (;;)
		;
}
