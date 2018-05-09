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
#define STORE_xFAR(reg_var, reg) u32_t reg_var = (u32_t)reg
#else
#define PR_EXC(...)
#define STORE_xFAR(reg_var, reg)
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

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	int escalation = 0;

	if (3 == fault) { /* hard fault */
		escalation = SCB->HFSR & SCB_HFSR_FORCED_Msk;
		PR_EXC("HARD FAULT: %s\n",
		       escalation ? "Escalation (see below)!"
				  : "Bus fault on vector table read\n");
	}

	PR_EXC("MMFSR: 0x%x, BFSR: 0x%x, UFSR: 0x%x\n",
	       SCB_MMFSR, SCB_BFSR, SCB_MMFSR);
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	PR_EXC("SFSR: 0x%x\n", SAU->SFSR);
#endif /* CONFIG_ARM_SECURE_FIRMWARE */

	/* In a fault handler, to determine the true faulting address:
	 * 1. Read and save the MMFAR or BFAR value.
	 * 2. Read the MMARVALID bit in the MMFSR, or the BFARVALID bit in the
	 * BFSR. The MMFAR or BFAR address is valid only if this bit is 1.
	 *
	 * Software must follow this sequence because another higher priority
	 * exception might change the MMFAR or BFAR value.
	 */
	STORE_xFAR(mmfar, SCB->MMFAR);
	STORE_xFAR(bfar, SCB->BFAR);
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	STORE_xFAR(sfar, SAU->SFAR);
#endif /* CONFIG_ARM_SECURE_FIRMWARE */

	if (SCB->CFSR & CFSR_MMARVALID_Msk) {
		PR_EXC("MMFAR: 0x%x\n", mmfar);
		if (escalation) {
			/* clear MMAR[VALID] to reset */
			SCB->CFSR &= ~CFSR_MMARVALID_Msk;
		}
	}
	if (SCB->CFSR & CFSR_BFARVALID_Msk) {
		PR_EXC("BFAR: 0x%x\n", bfar);
		if (escalation) {
			/* clear CFSR_BFAR[VALID] to reset */
			SCB->CFSR &= ~CFSR_BFARVALID_Msk;
		}
	}
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	if (SAU->SFSR & SAU_SFSR_SFARVALID_Msk) {
		PR_EXC("SFAR: 0x%x\n", sfar);
		if (escalation) {
			/* clear SFSR_SFAR[VALID] to reset */
			SAU->SFSR &= ~SAU_SFSR_SFARVALID_Msk;
		}
	}

	/* clear SFSR sticky bits */
	SAU->SFSR |= 0xFF;
#endif /* CONFIG_ARM_SECURE_FIRMWARE */

	/* clear USFR sticky bits */
	SCB->CFSR |= SCB_CFSR_USGFAULTSR_Msk;
#if defined(CONFIG_ARMV8_M_MAINLINE)
	/* clear BSFR sticky bits */
	SCB->CFSR |= SCB_CFSR_BUSFAULTSR_Msk;
#endif /* CONFIG_ARMV8_M_MAINLINE */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
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

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
/* HardFault is used for all fault conditions on ARMv6-M. */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)

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

	if (SCB->CFSR & SCB_CFSR_MSTKERR_Msk) {
		PR_EXC("  Stacking error\n");
	} else if (SCB->CFSR & SCB_CFSR_MUNSTKERR_Msk) {
		PR_EXC("  Unstacking error\n");
	} else if (SCB->CFSR & SCB_CFSR_DACCVIOL_Msk) {
		PR_EXC("  Data Access Violation\n");
		/* In a fault handler, to determine the true faulting address:
		 * 1. Read and save the MMFAR value.
		 * 2. Read the MMARVALID bit in the MMFSR.
		 * The MMFAR address is valid only if this bit is 1.
		 *
		 * Software must follow this sequence because another higher
		 * priority exception might change the MMFAR value.
		 */
		STORE_xFAR(mmfar, SCB->MMFAR);

		if (SCB->CFSR & SCB_CFSR_MMARVALID_Msk) {
			PR_EXC("  Address: 0x%x\n", mmfar);
			if (fromHardFault) {
				/* clear SCB_MMAR[VALID] to reset */
				SCB->CFSR &= ~SCB_CFSR_MMARVALID_Msk;
			}
		}
	} else if (SCB->CFSR & SCB_CFSR_IACCVIOL_Msk) {
		PR_EXC("  Instruction Access Violation\n");
#if !defined(CONFIG_ARMV7_M_ARMV8_M_FP)
	}
