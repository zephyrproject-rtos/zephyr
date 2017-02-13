/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <kernel_structs.h>
#include <inttypes.h>

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#define PRINTK(...) printk(__VA_ARGS__)
#else
#define PRINTK(...)
#endif

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
 * @brief Nanokernel fatal error handler
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

	case _NANO_ERR_INVALID_TASK_EXIT:
		PRINTK("***** Invalid Exit Software Error! *****\n");
		break;

#if defined(CONFIG_STACK_CANARIES)
	case _NANO_ERR_STACK_CHK_FAIL:
		PRINTK("***** Stack Check Fail! *****\n");
		break;
#endif /* CONFIG_STACK_CANARIES */

	case _NANO_ERR_ALLOCATION_FAIL:
		PRINTK("**** Kernel Allocation Failure! ****\n");
		break;

	default:
		PRINTK("**** Unknown Fatal Error %d! ****\n", reason);
		break;
	}

	PRINTK("Current thread ID = %p\n"
	       "Faulting instruction address = 0x%" PRIx32 "\n"
	       "  ra: 0x%" PRIx32 "  gp: 0x%" PRIx32
	       "  tp: 0x%" PRIx32 "  t0: 0x%" PRIx32 "\n"
	       "  t1: 0x%" PRIx32 "  t2: 0x%" PRIx32
	       "  t3: 0x%" PRIx32 "  t4: 0x%" PRIx32 "\n"
	       "  t5: 0x%" PRIx32 "  t6: 0x%" PRIx32
	       "  a0: 0x%" PRIx32 "  a1: 0x%" PRIx32 "\n"
	       "  a2: 0x%" PRIx32 "  a3: 0x%" PRIx32
	       "  a4: 0x%" PRIx32 "  a5: 0x%" PRIx32 "\n"
	       "  a6: 0x%" PRIx32 "  a7: 0x%" PRIx32 "\n",
	       k_current_get(),
	       (esf->mepc == 0xdeadbaad) ? 0xdeadbaad : esf->mepc - 4,
	       esf->ra, esf->gp, esf->tp, esf->t0,
	       esf->t1, esf->t2, esf->t3, esf->t4,
	       esf->t5, esf->t6, esf->a0, esf->a1,
	       esf->a2, esf->a3, esf->a4, esf->a5,
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
void _SysFatalErrorHandler(unsigned int reason, const NANO_ESF *esf)
{
	ARG_UNUSED(reason);
	ARG_UNUSED(esf);

#if !defined(CONFIG_SIMPLE_FATAL_ERROR_HANDLER)
	if (k_is_in_isr() || _is_thread_essential()) {
		PRINTK("Fatal fault in %s! Spinning...\n",
		       k_is_in_isr() ? "ISR" : "essential thread");
		/* spin forever */
		for (;;)
			__asm__ volatile("nop");
	}
	PRINTK("Fatal fault in thread! Aborting.\n");
	k_thread_abort(_current);

#else
	for (;;) {
		k_cpu_idle();
	}

#endif

	CODE_UNREACHABLE;
}


#ifdef CONFIG_PRINTK
static char *cause_str(uint32_t cause)
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
#endif


FUNC_NORETURN void _Fault(const NANO_ESF *esf)
{
	uint32_t mcause;

	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));

	mcause &= SOC_MCAUSE_EXP_MASK;

	PRINTK("Exception cause %s (%d)\n", cause_str(mcause), (int)mcause);

	_NanoFatalErrorHandler(_NANO_ERR_CPU_EXCEPTION, esf);
}
