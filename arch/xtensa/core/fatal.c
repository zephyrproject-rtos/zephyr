/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_arch_data.h>
#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#define PR_EXC(...) printk(__VA_ARGS__)
#else
#define PR_EXC(...)
#endif /* CONFIG_PRINTK */

const NANO_ESF _default_esf = {
	{0xdeaddead}, /* sp */
	0xdeaddead, /* pc */
};

extern void exit(int exit_code);

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
 * @param reason the reason that the handler was called
 * @param pEsf pointer to the exception stack frame
 *
 * @return This function does not return.
 */
void _NanoFatalErrorHandler(unsigned int reason, const NANO_ESF *pEsf)
{
	switch (reason) {
	case _NANO_ERR_INVALID_TASK_EXIT:
		PR_EXC("***** Invalid Exit Software Error! *****\n");
		break;
#if defined(CONFIG_STACK_CANARIES)
	case _NANO_ERR_STACK_CHK_FAIL:
		PR_EXC("***** Stack Check Fail! *****\n");
		break;
#endif /* CONFIG_STACK_CANARIES */
	case _NANO_ERR_ALLOCATION_FAIL:
		PR_EXC("**** Kernel Allocation Failure! ****\n");
		break;
	default:
		PR_EXC("**** Unknown Fatal Error %d! ****\n", reason);
		break;
	}
	PR_EXC("Current thread ID = 0x%x\n"
	       "Faulting instruction address = 0x%x\n",
	       k_current_get(),
	       pEsf->pc);
	/*
	 * Now that the error has been reported, call the user implemented
	 * policy
	 * to respond to the error.  The decisions as to what responses are
	 * appropriate to the various errors are something the customer must
	 * decide.
	 */
	/* TODO: call _SysFatalErrorHandler(reason, pEsf); */
	exit(253);
}

void FatalErrorHandler(void)
{
	unsigned int tmpReg = 0;
	unsigned int Esf[5];

	__asm__ volatile("rsr %0, 177\n\t" : "=r"(tmpReg)); /* epc */
	Esf[0] = tmpReg;
	__asm__ volatile("rsr %0, 232\n\t" : "=r"(tmpReg)); /* exccause */
	Esf[1] = tmpReg;
	__asm__ volatile("rsr %0, 209\n\t" : "=r"(tmpReg)); /* excsave */
	Esf[2] = tmpReg;
	__asm__ volatile("rsr %0, 230\n\t" : "=r"(tmpReg)); /* ps */
	Esf[3] = tmpReg;
	__asm__ volatile("rsr %0, 238\n\t" : "=r"(tmpReg)); /* excvaddr */
	Esf[4] = tmpReg;
	PR_EXC("Error\nEPC      = 0x%x\n"
	       "EXCCAUSE = 0x%x\n"
	       "EXCSAVE  = 0x%x\n"
	       "PS       = 0x%x\n"
	       "EXCVADDR = 0x%x\n",
		   Esf[0], Esf[1], Esf[2], Esf[3], Esf[4]);
	exit(255);
}

void ReservedInterruptHandler(unsigned int intNo)
{
	unsigned int tmpReg = 0;
	unsigned int Esf[5];

	__asm__ volatile("rsr %0, 177\n\t" : "=r"(tmpReg)); /* epc */
	Esf[0] = tmpReg;
	__asm__ volatile("rsr %0, 232\n\t" : "=r"(tmpReg)); /* exccause */
	Esf[1] = tmpReg;
	__asm__ volatile("rsr %0, 209\n\t" : "=r"(tmpReg)); /* excsave */
	Esf[2] = tmpReg;
	__asm__ volatile("rsr %0, 230\n\t" : "=r"(tmpReg)); /* ps */
	Esf[3] = tmpReg;
	__asm__ volatile("rsr %0, 228\n\t" : "=r"(tmpReg)); /* intenable */
	Esf[4] = tmpReg;
	PR_EXC("Error, unhandled interrupt\n"
	       "EPC       = 0x%x\n"
	       "EXCCAUSE  = 0x%x\n"
	       "EXCSAVE   = 0x%x\n"
	       "PS        = 0x%x\n"
	       "INTENABLE = 0x%x\n"
	       "INTERRUPT = 0x%x\n",
	       Esf[0], Esf[1], Esf[2], Esf[3], Esf[4], (1 << intNo));
	exit(254);
}

