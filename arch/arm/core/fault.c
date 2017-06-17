/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common fault handler for ARM Cortex-M
 *
 * Common fault handler for ARM Cortex-M processors.
 */

#include <toolchain.h>
#include <linker/sections.h>

#include <kernel.h>
#include <kernel_structs.h>
#include <inttypes.h>

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#define PR_EXC(...) printk(__VA_ARGS__)
#else
#define PR_EXC(...)
#endif /* CONFIG_PRINTK */

#if (CONFIG_FAULT_DUMP > 0)
#define FAULT_DUMP(esf, fault) _FaultDump(esf, fault)
#else
#define FAULT_DUMP(esf, fault) \
	do {                   \
		(void) esf;    \
		(void) fault;  \
	} while ((0))
#endif

#if (CONFIG_FAULT_DUMP == 1)
/**
 *
 * @brief Dump information regarding fault (FAULT_DUMP == 1)
 *
 * Dump information regarding the fault when CONFIG_FAULT_DUMP is set to 1
 * (short form).
 *
 * eg. (precise bus error escalated to hard fault):
 *
 * Fault! EXC #3, Thread: 0x200000dc, instr: 0x000011d3
 * HARD FAULT: Escalation (see below)!
 * MMFSR: 0x00000000, BFSR: 0x00000082, UFSR: 0x00000000
 * BFAR: 0xff001234
 *
 * @return N/A
 */
void _FaultDump(const NANO_ESF *esf, int fault)
{
	PR_EXC("Fault! EXC #%d, Thread: %p, instr @ 0x%x\n",
	       fault,
	       k_current_get(),
	       esf->pc);

#if defined(CONFIG_ARMV6_M)
#elif defined(CONFIG_ARMV7_M)
	int escalation = 0;

	if (3 == fault) { /* hard fault */
		escalation = SCB->HFSR & SCB_HFSR_FORCED_Msk;
		PR_EXC("HARD FAULT: %s\n",
		       escalation ? "Escalation (see below)!"
				  : "Bus fault on vector table read\n");
	}

	PR_EXC("MMFSR: 0x%x, BFSR: 0x%x, UFSR: 0x%x\n",
	       SCB_MMFSR, SCB_BFSR, SCB_MMFSR);

	if (SCB->CFSR & CFSR_MMARVALID_Msk) {
		PR_EXC("MMFAR: 0x%x\n", SCB->MMFAR);
		if (escalation) {
			/* clear MMAR[VALID] to reset */
			SCB->CFSR &= ~CFSR_MMARVALID_Msk;
		}
	}
	if (SCB->CFSR & CFSR_BFARVALID_Msk) {
		PR_EXC("BFAR: 0x%x\n", SCB->BFAR);
		if (escalation) {
			/* clear CFSR_BFAR[VALID] to reset */
			SCB->CFSR &= ~CFSR_BFARVALID_Msk;
		}
	}

	/* clear USFR sticky bits */
	SCB->CFSR |= SCB_CFSR_USGFAULTSR_Msk;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
}
#endif

#if (CONFIG_FAULT_DUMP == 2)
/**
 *
 * @brief Dump thread information
 *
 * See _FaultDump() for example.
 *
 * @return N/A
 */
static void _FaultThreadShow(const NANO_ESF *esf)
{
	PR_EXC("  Executing thread ID (thread): %p\n"
	       "  Faulting instruction address:  0x%x\n",
	       k_current_get(), esf->pc);
}

#if defined(CONFIG_ARMV6_M)
#elif defined(CONFIG_ARMV7_M)

/**
 *
 * @brief Dump MPU fault information
 *
 * See _FaultDump() for example.
 *
 * @return N/A
 */
static void _MpuFault(const NANO_ESF *esf, int fromHardFault)
{
	PR_EXC("***** MPU FAULT *****\n");

	_FaultThreadShow(esf);

	if (SCB->CFSR & CFSR_MSTKERR_Msk) {
		PR_EXC("  Stacking error\n");
	} else if (SCB->CFSR & CFSR_MUNSTKERR_Msk) {
		PR_EXC("  Unstacking error\n");
	} else if (SCB->CFSR & CFSR_DACCVIOL_Msk) {
		PR_EXC("  Data Access Violation\n");
		if (SCB->CFSR & CFSR_MMARVALID_Msk) {
			PR_EXC("  Address: 0x%x\n", (u32_t)SCB->MMFAR);
			if (fromHardFault) {
				/* clear MMAR[VALID] to reset */
				SCB->CFSR &= ~CFSR_MMARVALID_Msk;
			}
		}
	} else if (SCB->CFSR & CFSR_IACCVIOL_Msk) {
		PR_EXC("  Instruction Access Violation\n");
	}
}

/**
 *
 * @brief Dump bus fault information
 *
 * See _FaultDump() for example.
 *
 * @return N/A
 */
static void _BusFault(const NANO_ESF *esf, int fromHardFault)
{
	PR_EXC("***** BUS FAULT *****\n");

	_FaultThreadShow(esf);

	if (SCB->CFSR & CFSR_STKERR_Msk) {
		PR_EXC("  Stacking error\n");
	} else if (SCB->CFSR & CFSR_UNSTKERR_Msk) {
		PR_EXC("  Unstacking error\n");
	} else if (SCB->CFSR & CFSR_PRECISERR_Msk) {
		PR_EXC("  Precise data bus error\n");
		if (SCB->CFSR & CFSR_BFARVALID_Msk) {
			PR_EXC("  Address: 0x%x\n", (u32_t)SCB->BFAR);
			if (fromHardFault) {
				/* clear CFSR_BFAR[VALID] to reset */
				SCB->CFSR &= ~CFSR_BFARVALID_Msk;
			}
		}
		/* it's possible to have both a precise and imprecise fault */
		if (SCB->CFSR & CFSR_IMPRECISERR_Msk) {
			PR_EXC("  Imprecise data bus error\n");
		}
	} else if (SCB->CFSR & CFSR_IMPRECISERR_Msk) {
		PR_EXC("  Imprecise data bus error\n");
	} else if (SCB->CFSR & CFSR_IBUSERR_Msk) {
		PR_EXC("  Instruction bus error\n");
	}
}

