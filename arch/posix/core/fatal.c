/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*Started as a copy of the nios2 one with some prunning*/

#include <kernel.h>
#include <arch/cpu.h>
#include <kernel_structs.h>
#include <misc/printk.h>
#include <inttypes.h>
#include "posix_soc_if.h"

const NANO_ESF _default_esf = {
	0xdeadbaad
};

/**
 *
 * @brief Kernel fatal error handler
 *
 * This routine is called when a fatal error condition is detected by either
 * hardware or software.
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

#if defined(CONFIG_EXTRA_EXCEPTION_INFO) && defined(CONFIG_PRINTK) \
		&& defined(ALT_CPU_HAS_EXTRA_EXCEPTION_INFO)
static char *cause_str(u32_t cause_code)
{
	switch (cause_code) {
	case 0:
		return "reset";
	case 1:
		return "processor-only reset request";
	case 2:
		return "interrupt";
	case 3:
		return "trap";
	case 4:
		return "unimplemented instruction";
	case 5:
		return "illegal instruction";
	case 6:
		return "misaligned data address";
	case 7:
		return "misaligned destination address";
	case 8:
		return "division error";
	case 9:
		return "supervisor-only instruction address";
	case 10:
		return "supervisor-only instruction";
	case 11:
		return "supervisor-only data address";
	case 12:
		return "TLB miss";
	case 13:
		return "TLB permission violation (execute)";
	case 14:
		return "TLB permission violation (read)";
	case 15:
		return "TLB permission violation (write)";
	case 16:
		return "MPU region violation (instruction)";
	case 17:
		return "MPU region violation (data)";
	case 18:
		return "ECC TLB error";
	case 19:
		return "ECC fetch error (instruction)";
	case 20:
		return "ECC register file error";
	case 21:
		return "ECC data error";
	case 22:
		return "ECC data cache writeback error";
	case 23:
		return "bus instruction fetch error";
	case 24:
		return "bus data region violation";
	default:
		return "unknown";
	}
}
#endif

FUNC_NORETURN void _Fault(const NANO_ESF *esf)
{
#ifdef CONFIG_PRINTK
	/* Unfortunately, completely unavailable on Nios II/e cores */
#ifdef ALT_CPU_HAS_EXTRA_EXCEPTION_INFO
	u32_t exc_reg, badaddr_reg, eccftl;
	enum nios2_exception_cause cause;

	exc_reg = _nios2_creg_read(NIOS2_CR_EXCEPTION);

	/* Bit 31 indicates potentially fatal ECC error */
	eccftl = (exc_reg & NIOS2_EXCEPTION_REG_ECCFTL_MASK) != 0;

	/* Bits 2-6 contain the cause code */
	cause = (exc_reg & NIOS2_EXCEPTION_REG_CAUSE_MASK)
			>> NIOS2_EXCEPTION_REG_CAUSE_OFST;

	printk("Exception cause: %d ECCFTL: 0x%x\n", cause, eccftl);
#if CONFIG_EXTRA_EXCEPTION_INFO
	printk("reason: %s\n", cause_str(cause));
#endif
	if (BIT(cause) & NIOS2_BADADDR_CAUSE_MASK) {
		badaddr_reg = _nios2_creg_read(NIOS2_CR_BADADDR);
		printk("Badaddr: 0x%x\n", badaddr_reg);
	}
#endif /* ALT_CPU_HAS_EXTRA_EXCEPTION_INFO */
#endif /* CONFIG_PRINTK */

	_NanoFatalErrorHandler(_NANO_ERR_CPU_EXCEPTION, esf);
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
		ps_print_error_and_exit(
			"Fatal fault in %s! Stopping...\n",
			k_is_in_isr() ? "ISR" : "essential thread");
	}
	printk("Fatal fault in thread %p! Aborting.\n", _current);
	k_thread_abort(_current);

hang_system:
#else
	ARG_UNUSED(reason);
#endif

	ps_print_error_and_exit(
		"Stopped in _SysFatalErrorHandler()\n");
	CODE_UNREACHABLE;
}
