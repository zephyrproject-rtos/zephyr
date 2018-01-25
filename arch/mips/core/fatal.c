/*
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <inttypes.h>
#include <misc/printk.h>

const NANO_ESF _default_esf;

/**
 *
 * @brief Fatal error handler
 *
 * This routine is called when a fatal error condition is detected by either
 * hardware or software.
 *
 * The caller is expected to always provide a usable ESF.  In the event that the
 * fatal error does not have a hardware generated ESF, the caller should either
 * create its own or call _Fault instead.
 *
 * @param reason the reason that the handler was called
 * @param esf pointer to the exception stack frame
 *
 * @return This function does not return.
 */
FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
					  const NANO_ESF *esf)
{
	switch (reason) {
	case _NANO_ERR_CPU_EXCEPTION:
	case _NANO_ERR_SPURIOUS_INT:
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
		"EPC = 0x%x\n status: 0x%x  cause: 0x%x  r0: 0x%x  r1: 0x%x\n"
		"  r2: 0x%x  r3: 0x%x  r4: 0x%x  r5: 0x%x\n"
		"  r6: 0x%x  r7: 0x%x\n",
		k_current_get(), esf->epc,
		esf->status, esf->cause, esf->r[0], esf->r[1], esf->r[2],
		esf->r[3], esf->r[4], esf->r[5], esf->r[6], esf->r[7]);

	_SysFatalErrorHandler(reason, esf);
	/* spin forever */
	for (;;)
		__asm__ volatile("nop");
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
 * @param reason fatal error reason
 * @param esf pointer to exception stack frame
 *
 * @return N/A
 */
FUNC_NORETURN __weak void _SysFatalErrorHandler(unsigned int reason,
						const NANO_ESF *esf)
{
	ARG_UNUSED(esf);

#if !defined(CONFIG_SIMPLE_FATAL_ERROR_HANDLER)
#ifdef CONFIG_STACK_SENTINEL
	if (reason == _NANO_ERR_STACK_CHK_FAIL) {
		goto hang_system;
	}
#endif
	if (reason == _NANO_ERR_KERNEL_PANIC) {
		goto hang_system;
	}
	if (k_is_in_isr() || _is_thread_essential()) {
		printk("Fatal fault in %s! Spinning...\n",
			   k_is_in_isr() ? "ISR" : "essential thread");
		goto hang_system;
	}
	printk("Fatal fault in thread %p! Aborting.\n", _current);
	k_thread_abort(_current);

hang_system:
#else
	ARG_UNUSED(reason);
#endif

	for (;;) {
		k_cpu_idle();
	}
	CODE_UNREACHABLE;
}


#ifdef CONFIG_PRINTK
static char *cause_str(u32_t cause)
{
	switch (cause) {
	case 4:
		return "Address error on load";
	case 5:
		return "Address error on store";
	case 6:
		return "Bus error on instruction fetch";
	case 7:
		return "Bus error on load/store";
	case 9:
		return "Breakpoint";
	case 10:
		return "Reserved instruction";
	default:
		return "unknown";
	}
}
#endif


FUNC_NORETURN void _Fault(const NANO_ESF *esf)
{
#ifdef CONFIG_PRINTK
	u32_t cause;

	cause = mips32_getcr() & SOC_CAUSE_EXP_MASK;

	printk("Exception cause %s (%d)\n", cause_str(cause), (int)cause);
#endif

	_NanoFatalErrorHandler(_NANO_ERR_CPU_EXCEPTION, esf);
}