#else
	} else if (SCB->CFSR & SCB_CFSR_MLSPERR_Msk) {
		PR_EXC("  Floating-point lazy state preservation error\n");
	}
#endif /* !defined(CONFIG_ARMV7_M_ARMV8_M_FP) */
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

	if (SCB->CFSR & SCB_CFSR_STKERR_Msk) {
		PR_EXC("  Stacking error\n");
	} else if (SCB->CFSR & SCB_CFSR_UNSTKERR_Msk) {
		PR_EXC("  Unstacking error\n");
	} else if (SCB->CFSR & SCB_CFSR_PRECISERR_Msk) {
		PR_EXC("  Precise data bus error\n");
		/* In a fault handler, to determine the true faulting address:
		 * 1. Read and save the BFAR value.
		 * 2. Read the BFARVALID bit in the BFSR.
		 * The BFAR address is valid only if this bit is 1.
		 *
		 * Software must follow this sequence because another
		 * higher priority exception might change the BFAR value.
		 */
		STORE_xFAR(bfar, SCB->BFAR);

		if (SCB->CFSR & SCB_CFSR_BFARVALID_Msk) {
			PR_EXC("  Address: 0x%x\n", bfar);
			if (fromHardFault) {
				/* clear SCB_CFSR_BFAR[VALID] to reset */
				SCB->CFSR &= ~SCB_CFSR_BFARVALID_Msk;
			}
		}
		/* it's possible to have both a precise and imprecise fault */
		if (SCB->CFSR & SCB_CFSR_IMPRECISERR_Msk) {
			PR_EXC("  Imprecise data bus error\n");
		}
	} else if (SCB->CFSR & SCB_CFSR_IMPRECISERR_Msk) {
		PR_EXC("  Imprecise data bus error\n");
	} else if (SCB->CFSR & SCB_CFSR_IBUSERR_Msk) {
		PR_EXC("  Instruction bus error\n");
#if !defined(CONFIG_ARMV7_M_ARMV8_M_FP)
	}
#else
	} else if (SCB->CFSR & SCB_CFSR_LSPERR_Msk) {
		PR_EXC("  Floating-point lazy state preservation error\n");
	}
#endif /* !defined(CONFIG_ARMV7_M_ARMV8_M_FP) */

#if defined(CONFIG_ARMV8_M_MAINLINE)
	/* clear BSFR sticky bits */
	SCB->CFSR |= SCB_CFSR_BUSFAULTSR_Msk;
#endif /* CONFIG_ARMV8_M_MAINLINE */
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
	if (SCB->CFSR & SCB_CFSR_DIVBYZERO_Msk) {
		PR_EXC("  Division by zero\n");
	}
	if (SCB->CFSR & SCB_CFSR_UNALIGNED_Msk) {
		PR_EXC("  Unaligned memory access\n");
	}
#if defined(CONFIG_ARMV8_M_MAINLINE)
	if (SCB->CFSR & SCB_CFSR_STKOF_Msk) {
		PR_EXC("  Stack overflow\n");
	}
#endif /* CONFIG_ARMV8_M_MAINLINE */
	if (SCB->CFSR & SCB_CFSR_NOCP_Msk) {
		PR_EXC("  No coprocessor instructions\n");
	}
	if (SCB->CFSR & SCB_CFSR_INVPC_Msk) {
		PR_EXC("  Illegal load of EXC_RETURN into PC\n");
	}
	if (SCB->CFSR & SCB_CFSR_INVSTATE_Msk) {
		PR_EXC("  Illegal use of the EPSR\n");
	}
	if (SCB->CFSR & SCB_CFSR_UNDEFINSTR_Msk) {
		PR_EXC("  Attempt to execute undefined instruction\n");
	}

	/* clear USFR sticky bits */
	SCB->CFSR |= SCB_CFSR_USGFAULTSR_Msk;
}

