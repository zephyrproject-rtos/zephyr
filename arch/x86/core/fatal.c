/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel fatal error handler
 *
 * This module provides the _NanoFatalErrorHandler() routine.
 */

#include <toolchain.h>
#include <linker/sections.h>

#include <kernel.h>
#include <kernel_structs.h>
#include <misc/printk.h>
#include <arch/x86/irq_controller.h>
#include <arch/x86/segmentation.h>
#include <exception.h>
#include <inttypes.h>

__weak void _debug_fatal_hook(const NANO_ESF *esf) { ARG_UNUSED(esf); }


/**
 *
 * @brief Kernel fatal error handler
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
	_debug_fatal_hook(pEsf);

#ifdef CONFIG_PRINTK

	/* Display diagnostic information about the error */

	switch (reason) {
	case _NANO_ERR_CPU_EXCEPTION:
		break;

	case _NANO_ERR_SPURIOUS_INT: {
		int vector = _irq_controller_isr_vector_get();

		printk("***** Unhandled interrupt vector ");
		if (vector >= 0) {
			printk("%d ", vector);
		}
		printk("*****\n");
		break;
	}
	case _NANO_ERR_INVALID_TASK_EXIT:
		printk("***** Invalid Exit Software Error! *****\n");
		break;

#if defined(CONFIG_STACK_CANARIES) || defined(CONFIG_STACK_SENTINEL)
	case _NANO_ERR_STACK_CHK_FAIL:
		printk("***** Stack Check Fail! *****\n");
		break;
#endif /* CONFIG_STACK_CANARIES */

	case _NANO_ERR_KERNEL_OOPS:
		printk("***** Kernel OOPS! *****\n");
		break;

	case _NANO_ERR_KERNEL_PANIC:
		printk("***** Kernel Panic! *****\n");
		break;

	case _NANO_ERR_ALLOCATION_FAIL:
		printk("**** Kernel Allocation Failure! ****\n");
		break;

	default:
		printk("**** Unknown Fatal Error %d! ****\n", reason);
		break;
	}

	printk("Current thread ID = %p\n"
	       "Faulting segment:address = 0x%04x:0x%08x\n"
	       "eax: 0x%08x, ebx: 0x%08x, ecx: 0x%08x, edx: 0x%08x\n"
	       "esi: 0x%08x, edi: 0x%08x, ebp: 0x%08x, esp: 0x%08x\n"
	       "eflags: 0x%x\n",
	       k_current_get(),
	       pEsf->cs & 0xFFFF, pEsf->eip,
	       pEsf->eax, pEsf->ebx, pEsf->ecx, pEsf->edx,
	       pEsf->esi, pEsf->edi, pEsf->ebp, pEsf->esp,
	       pEsf->eflags);
#endif /* CONFIG_PRINTK */


	/*
	 * Error was fatal to a kernel task or a fiber; invoke the system
	 * fatal error handling policy defined for the platform.
	 */

	_SysFatalErrorHandler(reason, pEsf);
}

#ifdef CONFIG_X86_KERNEL_OOPS
/* The reason code gets pushed onto the stack right before the exception is
 * triggered, so it would be after the nano_esf data
 */
struct oops_esf {
	NANO_ESF nano_esf;
	unsigned int reason;
};

FUNC_NORETURN void _do_kernel_oops(const struct oops_esf *esf)
{
	_NanoFatalErrorHandler(esf->reason, &esf->nano_esf);
}

extern void (*_kernel_oops_handler)(void);
NANO_CPU_INT_REGISTER(_kernel_oops_handler, NANO_SOFT_IRQ,
		      CONFIG_X86_KERNEL_OOPS_VECTOR / 16,
		      CONFIG_X86_KERNEL_OOPS_VECTOR, 0);
#else
/*
 * Define a default ESF for use with _NanoFatalErrorHandler() in the event
 * the caller does not have a NANO_ESF to pass
 */
