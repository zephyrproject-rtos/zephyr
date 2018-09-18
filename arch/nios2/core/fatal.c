/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <kernel_structs.h>
#include <inttypes.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>

LOG_MODULE_REGISTER(fatal);

const NANO_ESF _default_esf = {
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
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
	LOG_PANIC();

	switch (reason) {
	case _NANO_ERR_CPU_EXCEPTION:
	case _NANO_ERR_SPURIOUS_INT:
		break;

	case _NANO_ERR_ALLOCATION_FAIL:
		LOG_ERR("**** Kernel Allocation Failure! ****");
		break;

	case _NANO_ERR_KERNEL_OOPS:
		LOG_ERR("***** Kernel OOPS! *****");
		break;

	case _NANO_ERR_KERNEL_PANIC:
		LOG_ERR("***** Kernel Panic! *****");
		break;

#ifdef CONFIG_STACK_SENTINEL
	case _NANO_ERR_STACK_CHK_FAIL:
		LOG_ERR("***** Stack overflow *****");
		break;
#endif
	default:
		LOG_ERR("**** Unknown Fatal Error %u! ****", reason);
		break;
	}

	/* Subtract 4 from EA since we added 4 earlier so that the faulting
	 * instruction isn't retried.
	 *
	 * TODO: Only caller-saved registers get saved upon exception entry.
	 * We may want to introduce a config option to save and dump all
	 * registers, at the expense of some stack space.
	 */
	LOG_ERR("Current thread ID: %p",k_current_get());
	LOG_ERR("Faulting instruction: 0x%x", esf->instr - 4);
	LOG_ERR("  r1: 0x%x  r2: 0x%x  r3: 0x%x  r4: 0x%x",
			esf->r1, esf->r2, esf->r3, esf->r4);
	LOG_ERR("  r5: 0x%x  r6: 0x%x  r7: 0x%x  r8: 0x%x",
			esf->r5, esf->r6, esf->r7, esf->r8);
	LOG_ERR("  r9: 0x%x r10: 0x%x r11: 0x%x r12: 0x%x",
			esf->r9, esf->r10, esf->r11, esf->r12);
	LOG_ERR(" r13: 0x%x r14: 0x%x r15: 0x%x  ra: 0x%x",
			esf->r13, esf->r14, esf->r15, esf->ra);
	LOG_ERR("estatus: %x", esf->estatus);

	_SysFatalErrorHandler(reason, esf);
}

#if defined(CONFIG_EXTRA_EXCEPTION_INFO) \
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

	LOG_ERR("Exception cause: %d ECCFTL: 0x%x", cause, eccftl);
#if CONFIG_EXTRA_EXCEPTION_INFO
	LOG_ERR("reason: %s", cause_str(cause));
#endif
	if (BIT(cause) & NIOS2_BADADDR_CAUSE_MASK) {
		badaddr_reg = _nios2_creg_read(NIOS2_CR_BADADDR);
		LOG_ERR("Badaddr: 0x%x", badaddr_reg);
	}
#endif /* ALT_CPU_HAS_EXTRA_EXCEPTION_INFO */

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

#ifdef ALT_CPU_HAS_DEBUG_STUB
	_nios2_break();
#endif
	for (;;) {
		k_cpu_idle();
	}
	CODE_UNREACHABLE;
}
