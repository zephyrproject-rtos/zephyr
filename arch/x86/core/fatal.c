/* fatal.c - nanokernel fatal error handler */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

/*
 * DESCRIPTION
 * This module provides the _NanoFatalErrorHandler() routine.
 */

#include <toolchain.h>
#include <sections.h>

#include <nanokernel.h>
#include <nano_private.h>
#include <misc/printk.h>
#include <asmPrv.h>

/*
 * Define a default ESF for use with _NanoFatalErrorHandler() in the event
 * the caller does not have a NANO_ESF to pass
 */
const NANO_ESF _default_esf = {
	0xdeaddead, /* CR2 */
#ifdef CONFIG_GDB_INFO
	0xdeaddead, /* EBP */
	0xdeaddead, /* EBX */
	0xdeaddead, /* ESI */
	0xdeaddead, /* EDI */
#endif		    /* CONFIG_GDB_INFO */
	0xdeaddead, /* EDX */
	0xdeaddead, /* ECX */
	0xdeaddead, /* EAX */
	0xdeaddead, /* error code */
	0xdeaddead, /* EIP */
	0xdeaddead, /* CS */
	0xdeaddead, /* EFLAGS */
	0xdeaddead, /* ESP */
	0xdeaddead  /* SS */
};

/**
 *
 * @brief Nanokernel fatal error handler
 *
 * This routine is called when a fatal error condition is detected by either
 * hardware or software.
 *
 * The caller is expected to always provide a usable ESF.  In the event that the
 * fatal error does not have a hardware generated ESF, the caller should either
 * create its own or use a pointer to the global default ESF <_default_esf>.
 *
 * @param reason the reason that the handler was called
 * @param pEsf pointer to the exception stack frame
 *
 * @return This function does not return.
 */
FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
					  const NANO_ESF *pEsf)
{

#ifdef CONFIG_PRINTK

	/* Display diagnostic information about the error */

	switch (reason) {
	case _NANO_ERR_SPURIOUS_INT:
		printk("***** Unhandled exception/interrupt occurred! "
		       "*****\n");
		break;


	case _NANO_ERR_INVALID_TASK_EXIT:
		printk("***** Invalid Exit Software Error! *****\n");
		break;

#if defined(CONFIG_STACK_CANARIES)
	case _NANO_ERR_STACK_CHK_FAIL:
		printk("***** Stack Check Fail! *****\n");
		break;
#endif /* CONFIG_STACK_CANARIES */


	case _NANO_ERR_ALLOCATION_FAIL:
		printk("**** Kernel Allocation Failure! ****\n");
		break;

	default:
		printk("**** Unknown Fatal Error %d! ****\n", reason);
		break;
	}

	printk("Current thread ID = 0x%x\n"
	       "Faulting instruction address = 0x%x\n",
	       sys_thread_self_get(),
	       pEsf->eip);
#endif /* CONFIG_PRINTK */


	/*
	 * Error was fatal to a kernel task or a fiber; invoke the system
	 * fatal error handling policy defined for the platform.
	 */

	_SysFatalErrorHandler(reason, pEsf);
}

#if CONFIG_EXCEPTION_DEBUG

static FUNC_NORETURN void generic_exc_handle(char *description,
					     unsigned int vector,
					     const NANO_ESF *pEsf)
{
	printk("***** CPU exception %d: %s\n", vector, description);
	if ((1 << vector) & _EXC_ERROR_CODE_FAULTS) {
		printk("***** Exception code: 0x%x\n", pEsf->errorCode);
	}
	_NanoFatalErrorHandler(_NANO_ERR_SPURIOUS_INT, pEsf);
}

#define EXC_FUNC(vector, description) \
FUNC_NORETURN void handle_exc_##vector(const NANO_ESF *pEsf) \
{ \
	generic_exc_handle(description, vector, pEsf); \
}

EXC_FUNC(IV_DIVIDE_ERROR, "Division by zero");
EXC_FUNC(IV_NON_MASKABLE_INTERRUPT, "Non-maskable interrupt");
EXC_FUNC(IV_OVERFLOW, "Overflow");
EXC_FUNC(IV_BOUND_RANGE, "Bounds");
EXC_FUNC(IV_INVALID_OPCODE, "Invalid opcode");
#ifndef CONFIG_FP_SHARING
EXC_FUNC(IV_DEVICE_NOT_AVAILABLE, "FPU device not available");
#endif
EXC_FUNC(IV_DOUBLE_FAULT, "Double fault");
EXC_FUNC(IV_INVALID_TSS, "Invalid task state segment");
EXC_FUNC(IV_SEGMENT_NOT_PRESENT, "Segment not present");
EXC_FUNC(IV_STACK_FAULT, "Stack fault");
EXC_FUNC(IV_GENERAL_PROTECTION, "General protection fault");
EXC_FUNC(IV_PAGE_FAULT, "Page fault");
EXC_FUNC(IV_X87_FPU_FP_ERROR, "Floating point error");
EXC_FUNC(IV_ALIGNMENT_CHECK, "Alignment error");
EXC_FUNC(IV_MACHINE_CHECK, "Machine check");

#endif /* CONFIG_EXCEPTION_DEBUG */