const NANO_ESF _default_esf = {
	0xdeaddead, /* ESP */
	0xdeaddead, /* EBP */
	0xdeaddead, /* EBX */
	0xdeaddead, /* ESI */
	0xdeaddead, /* EDI */
	0xdeaddead, /* EDX */
	0xdeaddead, /* ECX */
	0xdeaddead, /* EAX */
	0xdeaddead, /* error code */
	0xdeaddead, /* EIP */
	0xdeaddead, /* CS */
	0xdeaddead, /* EFLAGS */
};
#endif /* CONFIG_X86_KERNEL_OOPS */

#if CONFIG_EXCEPTION_DEBUG

static FUNC_NORETURN void generic_exc_handle(unsigned int vector,
					     const NANO_ESF *pEsf)
{
	printk("***** CPU exception %d\n", vector);
	if ((1 << vector) & _EXC_ERROR_CODE_FAULTS) {
		printk("***** Exception code: 0x%x\n", pEsf->errorCode);
	}
	_NanoFatalErrorHandler(_NANO_ERR_CPU_EXCEPTION, pEsf);
}

#define _EXC_FUNC(vector) \
FUNC_NORETURN void handle_exc_##vector(const NANO_ESF *pEsf) \
{ \
	generic_exc_handle(vector, pEsf); \
}

#define _EXC_FUNC_CODE(vector) \
	_EXC_FUNC(vector) \
	_EXCEPTION_CONNECT_CODE(handle_exc_##vector, vector)

#define _EXC_FUNC_NOCODE(vector) \
	_EXC_FUNC(vector) \
	_EXCEPTION_CONNECT_NOCODE(handle_exc_##vector, vector)

/* Necessary indirection to ensure 'vector' is expanded before we expand
 * the handle_exc_##vector
 */
#define EXC_FUNC_NOCODE(vector) \
	_EXC_FUNC_NOCODE(vector)

#define EXC_FUNC_CODE(vector) \
	_EXC_FUNC_CODE(vector)

EXC_FUNC_NOCODE(IV_DIVIDE_ERROR);
EXC_FUNC_NOCODE(IV_NON_MASKABLE_INTERRUPT);
EXC_FUNC_NOCODE(IV_OVERFLOW);
EXC_FUNC_NOCODE(IV_BOUND_RANGE);
EXC_FUNC_NOCODE(IV_INVALID_OPCODE);
EXC_FUNC_NOCODE(IV_DEVICE_NOT_AVAILABLE);
EXC_FUNC_CODE(IV_DOUBLE_FAULT);
EXC_FUNC_CODE(IV_INVALID_TSS);
EXC_FUNC_CODE(IV_SEGMENT_NOT_PRESENT);
EXC_FUNC_CODE(IV_STACK_FAULT);
EXC_FUNC_CODE(IV_GENERAL_PROTECTION);
EXC_FUNC_NOCODE(IV_X87_FPU_FP_ERROR);
EXC_FUNC_CODE(IV_ALIGNMENT_CHECK);
EXC_FUNC_NOCODE(IV_MACHINE_CHECK);

/* Page fault error code flags */
#define PRESENT	BIT(0)
#define WR	BIT(1)
#define US	BIT(2)
#define RSVD	BIT(3)
#define ID	BIT(4)
#define PK	BIT(5)
#define SGX	BIT(15)

FUNC_NORETURN void page_fault_handler(const NANO_ESF *pEsf)
{
	u32_t err, cr2;

	/* See Section 6.15 of the IA32 Software Developer's Manual vol 3 */
	__asm__ ("mov %%cr2, %0" : "=r" (cr2));

	err = pEsf->errorCode;
	printk("***** CPU Page Fault (error code 0x%08x)\n", err);

	printk("%s thread %s address 0x%08x\n",
	       err & US ? "User" : "Supervisor",
	       err & ID ? "executed" : (err & WR ? "wrote" : "read"),
	       cr2);

	_NanoFatalErrorHandler(_NANO_ERR_CPU_EXCEPTION, pEsf);
}
_EXCEPTION_CONNECT_CODE(page_fault_handler, IV_PAGE_FAULT);
#endif /* CONFIG_EXCEPTION_DEBUG */