#if defined(CONFIG_ARM_SECURE_FIRMWARE)
/**
 *
 * @brief Dump secure fault information
 *
 * See _FaultDump() for example.
 *
 * @return N/A
 */
static void _SecureFault(const NANO_ESF *esf)
{
	PR_EXC("***** SECURE FAULT *****\n");

	_FaultThreadShow(esf);

	STORE_xFAR(sfar, SAU->SFAR);
	if (SAU->SFSR & SAU_SFSR_SFARVALID_Msk) {
		PR_EXC("  Address: 0x%x\n", sfar);
	}

	/* bits are sticky: they stack and must be reset */
	if (SAU->SFSR & SAU_SFSR_INVEP_Msk) {
		PR_EXC("  Invalid entry point\n");
	} else if (SAU->SFSR & SAU_SFSR_INVIS_Msk) {
		PR_EXC("  Invalid integrity signature\n");
	} else if (SAU->SFSR & SAU_SFSR_INVER_Msk) {
		PR_EXC("  Invalid exception return\n");
	} else if (SAU->SFSR & SAU_SFSR_AUVIOL_Msk) {
		PR_EXC("  Attribution unit violation\n");
	} else if (SAU->SFSR & SAU_SFSR_INVTRAN_Msk) {
		PR_EXC("  Invalid transition\n");
	} else if (SAU->SFSR & SAU_SFSR_LSPERR_Msk) {
		PR_EXC("  Lazy state preservation\n");
	} else if (SAU->SFSR & SAU_SFSR_LSERR_Msk) {
		PR_EXC("  Lazy state error\n");
	}

	/* SecureFault is never banked between security states. Therefore,
	 * we may wish to, additionally, inspect the state of the Non-Secure
	 * execution (program counter), to gain more information regarding
	 * the root cause of the fault.
	 */
	NANO_ESF *esf_ns;
	if (SCB_NS->ICSR & SCB_ICSR_RETTOBASE_Msk) {
		esf_ns = (NANO_ESF *)__TZ_get_PSP_NS();
	} else {
		esf_ns = (NANO_ESF *)__TZ_get_MSP_NS();
	}
	PR_EXC("  NS instruction address:  0x%x\n", esf_ns->pc);

	/* clear SFSR sticky bits */
	SAU->SFSR |= 0xFF;
}
#endif /* defined(CONFIG_ARM_SECURE_FIRMWARE) */

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
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

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

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	_FaultThreadShow(esf);
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
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
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
		} else if (SAU->SFSR) {
			_SecureFault(esf);
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
		}
	}
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
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
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	/* HardFault is used for all fault conditions on ARMv6-M. */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	case 4:
		_MpuFault(esf, 0);
		break;
	case 5:
		_BusFault(esf, 0);
		break;
	case 6:
		_UsageFault(esf);
		break;
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	case 7:
		_SecureFault(esf);
		break;
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
	case 12:
		_DebugMonitor(esf);
		break;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
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
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
#if defined(CONFIG_BUILTIN_STACK_GUARD)
	/* If Stack guarding via SP limit checking is enabled, disable
	 * SP limit checking inside HardFault and NMI. This is done
	 * in order to allow for the desired fault logging to execute
	 * properly in all cases.
	 *
	 * Note that this could allow a Secure Firmware Main Stack
	 * to descend into non-secure region during HardFault and
	 * NMI exception entry. To prevent from this, non-secure
	 * memory regions must be located higher than secure memory
	 * regions.
	 *
	 * For Non-Secure Firmware this could allow the Non-Secure Main
	 * Stack to attempt to descend into secure region, in which case a
	 * Secure Hard Fault will occur and we can track the fault from there.
	 */
	SCB->CCR |= SCB_CCR_STKOFHFNMIGN_Msk;
#endif /* CONFIG_BUILTIN_STACK_GUARD */
}
