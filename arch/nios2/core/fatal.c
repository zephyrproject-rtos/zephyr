/*
 * Copyright (c) 2016 Intel Corporation
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

#include <nanokernel.h>
#include <arch/cpu.h>
#include <nano_private.h>
#include <misc/printk.h>

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

	default:
		printk("**** Unknown Fatal Error %d! ****\n", reason);
		break;
	}

	printk("Current thread ID: 0x%x\n"
	       "Faulting instruction: 0x%x\n"
	       " r1: 0x%x  r2: 0x%x  r3: 0x%x  r4: 0x%x\n"
	       " r5: 0x%x  r6: 0x%x  r7: 0x%x  r8: 0x%x\n"
	       " r9: 0x%x r10: 0x%x r11: 0x%x r12: 0x%x\n"
	       "r13: 0x%x r14: 0x%x r15: 0x%x  ra: 0x%x\n"
	       "estatus: %x\n", sys_thread_self_get(), esf->instr,
	       esf->r1, esf->r2, esf->r3, esf->r4,
	       esf->r5, esf->r6, esf->r7, esf->r8,
	       esf->r9, esf->r10, esf->r11, esf->r12,
	       esf->r13, esf->r14, esf->r15, esf->ra,
	       esf->estatus);
#endif

	_SysFatalErrorHandler(reason, esf);
}



FUNC_NORETURN void _Fault(const NANO_ESF *esf)
{
#ifdef CONFIG_PRINTK
	/* Unfortunately, completely unavailable on Nios II/e cores */
#ifdef NIOS2_HAS_EXTRA_EXCEPTION_INFO
	uint32_t exc_reg, badaddr_reg, eccftl;
	enum nios2_exception_cause cause;

	exc_reg = _nios2_creg_read(NIOS2_CR_EXCEPTION);

	/* Bit 31 indicates potentially fatal ECC error */
	eccftl = (exc_reg & NIOS2_EXCEPTION_REG_ECCFTL_MASK) != 0;

	/* Bits 2-6 contain the cause code */
	cause = (exc_reg & NIOS2_EXCEPTION_REG_CAUSE_MASK)
		 >> NIOS2_EXCEPTION_REG_CAUSE_OFST;
	printk("Exception cause: 0x%x ECCFTL: %d\n", exc_reg, eccftl);
	if (BIT(cause) & NIOS2_BADADDR_CAUSE_MASK) {
		badaddr_reg = _nios2_creg_read(NIOS2_CR_BADADDR);
		printk("Badaddr: 0x%x\n", badaddr_reg);
	}
#endif /* NIOS2_HAS_EXTRA_EXCEPTION_INFO */
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
FUNC_NORETURN void _SysFatalErrorHandler(unsigned int reason,
					 const NANO_ESF *pEsf)
{
	nano_context_type_t curCtx = sys_execution_context_type_get();

	ARG_UNUSED(reason);
	ARG_UNUSED(pEsf);

	if ((curCtx != NANO_CTX_ISR) && !_is_thread_essential()) {
#ifdef CONFIG_MICROKERNEL
		if (curCtx == NANO_CTX_TASK) {
			extern FUNC_NORETURN void _TaskAbort(void);
			printk("Fatal task error! Aborting task.\n");
			_TaskAbort();
		} else
#endif /* CONFIG_MICROKERNEL */
		{
			printk("Fatal fiber error! Aborting fiber.\n");
			fiber_abort();
		}
		CODE_UNREACHABLE;
	}

	printk("Fatal fault in %s ! Spinning...\n",
	       curCtx == NANO_CTX_ISR
		       ? "ISR"
		       : curCtx == NANO_CTX_FIBER ? "essential fiber"
						  : "essential task");
#ifdef NIOS2_HAS_DEBUG_STUB
	_nios2_break();
#endif
	for (;;)
		; /* Spin forever */
}

