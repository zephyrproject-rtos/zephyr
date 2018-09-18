/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
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
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
#if defined(CONFIG_SOC_RISCV32_PULPINO)
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
#endif
};


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
	LOG_PANIC();

	switch (reason) {
	case _NANO_ERR_CPU_EXCEPTION:
	case _NANO_ERR_SPURIOUS_INT:
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
	LOG_ERR("Faulting instruction address = 0x%x",
	       (esf->mepc == 0xdeadbaad) ? 0xdeadbaad : esf->mepc);
	LOG_ERR("  ra: 0x%x  gp: 0x%x  tp: 0x%x  t0: 0x%x",
	       esf->ra, esf->gp, esf->tp, esf->t0);
	LOG_ERR("  t1: 0x%x  t2: 0x%x  t3: 0x%x  t4: 0x%x",
	       esf->t1, esf->t2, esf->t3, esf->t4);
	LOG_ERR("  t5: 0x%x  t6: 0x%x  a0: 0x%x  a1: 0x%x",
	       esf->t5, esf->t6, esf->a0, esf->a1);
	LOG_ERR("  a2: 0x%x  a3: 0x%x  a4: 0x%x  a5: 0x%x",
	       esf->a2, esf->a3, esf->a4, esf->a5);
	LOG_ERR("  a6: 0x%x  a7: 0x%x",
	       esf->a6, esf->a7);

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

	for (;;) {
		k_cpu_idle();
	}
	CODE_UNREACHABLE;
}

static char *cause_str(u32_t cause)
{
	switch (cause) {
	case 0:
		return "Instruction address misaligned";
	case 1:
		return "Instruction Access fault";
	case 2:
		return "Illegal instruction";
	case 3:
		return "Breakpoint";
	case 4:
		return "Load address misaligned";
	case 5:
		return "Load access fault";
	default:
		return "unknown";
	}
}

FUNC_NORETURN void _Fault(const NANO_ESF *esf)
{
	u32_t mcause;

	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));

	mcause &= SOC_MCAUSE_EXP_MASK;

	LOG_ERR("Exception cause %s (%d)", cause_str(mcause), (int)mcause);

	_NanoFatalErrorHandler(_NANO_ERR_CPU_EXCEPTION, esf);
}