/**
 *
 * @brief Dump usage fault information
 *
 * See _FaultDump() for example.
 *
 * @return N/A
 */
static void _UsageFault(const NANO_ESF *esf)
{
	PR_EXC("***** USAGE FAULT *****\n");

	_FaultThreadShow(esf);

	/* bits are sticky: they stack and must be reset */
	if (SCB->CFSR & CFSR_DIVBYZERO_Msk) {
		PR_EXC("  Division by zero\n");
	}
	if (SCB->CFSR & CFSR_UNALIGNED_Msk) {
		PR_EXC("  Unaligned memory access\n");
	}
	if (SCB->CFSR & CFSR_NOCP_Msk) {
		PR_EXC("  No coprocessor instructions\n");
	}
	if (SCB->CFSR & CFSR_INVPC_Msk) {
		PR_EXC("  Illegal load of EXC_RETURN into PC\n");
	}
	if (SCB->CFSR & CFSR_INVSTATE_Msk) {
		PR_EXC("  Illegal use of the EPSR\n");
	}
	if (SCB->CFSR & CFSR_UNDEFINSTR_Msk) {
		PR_EXC("  Attempt to execute undefined instruction\n");
	}

	/* clear USFR sticky bits */
	SCB->CFSR |= SCB_CFSR_USGFAULTSR_Msk;
}

/**
 *
 * @brief Dump debug monitor exception information
 *
 * See _FaultDump() for example.
 *
 * @return N/A
 */
static void _DebugMonitor(const NANO_ESF *esf)
{
	ARG_UNUSED(esf);

	PR_EXC("***** Debug monitor exception (not implemented) *****\n");
}

#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */

/**
 *
 * @brief Dump hard fault information
 *
 * See _FaultDump() for example.
 *
 * @return N/A
 */
static void _HardFault(const NANO_ESF *esf)
{
	PR_EXC("***** HARD FAULT *****\n");

#if defined(CONFIG_ARMV6_M)
	_FaultThreadShow(esf);
#elif defined(CONFIG_ARMV7_M)
	if (SCB->HFSR & SCB_HFSR_VECTTBL_Msk) {
		PR_EXC("  Bus fault on vector table read\n");
	} else if (SCB->HFSR & SCB_HFSR_FORCED_Msk) {
		PR_EXC("  Fault escalation (see below)\n");
		if (SCB_MMFSR) {
			_MpuFault(esf, 1);
		} else if (SCB_BFSR) {
			_BusFault(esf, 1);
		} else if (SCB_UFSR) {
			_UsageFault(esf);
		}
	}
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
}

/**
 *
 * @brief Dump reserved exception information
 *
 * See _FaultDump() for example.
 *
 * @return N/A
 */
static void _ReservedException(const NANO_ESF *esf, int fault)
{
	ARG_UNUSED(esf);

	PR_EXC("***** %s %d) *****\n",
	       fault < 16 ? "Reserved Exception (" : "Spurious interrupt (IRQ ",
	       fault - 16);
}

/**
 *
 * @brief Dump information regarding fault (FAULT_DUMP == 2)
 *
 * Dump information regarding the fault when CONFIG_FAULT_DUMP is set to 2
 * (long form).
 *
 * eg. (precise bus error escalated to hard fault):
 *
 * Executing thread ID (thread): 0x200000dc
 * Faulting instruction address:  0x000011d3
 * ***** HARD FAULT *****
 *    Fault escalation (see below)
 * ***** BUS FAULT *****
 *   Precise data bus error
 *   Address: 0xff001234
 *
 * @return N/A
 */
static void _FaultDump(const NANO_ESF *esf, int fault)
{
	switch (fault) {
	case 3:
		_HardFault(esf);
		break;
#if defined(CONFIG_ARMV6_M)
#elif defined(CONFIG_ARMV7_M)
	case 4:
		_MpuFault(esf, 0);
		break;
	case 5:
		_BusFault(esf, 0);
		break;
	case 6:
		_UsageFault(esf);
		break;
	case 12:
		_DebugMonitor(esf);
		break;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
	default:
		_ReservedException(esf, fault);
		break;
	}
}
#endif /* FAULT_DUMP == 2 */

/**
 *
 * @brief Fault handler
 *
 * This routine is called when fatal error conditions are detected by hardware
 * and is responsible only for reporting the error. Once reported, it then
 * invokes the user provided routine _SysFatalErrorHandler() which is
 * responsible for implementing the error handling policy.
 *
 * Since the ESF can be either on the MSP or PSP depending if an exception or
 * interrupt was already being handled, it is passed a pointer to both and has
 * to find out on which the ESP is present.
 *
 * @param esf ESF on the stack, either MSP or PSP depending at what processor
 *            state the exception was taken.
 */
void _Fault(const NANO_ESF *esf)
{
	int fault = SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk;

	FAULT_DUMP(esf, fault);

	_SysFatalErrorHandler(_NANO_ERR_HW_EXCEPTION, esf);
}

/**
 *
 * @brief Initialization of fault handling
 *
 * Turns on the desired hardware faults.
 *
 * @return N/A
 */
void _FaultInit(void)
{
#if defined(CONFIG_ARMV6_M)
#elif defined(CONFIG_ARMV7_M)
	SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
}
