/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <kernel_structs.h>
#include <inttypes.h>
#include <kernel_arch_data.h>
#include <xtensa/specreg.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>

LOG_MODULE_REGISTER(fatal);

#ifdef XT_SIMULATOR
#include <xtensa/simcall.h>
#endif

const NANO_ESF _default_esf = {
	{0xdeaddead}, /* sp */
	0xdeaddead, /* pc */
};

/* Need to do this as a macro since regnum must be an immediate value */
#define get_sreg(regnum_p) ({ \
	unsigned int retval; \
	__asm__ volatile( \
	    "rsr %[retval], %[regnum]\n\t" \
	    : [retval] "=r" (retval) \
	    : [regnum] "i" (regnum_p)); \
	retval; \
	})

/**
 *
 * @brief Fatal error handler
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
 * @param reason the reason that the handler was called
 * @param pEsf pointer to the exception stack frame
 *
 * @return This function does not return.
 */
XTENSA_ERR_NORET void _NanoFatalErrorHandler(unsigned int reason,
					     const NANO_ESF *pEsf)
{
	LOG_PANIC();

	switch (reason) {
	case _NANO_ERR_HW_EXCEPTION:
	case _NANO_ERR_RESERVED_IRQ:
		break;

#if defined(CONFIG_STACK_CANARIES) || defined(CONFIG_STACK_SENTINEL)
	case _NANO_ERR_STACK_CHK_FAIL:
		LOG_ERR("***** Stack Check Fail! *****");
		break;
#endif /* CONFIG_STACK_CANARIES */
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
	LOG_ERR("Current thread ID = %p", k_current_get());
	LOG_ERR("Faulting instruction address = 0x%x", pEsf->pc);

	/*
	 * Now that the error has been reported, call the user implemented
	 * policy
	 * to respond to the error.  The decisions as to what responses are
	 * appropriate to the various errors are something the customer must
	 * decide.
	 */
	_SysFatalErrorHandler(reason, pEsf);
}

static char *cause_str(unsigned int cause_code)
{
	switch (cause_code) {
	case 0:
		return "illegal instruction";
	case 1:
		return "syscall";
	case 2:
		return "instr fetch error";
	case 3:
		return "load/store error";
	case 4:
		return "level-1 interrupt";
	case 5:
		return "alloca";
	case 6:
		return "divide by zero";
	case 8:
		return "privileged";
	case 9:
		return "load/store alignment";
	case 12:
		return "instr PIF data error";
	case 13:
		return "load/store PIF data error";
	case 14:
		return "instr PIF addr error";
	case 15:
		return "load/store PIF addr error";
	case 16:
		return "instr TLB miss";
	case 17:
		return "instr TLB multi hit";
	case 18:
		return "instr fetch privilege";
	case 20:
		return "inst fetch prohibited";
	case 24:
		return "load/store TLB miss";
	case 25:
		return "load/store TLB multi hit";
	case 26:
		return "load/store privilege";
	case 28:
		return "load prohibited";
	case 29:
		return "store prohibited";
	case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
		return "coprocessor disabled";
	default:
		return "unknown/reserved";
	}
}

static inline unsigned int get_bits(int offset, int num_bits, unsigned int val)
{
	int mask;

	mask = (1 << num_bits) - 1;
	val = val >> offset;
	return val & mask;
}

static void dump_exc_state(void)
{
	unsigned int cause, ps;

	cause = get_sreg(EXCCAUSE);
	ps = get_sreg(PS);

	LOG_ERR("Exception cause %d (%s):"
	       "  EPC1     : 0x%08x EXCSAVE1 : 0x%08x EXCVADDR : 0x%08x",
	       cause, cause_str(cause), get_sreg(EPC_1),
	       get_sreg(EXCSAVE_1), get_sreg(EXCVADDR));

	LOG_ERR("Program state (PS):");
	LOG_ERR("  INTLEVEL : %02d EXCM    : %d UM  : %d RING : %d WOE : %d",
	       get_bits(0, 4, ps), get_bits(4, 1, ps), get_bits(5, 1, ps),
	       get_bits(6, 2, ps), get_bits(18, 1, ps));
#ifndef __XTENSA_CALL0_ABI__
	LOG_ERR("  OWB      : %02d CALLINC : %d",
	       get_bits(8, 4, ps), get_bits(16, 2, ps));
#endif
}


XTENSA_ERR_NORET void FatalErrorHandler(void)
{
	LOG_ERR("*** Unhandled exception ****");
	dump_exc_state();
	_NanoFatalErrorHandler(_NANO_ERR_HW_EXCEPTION, &_default_esf);
}

XTENSA_ERR_NORET void ReservedInterruptHandler(unsigned int intNo)
{
	LOG_ERR("*** Reserved Interrupt ***");
	dump_exc_state();
	LOG_ERR("INTENABLE = 0x%x INTERRUPT = 0x%x (%d)",
	       get_sreg(INTENABLE), (1 << intNo), intNo);
	_NanoFatalErrorHandler(_NANO_ERR_RESERVED_IRQ, &_default_esf);
}

void exit(int return_code)
{
#ifdef XT_SIMULATOR
	__asm__ (
	    "mov a3, %[code]\n\t"
	    "movi a2, %[call]\n\t"
	    "simcall\n\t"
	    :
	    : [code] "r" (return_code), [call] "i" (SYS_exit)
	    : "a3", "a2");
#else
	LOG_ERR("exit(%d)", return_code);
	k_panic();
#endif
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
XTENSA_ERR_NORET __weak void _SysFatalErrorHandler(unsigned int reason,
						   const NANO_ESF *pEsf)
{
	ARG_UNUSED(pEsf);

	LOG_PANIC();

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
		LOG_ERR("Fatal fault in %s! Spinning...",
		       k_is_in_isr() ? "ISR" : "essential thread");
		goto hang_system;
	}
	LOG_ERR("Fatal fault in thread %p! Aborting.", _current);
	k_thread_abort(_current);

hang_system:
#else
	ARG_UNUSED(reason);
#endif

#ifdef XT_SIMULATOR
	exit(255 - reason);
#else
	for (;;) {
		k_cpu_idle();
	}
#endif
	CODE_UNREACHABLE;
}

