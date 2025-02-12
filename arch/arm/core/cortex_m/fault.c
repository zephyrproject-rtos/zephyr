/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common fault handler for ARM Cortex-M
 *
 * Common fault handler for ARM Cortex-M processors.
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <inttypes.h>
#include <zephyr/arch/common/exc_handle.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#if defined(CONFIG_PRINTK) || defined(CONFIG_LOG)
#define PR_EXC(...) LOG_ERR(__VA_ARGS__)
#define STORE_xFAR(reg_var, reg) uint32_t reg_var = (uint32_t)reg
#else
#define PR_EXC(...)
#define STORE_xFAR(reg_var, reg)
#endif /* CONFIG_PRINTK || CONFIG_LOG */

#if (CONFIG_FAULT_DUMP == 2)
#define PR_FAULT_INFO(...) PR_EXC(__VA_ARGS__)
#else
#define PR_FAULT_INFO(...)
#endif

#if defined(CONFIG_ARM_MPU) && defined(CONFIG_CPU_HAS_NXP_MPU)
#define EMN(edr)   (((edr) & SYSMPU_EDR_EMN_MASK) >> SYSMPU_EDR_EMN_SHIFT)
#define EACD(edr)  (((edr) & SYSMPU_EDR_EACD_MASK) >> SYSMPU_EDR_EACD_SHIFT)
#endif

/* Exception Return (EXC_RETURN) is provided in LR upon exception entry.
 * It is used to perform an exception return and to detect possible state
 * transition upon exception.
 */

/* Prefix. Indicates that this is an EXC_RETURN value.
 * This field reads as 0b11111111.
 */
#define EXC_RETURN_INDICATOR_PREFIX     (0xFF << 24)
/* bit[0]: Exception Secure. The security domain the exception was taken to. */
#define EXC_RETURN_EXCEPTION_SECURE_Pos 0
#define EXC_RETURN_EXCEPTION_SECURE_Msk \
		BIT(EXC_RETURN_EXCEPTION_SECURE_Pos)
#define EXC_RETURN_EXCEPTION_SECURE_Non_Secure 0
#define EXC_RETURN_EXCEPTION_SECURE_Secure EXC_RETURN_EXCEPTION_SECURE_Msk
/* bit[2]: Stack Pointer selection. */
#define EXC_RETURN_SPSEL_Pos 2
#define EXC_RETURN_SPSEL_Msk BIT(EXC_RETURN_SPSEL_Pos)
#define EXC_RETURN_SPSEL_MAIN 0
#define EXC_RETURN_SPSEL_PROCESS EXC_RETURN_SPSEL_Msk
/* bit[3]: Mode. Indicates the Mode that was stacked from. */
#define EXC_RETURN_MODE_Pos 3
#define EXC_RETURN_MODE_Msk BIT(EXC_RETURN_MODE_Pos)
#define EXC_RETURN_MODE_HANDLER 0
#define EXC_RETURN_MODE_THREAD EXC_RETURN_MODE_Msk
/* bit[4]: Stack frame type. Indicates whether the stack frame is a standard
 * integer only stack frame or an extended floating-point stack frame.
 */
#define EXC_RETURN_STACK_FRAME_TYPE_Pos 4
#define EXC_RETURN_STACK_FRAME_TYPE_Msk BIT(EXC_RETURN_STACK_FRAME_TYPE_Pos)
#define EXC_RETURN_STACK_FRAME_TYPE_EXTENDED 0
#define EXC_RETURN_STACK_FRAME_TYPE_STANDARD EXC_RETURN_STACK_FRAME_TYPE_Msk
/* bit[5]: Default callee register stacking. Indicates whether the default
 * stacking rules apply, or whether the callee registers are already on the
 * stack.
 */
#define EXC_RETURN_CALLEE_STACK_Pos 5
#define EXC_RETURN_CALLEE_STACK_Msk BIT(EXC_RETURN_CALLEE_STACK_Pos)
#define EXC_RETURN_CALLEE_STACK_SKIPPED 0
#define EXC_RETURN_CALLEE_STACK_DEFAULT EXC_RETURN_CALLEE_STACK_Msk
/* bit[6]: Secure or Non-secure stack. Indicates whether a Secure or
 * Non-secure stack is used to restore stack frame on exception return.
 */
#define EXC_RETURN_RETURN_STACK_Pos 6
#define EXC_RETURN_RETURN_STACK_Msk BIT(EXC_RETURN_RETURN_STACK_Pos)
#define EXC_RETURN_RETURN_STACK_Non_Secure 0
#define EXC_RETURN_RETURN_STACK_Secure EXC_RETURN_RETURN_STACK_Msk

/* Integrity signature for an ARMv8-M implementation */
#if defined(CONFIG_ARMV7_M_ARMV8_M_FP)
#define INTEGRITY_SIGNATURE_STD 0xFEFA125BUL
#define INTEGRITY_SIGNATURE_EXT 0xFEFA125AUL
#else
#define INTEGRITY_SIGNATURE 0xFEFA125BUL
#endif /* CONFIG_ARMV7_M_ARMV8_M_FP */
/* Size (in words) of the additional state context that is pushed
 * to the Secure stack during a Non-Secure exception entry.
 */
#define ADDITIONAL_STATE_CONTEXT_WORDS 10

#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
/* helpers to access memory/bus/usage faults */
#define SCB_CFSR_MEMFAULTSR \
	(uint32_t)((SCB->CFSR & SCB_CFSR_MEMFAULTSR_Msk) \
		   >> SCB_CFSR_MEMFAULTSR_Pos)
#define SCB_CFSR_BUSFAULTSR \
	(uint32_t)((SCB->CFSR & SCB_CFSR_BUSFAULTSR_Msk) \
		   >> SCB_CFSR_BUSFAULTSR_Pos)
#define SCB_CFSR_USGFAULTSR \
	(uint32_t)((SCB->CFSR & SCB_CFSR_USGFAULTSR_Msk) \
		   >> SCB_CFSR_USGFAULTSR_Pos)
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */

/**
 *
 * Dump information regarding fault (FAULT_DUMP == 1)
 *
 * Dump information regarding the fault when CONFIG_FAULT_DUMP is set to 1
 * (short form).
 *
 * eg. (precise bus error escalated to hard fault):
 *
 * Fault! EXC #3
 * HARD FAULT: Escalation (see below)!
 * MMFSR: 0x00000000, BFSR: 0x00000082, UFSR: 0x00000000
 * BFAR: 0xff001234
 *
 *
 *
 * Dump information regarding fault (FAULT_DUMP == 2)
 *
 * Dump information regarding the fault when CONFIG_FAULT_DUMP is set to 2
 * (long form), and return the error code for the kernel to identify the fatal
 * error reason.
 *
 * eg. (precise bus error escalated to hard fault):
 *
 * ***** HARD FAULT *****
 *    Fault escalation (see below)
 * ***** BUS FAULT *****
 *   Precise data bus error
 *   Address: 0xff001234
 *
 */

#if (CONFIG_FAULT_DUMP == 1)
static void fault_show(const struct arch_esf *esf, int fault)
{
	PR_EXC("Fault! EXC #%d", fault);

#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	PR_EXC("MMFSR: 0x%x, BFSR: 0x%x, UFSR: 0x%x", SCB_CFSR_MEMFAULTSR,
	       SCB_CFSR_BUSFAULTSR, SCB_CFSR_USGFAULTSR);
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	PR_EXC("SFSR: 0x%x", SAU->SFSR);
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */
}
#else
/* For Dump level 2, detailed information is generated by the
 * fault handling functions for individual fault conditions, so this
 * function is left empty.
 *
 * For Dump level 0, no information needs to be generated.
 */
static void fault_show(const struct arch_esf *esf, int fault)
{
	(void)esf;
	(void)fault;
}
#endif /* FAULT_DUMP == 1 */

#ifdef CONFIG_USERSPACE
Z_EXC_DECLARE(z_arm_user_string_nlen);

static const struct z_exc_handle exceptions[] = {
	Z_EXC_HANDLE(z_arm_user_string_nlen)
};
#endif

/* Perform an assessment whether an MPU fault shall be
 * treated as recoverable.
 *
 * @return true if error is recoverable, otherwise return false.
 */
static bool memory_fault_recoverable(struct arch_esf *esf, bool synchronous)
{
#ifdef CONFIG_USERSPACE
	for (int i = 0; i < ARRAY_SIZE(exceptions); i++) {
		/* Mask out instruction mode */
		uint32_t start = (uint32_t)exceptions[i].start & ~0x1U;
		uint32_t end = (uint32_t)exceptions[i].end & ~0x1U;

#if defined(CONFIG_NULL_POINTER_EXCEPTION_DETECTION_DWT)
	/* Non-synchronous exceptions (e.g. DebugMonitor) may have
	 * allowed PC to continue to the next instruction.
	 */
	end += (synchronous) ? 0x0 : 0x4;
#else
	ARG_UNUSED(synchronous);
#endif
		if (esf->basic.pc >= start && esf->basic.pc < end) {
			esf->basic.pc = (uint32_t)(exceptions[i].fixup);
			return true;
		}
	}
#endif

	return false;
}

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
/* HardFault is used for all fault conditions on ARMv6-M. */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
uint32_t z_check_thread_stack_fail(const uint32_t fault_addr,
	const uint32_t psp);
#endif /* CONFIG_MPU_STACK_GUARD || defined(CONFIG_USERSPACE) */

/**
 *
 * @brief Dump MemManage fault information
 *
 * See z_arm_fault_dump() for example.
 *
 * @return error code to identify the fatal error reason
 */
static uint32_t mem_manage_fault(struct arch_esf *esf, int from_hard_fault,
			      bool *recoverable)
{
	uint32_t reason = K_ERR_ARM_MEM_GENERIC;
	uint32_t mmfar = -EINVAL;

	PR_FAULT_INFO("***** MPU FAULT *****");

	if ((SCB->CFSR & SCB_CFSR_MSTKERR_Msk) != 0) {
		reason = K_ERR_ARM_MEM_STACKING;
		PR_FAULT_INFO("  Stacking error (context area might be"
			" not valid)");
	}
	if ((SCB->CFSR & SCB_CFSR_MUNSTKERR_Msk) != 0) {
		reason = K_ERR_ARM_MEM_UNSTACKING;
		PR_FAULT_INFO("  Unstacking error");
	}
	if ((SCB->CFSR & SCB_CFSR_DACCVIOL_Msk) != 0) {
		reason = K_ERR_ARM_MEM_DATA_ACCESS;
		PR_FAULT_INFO("  Data Access Violation");
		/* In a fault handler, to determine the true faulting address:
		 * 1. Read and save the MMFAR value.
		 * 2. Read the MMARVALID bit in the MMFSR.
		 * The MMFAR address is valid only if this bit is 1.
		 *
		 * Software must follow this sequence because another higher
		 * priority exception might change the MMFAR value.
		 */
		uint32_t temp = SCB->MMFAR;

		if ((SCB->CFSR & SCB_CFSR_MMARVALID_Msk) != 0) {
			mmfar = temp;
			PR_EXC("  MMFAR Address: 0x%x", mmfar);
			if (from_hard_fault != 0) {
				/* clear SCB_MMAR[VALID] to reset */
				SCB->CFSR &= ~SCB_CFSR_MMARVALID_Msk;
			}
		}
	}
	if ((SCB->CFSR & SCB_CFSR_IACCVIOL_Msk) != 0) {
		reason = K_ERR_ARM_MEM_INSTRUCTION_ACCESS;
		PR_FAULT_INFO("  Instruction Access Violation");
	}
#if defined(CONFIG_ARMV7_M_ARMV8_M_FP)
	if ((SCB->CFSR & SCB_CFSR_MLSPERR_Msk) != 0) {
		reason = K_ERR_ARM_MEM_FP_LAZY_STATE_PRESERVATION;
		PR_FAULT_INFO(
			"  Floating-point lazy state preservation error");
	}
#endif /* CONFIG_ARMV7_M_ARMV8_M_FP */

	/* When stack protection is enabled, we need to assess
	 * if the memory violation error is a stack corruption.
	 *
	 * By design, being a Stacking MemManage fault is a necessary
	 * and sufficient condition for a thread stack corruption.
	 * [Cortex-M process stack pointer is always descending and
	 * is never modified by code (except for the context-switch
	 * routine), therefore, a stacking error implies the PSP has
	 * crossed into an area beyond the thread stack.]
	 *
	 * Data Access Violation errors may or may not be caused by
	 * thread stack overflows.
	 */
	if ((SCB->CFSR & SCB_CFSR_MSTKERR_Msk) ||
		(SCB->CFSR & SCB_CFSR_DACCVIOL_Msk)) {
#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
		/* MemManage Faults are always banked between security
		 * states. Therefore, we can safely assume the fault
		 * originated from the same security state.
		 *
		 * As we only assess thread stack corruption, we only
		 * process the error further if the stack frame is on
		 * PSP. For always-banked MemManage Fault, this is
		 * equivalent to inspecting the RETTOBASE flag.
		 *
		 * Note:
		 * It is possible that MMFAR address is not written by the
		 * Cortex-M core; this occurs when the stacking error is
		 * not accompanied by a data access violation error (i.e.
		 * when stack overflows due to the exception entry frame
		 * stacking): z_check_thread_stack_fail() shall be able to
		 * handle the case of 'mmfar' holding the -EINVAL value.
		 */
		if (SCB->ICSR & SCB_ICSR_RETTOBASE_Msk) {
			uint32_t min_stack_ptr = z_check_thread_stack_fail(mmfar,
				((uint32_t) &esf[0]));

			if (min_stack_ptr) {
				/* When MemManage Stacking Error has occurred,
				 * the stack context frame might be corrupted
				 * but the stack pointer may have actually
				 * descent below the allowed (thread) stack
				 * area. We may face a problem with un-stacking
				 * the frame, upon the exception return, if we
				 * do not have sufficient access permissions to
				 * read the corrupted stack frame. Therefore,
				 * we manually force the stack pointer to the
				 * lowest allowed position, inside the thread's
				 * stack.
				 *
				 * Note:
				 * The PSP will normally be adjusted in a tail-
				 * chained exception performing context switch,
				 * after aborting the corrupted thread. The
				 * adjustment, here, is required as tail-chain
				 * cannot always be guaranteed.
				 *
				 * The manual adjustment of PSP is safe, as we
				 * will not be re-scheduling this thread again
				 * for execution; thread stack corruption is a
				 * fatal error and a thread that corrupted its
				 * stack needs to be aborted.
				 */
				__set_PSP(min_stack_ptr);

				reason = K_ERR_STACK_CHK_FAIL;
			} else {
				__ASSERT(!(SCB->CFSR & SCB_CFSR_MSTKERR_Msk),
					"Stacking error not a stack fail\n");
			}
		}
#else
	(void)mmfar;
	__ASSERT(!(SCB->CFSR & SCB_CFSR_MSTKERR_Msk),
		"Stacking or Data Access Violation error "
		"without stack guard, user-mode or null-pointer detection\n");
#endif /* CONFIG_MPU_STACK_GUARD || CONFIG_USERSPACE */
	}

	/* When we were handling this fault, we may have triggered a fp
	 * lazy stacking Memory Manage fault. At the time of writing, this
	 * can happen when printing.  If that's true, we should clear the
	 * pending flag in addition to the clearing the reason for the fault
	 */
#if defined(CONFIG_ARMV7_M_ARMV8_M_FP)
	if ((SCB->CFSR & SCB_CFSR_MLSPERR_Msk) != 0) {
		SCB->SHCSR &= ~SCB_SHCSR_MEMFAULTPENDED_Msk;
	}
#endif /* CONFIG_ARMV7_M_ARMV8_M_FP */

	/* clear MMFSR sticky bits */
	SCB->CFSR |= SCB_CFSR_MEMFAULTSR_Msk;

	/* Assess whether system shall ignore/recover from this MPU fault. */
	*recoverable = memory_fault_recoverable(esf, true);

	return reason;
}

/**
 *
 * @brief Dump BusFault information
 *
 * See z_arm_fault_dump() for example.
 *
 * @return error code to identify the fatal error reason.
 *
 */
static int bus_fault(struct arch_esf *esf, int from_hard_fault, bool *recoverable)
{
	uint32_t reason = K_ERR_ARM_BUS_GENERIC;

	PR_FAULT_INFO("***** BUS FAULT *****");

	if (SCB->CFSR & SCB_CFSR_STKERR_Msk) {
		reason = K_ERR_ARM_BUS_STACKING;
		PR_FAULT_INFO("  Stacking error");
	}
	if (SCB->CFSR & SCB_CFSR_UNSTKERR_Msk) {
		reason = K_ERR_ARM_BUS_UNSTACKING;
		PR_FAULT_INFO("  Unstacking error");
	}
	if (SCB->CFSR & SCB_CFSR_PRECISERR_Msk) {
		reason = K_ERR_ARM_BUS_PRECISE_DATA_BUS;
		PR_FAULT_INFO("  Precise data bus error");
		/* In a fault handler, to determine the true faulting address:
		 * 1. Read and save the BFAR value.
		 * 2. Read the BFARVALID bit in the BFSR.
		 * The BFAR address is valid only if this bit is 1.
		 *
		 * Software must follow this sequence because another
		 * higher priority exception might change the BFAR value.
		 */
		STORE_xFAR(bfar, SCB->BFAR);

		if ((SCB->CFSR & SCB_CFSR_BFARVALID_Msk) != 0) {
			PR_EXC("  BFAR Address: 0x%x", bfar);
			if (from_hard_fault != 0) {
				/* clear SCB_CFSR_BFAR[VALID] to reset */
				SCB->CFSR &= ~SCB_CFSR_BFARVALID_Msk;
			}
		}
	}
	if (SCB->CFSR & SCB_CFSR_IMPRECISERR_Msk) {
		reason = K_ERR_ARM_BUS_IMPRECISE_DATA_BUS;
		PR_FAULT_INFO("  Imprecise data bus error");
	}
	if ((SCB->CFSR & SCB_CFSR_IBUSERR_Msk) != 0) {
		reason = K_ERR_ARM_BUS_INSTRUCTION_BUS;
		PR_FAULT_INFO("  Instruction bus error");
#if !defined(CONFIG_ARMV7_M_ARMV8_M_FP)
	}
#else
	} else if (SCB->CFSR & SCB_CFSR_LSPERR_Msk) {
		reason = K_ERR_ARM_BUS_FP_LAZY_STATE_PRESERVATION;
		PR_FAULT_INFO("  Floating-point lazy state preservation error");
	} else {
		;
	}
#endif /* !defined(CONFIG_ARMV7_M_ARMV8_M_FP) */

#if defined(CONFIG_ARM_MPU) && defined(CONFIG_CPU_HAS_NXP_MPU)
	uint32_t sperr = SYSMPU->CESR & SYSMPU_CESR_SPERR_MASK;
	uint32_t mask = BIT(31);
	int i;
	uint32_t ear = -EINVAL;

	if (sperr) {
		for (i = 0; i < SYSMPU_EAR_COUNT; i++, mask >>= 1) {
			if ((sperr & mask) == 0U) {
				continue;
			}
			STORE_xFAR(edr, SYSMPU->SP[i].EDR);
			ear = SYSMPU->SP[i].EAR;

			PR_FAULT_INFO("  NXP MPU error, port %d", i);
			PR_FAULT_INFO("    Mode: %s, %s Address: 0x%x",
			       edr & BIT(2) ? "Supervisor" : "User",
			       edr & BIT(1) ? "Data" : "Instruction",
			       ear);
			PR_FAULT_INFO(
					"    Type: %s, Master: %d, Regions: 0x%x",
			       edr & BIT(0) ? "Write" : "Read",
			       EMN(edr), EACD(edr));

			/* When stack protection is enabled, we need to assess
			 * if the memory violation error is a stack corruption.
			 *
			 * By design, being a Stacking Bus fault is a necessary
			 * and sufficient condition for a stack corruption.
			 */
			if (SCB->CFSR & SCB_CFSR_STKERR_Msk) {
#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
				/* Note: we can assume the fault originated
				 * from the same security state for ARM
				 * platforms implementing the NXP MPU
				 * (CONFIG_CPU_HAS_NXP_MPU=y).
				 *
				 * As we only assess thread stack corruption,
				 * we only process the error further, if the
				 * stack frame is on PSP. For NXP MPU-related
				 * Bus Faults (banked), this is equivalent to
				 * inspecting the RETTOBASE flag.
				 */
				if (SCB->ICSR & SCB_ICSR_RETTOBASE_Msk) {
					uint32_t min_stack_ptr =
						z_check_thread_stack_fail(ear,
							((uint32_t) &esf[0]));

					if (min_stack_ptr) {
						/* When BusFault Stacking Error
						 * has occurred, the stack
						 * context frame might be
						 * corrupted but the stack
						 * pointer may have actually
						 * moved. We may face problems
						 * with un-stacking the frame,
						 * upon exception return, if we
						 * do not have sufficient
						 * permissions to read the
						 * corrupted stack frame.
						 * Therefore, we manually force
						 * the stack pointer to the
						 * lowest allowed position.
						 *
						 * Note:
						 * The PSP will normally be
						 * adjusted in a tail-chained
						 * exception performing context
						 * switch, after aborting the
						 * corrupted thread. Here, the
						 * adjustment is required as
						 * tail-chain cannot always be
						 * guaranteed.
						 */
						__set_PSP(min_stack_ptr);

						reason =
							K_ERR_STACK_CHK_FAIL;
						break;
					}
				}
#else
				(void)ear;
				__ASSERT(0,
					"Stacking error without stack guard"
					"or User-mode support");
#endif /* CONFIG_MPU_STACK_GUARD || CONFIG_USERSPACE */
			}
		}
		SYSMPU->CESR &= ~sperr;
	}
#endif /* defined(CONFIG_ARM_MPU) && defined(CONFIG_CPU_HAS_NXP_MPU) */

	/* clear BFSR sticky bits */
	SCB->CFSR |= SCB_CFSR_BUSFAULTSR_Msk;

	*recoverable = memory_fault_recoverable(esf, true);

	return reason;
}

/**
 *
 * @brief Dump UsageFault information
 *
 * See z_arm_fault_dump() for example.
 *
 * @return error code to identify the fatal error reason
 */
static uint32_t usage_fault(const struct arch_esf *esf)
{
	uint32_t reason = K_ERR_ARM_USAGE_GENERIC;

	PR_FAULT_INFO("***** USAGE FAULT *****");

	/* bits are sticky: they stack and must be reset */
	if ((SCB->CFSR & SCB_CFSR_DIVBYZERO_Msk) != 0) {
		reason = K_ERR_ARM_USAGE_DIV_0;
		PR_FAULT_INFO("  Division by zero");
	}
	if ((SCB->CFSR & SCB_CFSR_UNALIGNED_Msk) != 0) {
		reason = K_ERR_ARM_USAGE_UNALIGNED_ACCESS;
		PR_FAULT_INFO("  Unaligned memory access");
	}
#if defined(CONFIG_ARMV8_M_MAINLINE)
	if ((SCB->CFSR & SCB_CFSR_STKOF_Msk) != 0) {
		reason = K_ERR_ARM_USAGE_STACK_OVERFLOW;
		PR_FAULT_INFO("  Stack overflow (context area not valid)");
#if defined(CONFIG_BUILTIN_STACK_GUARD)
		/* Stack Overflows are always reported as stack corruption
		 * errors. Note that the built-in stack overflow mechanism
		 * prevents the context area to be loaded on the stack upon
		 * UsageFault exception entry. As a result, we cannot rely
		 * on the reported faulty instruction address, to determine
		 * the instruction that triggered the stack overflow.
		 */
		reason = K_ERR_STACK_CHK_FAIL;
#endif /* CONFIG_BUILTIN_STACK_GUARD */
	}
#endif /* CONFIG_ARMV8_M_MAINLINE */
	if ((SCB->CFSR & SCB_CFSR_NOCP_Msk) != 0) {
		reason = K_ERR_ARM_USAGE_NO_COPROCESSOR;
		PR_FAULT_INFO("  No coprocessor instructions");
	}
	if ((SCB->CFSR & SCB_CFSR_INVPC_Msk) != 0) {
		reason = K_ERR_ARM_USAGE_ILLEGAL_EXC_RETURN;
		PR_FAULT_INFO("  Illegal load of EXC_RETURN into PC");
	}
	if ((SCB->CFSR & SCB_CFSR_INVSTATE_Msk) != 0) {
		reason = K_ERR_ARM_USAGE_ILLEGAL_EPSR;
		PR_FAULT_INFO("  Illegal use of the EPSR");
	}
	if ((SCB->CFSR & SCB_CFSR_UNDEFINSTR_Msk) != 0) {
		reason = K_ERR_ARM_USAGE_UNDEFINED_INSTRUCTION;
		PR_FAULT_INFO("  Attempt to execute undefined instruction");
	}

	/* clear UFSR sticky bits */
	SCB->CFSR |= SCB_CFSR_USGFAULTSR_Msk;

	return reason;
}

#if defined(CONFIG_ARM_SECURE_FIRMWARE)
/**
 *
 * @brief Dump SecureFault information
 *
 * See z_arm_fault_dump() for example.
 *
 * @return error code to identify the fatal error reason
 */
static uint32_t secure_fault(const struct arch_esf *esf)
{
	uint32_t reason = K_ERR_ARM_SECURE_GENERIC;

	PR_FAULT_INFO("***** SECURE FAULT *****");

	STORE_xFAR(sfar, SAU->SFAR);
	if ((SAU->SFSR & SAU_SFSR_SFARVALID_Msk) != 0) {
		PR_EXC("  Address: 0x%x", sfar);
	}

	/* bits are sticky: they stack and must be reset */
	if ((SAU->SFSR & SAU_SFSR_INVEP_Msk) != 0) {
		reason = K_ERR_ARM_SECURE_ENTRY_POINT;
		PR_FAULT_INFO("  Invalid entry point");
	} else if ((SAU->SFSR & SAU_SFSR_INVIS_Msk) != 0) {
		reason = K_ERR_ARM_SECURE_INTEGRITY_SIGNATURE;
		PR_FAULT_INFO("  Invalid integrity signature");
	} else if ((SAU->SFSR & SAU_SFSR_INVER_Msk) != 0) {
		reason = K_ERR_ARM_SECURE_EXCEPTION_RETURN;
		PR_FAULT_INFO("  Invalid exception return");
	} else if ((SAU->SFSR & SAU_SFSR_AUVIOL_Msk) != 0) {
		reason = K_ERR_ARM_SECURE_ATTRIBUTION_UNIT;
		PR_FAULT_INFO("  Attribution unit violation");
	} else if ((SAU->SFSR & SAU_SFSR_INVTRAN_Msk) != 0) {
		reason = K_ERR_ARM_SECURE_TRANSITION;
		PR_FAULT_INFO("  Invalid transition");
	} else if ((SAU->SFSR & SAU_SFSR_LSPERR_Msk) != 0) {
		reason = K_ERR_ARM_SECURE_LAZY_STATE_PRESERVATION;
		PR_FAULT_INFO("  Lazy state preservation");
	} else if ((SAU->SFSR & SAU_SFSR_LSERR_Msk) != 0) {
		reason = K_ERR_ARM_SECURE_LAZY_STATE_ERROR;
		PR_FAULT_INFO("  Lazy state error");
	}

	/* clear SFSR sticky bits */
	SAU->SFSR |= 0xFF;

	return reason;
}
#endif /* defined(CONFIG_ARM_SECURE_FIRMWARE) */

/**
 *
 * @brief Dump debug monitor exception information
 *
 * See z_arm_fault_dump() for example.
 *
 */
static void debug_monitor(struct arch_esf *esf, bool *recoverable)
{
	*recoverable = false;

	PR_FAULT_INFO(
		"***** Debug monitor exception *****");

#if defined(CONFIG_NULL_POINTER_EXCEPTION_DETECTION_DWT)
	if (!z_arm_debug_monitor_event_error_check()) {
		/* By default, all debug monitor exceptions that are not
		 * treated as errors by z_arm_debug_event_error_check(),
		 * they are considered as recoverable errors.
		 */
		*recoverable = true;
	} else {

		*recoverable = memory_fault_recoverable(esf, false);
	}

#endif
}

#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

static inline bool z_arm_is_synchronous_svc(struct arch_esf *esf)
{
	uint16_t *ret_addr = (uint16_t *)esf->basic.pc;
	/* SVC is a 16-bit instruction. On a synchronous SVC
	 * escalated to Hard Fault, the return address is the
	 * next instruction, i.e. after the SVC.
	 */
#define _SVC_OPCODE 0xDF00

	/* We are about to de-reference the program counter at the
	 * time of fault to determine if it was a SVC
	 * instruction. However, we don't know if the pc itself is
	 * valid -- we could have faulted due to trying to execute a
	 * corrupted function pointer.
	 *
	 * We will temporarily ignore BusFault's so a bad program
	 * counter does not trigger ARM lockup condition.
	 */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) && !defined(CONFIG_ARMV8_M_BASELINE)
	/* Note: ARMv6-M does not support CCR.BFHFNMIGN so this access
	 * could generate a fault if the pc was invalid.
	 */
	uint16_t fault_insn = *(ret_addr - 1);
#else
	SCB->CCR |= SCB_CCR_BFHFNMIGN_Msk;
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	uint16_t fault_insn = *(ret_addr - 1);

	SCB->CCR &= ~SCB_CCR_BFHFNMIGN_Msk;
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
#endif /* ARMV6_M_ARMV8_M_BASELINE && !ARMV8_M_BASELINE */

	if (((fault_insn & 0xff00) == _SVC_OPCODE) &&
		((fault_insn & 0x00ff) == _SVC_CALL_RUNTIME_EXCEPT)) {
		return true;
	}
#undef _SVC_OPCODE
	return false;
}

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
static inline bool z_arm_is_pc_valid(uintptr_t pc)
{
	/* Is it in valid text region */
	if ((((uintptr_t)&__text_region_start) <= pc) && (pc < ((uintptr_t)&__text_region_end))) {
		return true;
	}

	/* Is it in valid ramfunc range */
	if ((((uintptr_t)&__ramfunc_start) <= pc) && (pc < ((uintptr_t)&__ramfunc_end))) {
		return true;
	}

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_itcm), okay)
	/* Is it in the ITCM */
	if ((((uintptr_t)&__itcm_start) <= pc) && (pc < ((uintptr_t)&__itcm_end))) {
		return true;
	}
#endif

	return false;
}
#endif

/**
 *
 * @brief Dump hard fault information
 *
 * See z_arm_fault_dump() for example.
 *
 * @return error code to identify the fatal error reason
 */
static uint32_t hard_fault(struct arch_esf *esf, bool *recoverable)
{
	uint32_t reason = K_ERR_CPU_EXCEPTION;

	PR_FAULT_INFO("***** HARD FAULT *****");

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	/* Workaround for #18712:
	 * HardFault may be due to escalation, as a result of
	 * an SVC instruction that could not be executed; this
	 * can occur if ARCH_EXCEPT() is called by an ISR,
	 * which executes at priority equal to the SVC handler
	 * priority. We handle the case of Kernel OOPS and Stack
	 * Fail here.
	 */

	if (z_arm_is_pc_valid((uintptr_t)esf->basic.pc) && z_arm_is_synchronous_svc(esf)) {
		PR_EXC("ARCH_EXCEPT with reason %x\n", esf->basic.r0);
		reason = esf->basic.r0;
	}

	*recoverable = memory_fault_recoverable(esf, true);
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	*recoverable = false;

	if ((SCB->HFSR & SCB_HFSR_VECTTBL_Msk) != 0) {
		PR_EXC("  Bus fault on vector table read");
	} else if ((SCB->HFSR & SCB_HFSR_DEBUGEVT_Msk) != 0) {
		PR_EXC("  Debug event");
	} else if ((SCB->HFSR & SCB_HFSR_FORCED_Msk) != 0) {
		PR_EXC("  Fault escalation (see below)");
		if (z_arm_is_synchronous_svc(esf)) {
			PR_EXC("ARCH_EXCEPT with reason %x\n", esf->basic.r0);
			reason = esf->basic.r0;
		} else if ((SCB->CFSR & SCB_CFSR_MEMFAULTSR_Msk) != 0) {
			reason = mem_manage_fault(esf, 1, recoverable);
		} else if ((SCB->CFSR & SCB_CFSR_BUSFAULTSR_Msk) != 0) {
			reason = bus_fault(esf, 1, recoverable);
		} else if ((SCB->CFSR & SCB_CFSR_USGFAULTSR_Msk) != 0) {
			reason = usage_fault(esf);
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
		} else if (SAU->SFSR != 0) {
			reason = secure_fault(esf);
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
		} else {
			__ASSERT(0,
			"Fault escalation without FSR info");
		}
	} else {
		__ASSERT(0,
		"HardFault without HFSR info"
		" Shall never occur");
	}
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

	return reason;
}

/**
 *
 * @brief Dump reserved exception information
 *
 * See z_arm_fault_dump() for example.
 *
 */
static void reserved_exception(const struct arch_esf *esf, int fault)
{
	ARG_UNUSED(esf);

	PR_FAULT_INFO("***** %s %d) *****",
	       fault < 16 ? "Reserved Exception (" : "Spurious interrupt (IRQ ",
	       fault - 16);
}

/* Handler function for ARM fault conditions. */
static uint32_t fault_handle(struct arch_esf *esf, int fault, bool *recoverable)
{
	uint32_t reason = K_ERR_CPU_EXCEPTION;

	*recoverable = false;

	switch (fault) {
	case 3:
		reason = hard_fault(esf, recoverable);
		break;
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	/* HardFault is raised for all fault conditions on ARMv6-M. */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	case 4:
		reason = mem_manage_fault(esf, 0, recoverable);
		break;
	case 5:
		reason = bus_fault(esf, 0, recoverable);
		break;
	case 6:
		reason = usage_fault(esf);
		break;
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	case 7:
		reason = secure_fault(esf);
		break;
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
	case 12:
		debug_monitor(esf, recoverable);
		break;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
	default:
		reserved_exception(esf, fault);
		break;
	}

	if ((*recoverable) == false) {
		/* Dump generic information about the fault. */
		fault_show(esf, fault);
	}

	return reason;
}

#if defined(CONFIG_ARM_SECURE_FIRMWARE)
#if (CONFIG_FAULT_DUMP == 2)
/**
 * @brief Dump the Secure Stack information for an exception that
 * has occurred in Non-Secure state.
 *
 * @param secure_esf Pointer to the secure stack frame.
 */
static void secure_stack_dump(const struct arch_esf *secure_esf)
{
	/*
	 * In case a Non-Secure exception interrupted the Secure
	 * execution, the Secure state has stacked the additional
	 * state context and the top of the stack contains the
	 * integrity signature.
	 *
	 * In case of a Non-Secure function call the top of the
	 * stack contains the return address to Secure state.
	 */
	uint32_t *top_of_sec_stack = (uint32_t *)secure_esf;
	uint32_t sec_ret_addr;
#if defined(CONFIG_ARMV7_M_ARMV8_M_FP)
	if ((*top_of_sec_stack == INTEGRITY_SIGNATURE_STD) ||
		(*top_of_sec_stack == INTEGRITY_SIGNATURE_EXT)) {
#else
	if (*top_of_sec_stack == INTEGRITY_SIGNATURE) {
#endif /* CONFIG_ARMV7_M_ARMV8_M_FP */
		/* Secure state interrupted by a Non-Secure exception.
		 * The return address after the additional state
		 * context, stacked by the Secure code upon
		 * Non-Secure exception entry.
		 */
		top_of_sec_stack += ADDITIONAL_STATE_CONTEXT_WORDS;
		secure_esf = (const struct arch_esf *)top_of_sec_stack;
		sec_ret_addr = secure_esf->basic.pc;
	} else {
		/* Exception during Non-Secure function call.
		 * The return address is located on top of stack.
		 */
		sec_ret_addr = *top_of_sec_stack;
	}
	PR_FAULT_INFO("  S instruction address:  0x%x", sec_ret_addr);

}
#define SECURE_STACK_DUMP(esf) secure_stack_dump(esf)
#else
/* We do not dump the Secure stack information for lower dump levels. */
#define SECURE_STACK_DUMP(esf)
#endif /* CONFIG_FAULT_DUMP== 2 */
#endif /* CONFIG_ARM_SECURE_FIRMWARE */

/*
 * This internal function does the following:
 *
 * - Retrieves the exception stack frame
 * - Evaluates whether to report being in a nested exception
 *
 * If the ESF is not successfully retrieved, the function signals
 * an error by returning NULL.
 *
 * @return ESF pointer on success, otherwise return NULL
 */
static inline struct arch_esf *get_esf(uint32_t msp, uint32_t psp, uint32_t exc_return,
	bool *nested_exc)
{
	bool alternative_state_exc = false;
	struct arch_esf *ptr_esf = NULL;

	*nested_exc = false;

	if ((exc_return & EXC_RETURN_INDICATOR_PREFIX) !=
			EXC_RETURN_INDICATOR_PREFIX) {
		/* Invalid EXC_RETURN value. This is a fatal error. */
		return NULL;
	}

#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	if ((exc_return & EXC_RETURN_EXCEPTION_SECURE_Secure) == 0U) {
		/* Secure Firmware shall only handle Secure Exceptions.
		 * This is a fatal error.
		 */
		return NULL;
	}

	if (exc_return & EXC_RETURN_RETURN_STACK_Secure) {
		/* Exception entry occurred in Secure stack. */
	} else {
		/* Exception entry occurred in Non-Secure stack. Therefore,
		 * msp/psp point to the Secure stack, however, the actual
		 * exception stack frame is located in the Non-Secure stack.
		 */
		alternative_state_exc = true;

		/* Dump the Secure stack before handling the actual fault. */
		struct arch_esf *secure_esf;

		if (exc_return & EXC_RETURN_SPSEL_PROCESS) {
			/* Secure stack pointed by PSP */
			secure_esf = (struct arch_esf *)psp;
		} else {
			/* Secure stack pointed by MSP */
			secure_esf = (struct arch_esf *)msp;
			*nested_exc = true;
		}

		SECURE_STACK_DUMP(secure_esf);

		/* Handle the actual fault.
		 * Extract the correct stack frame from the Non-Secure state
		 * and supply it to the fault handing function.
		 */
		if (exc_return & EXC_RETURN_MODE_THREAD) {
			ptr_esf = (struct arch_esf *)__TZ_get_PSP_NS();
		} else {
			ptr_esf = (struct arch_esf *)__TZ_get_MSP_NS();
		}
	}
#elif defined(CONFIG_ARM_NONSECURE_FIRMWARE)
	if (exc_return & EXC_RETURN_EXCEPTION_SECURE_Secure) {
		/* Non-Secure Firmware shall only handle Non-Secure Exceptions.
		 * This is a fatal error.
		 */
		return NULL;
	}

	if (exc_return & EXC_RETURN_RETURN_STACK_Secure) {
		/* Exception entry occurred in Secure stack.
		 *
		 * Note that Non-Secure firmware cannot inspect the Secure
		 * stack to determine the root cause of the fault. Fault
		 * inspection will indicate the Non-Secure instruction
		 * that performed the branch to the Secure domain.
		 */
		alternative_state_exc = true;

		PR_FAULT_INFO("Exception occurred in Secure State");

		if (exc_return & EXC_RETURN_SPSEL_PROCESS) {
			/* Non-Secure stack frame on PSP */
			ptr_esf = (struct arch_esf *)psp;
		} else {
			/* Non-Secure stack frame on MSP */
			ptr_esf = (struct arch_esf *)msp;
		}
	} else {
		/* Exception entry occurred in Non-Secure stack. */
	}
#else
	/* The processor has a single execution state.
	 * We verify that the Thread mode is using PSP.
	 */
	if ((exc_return & EXC_RETURN_MODE_THREAD) &&
		(!(exc_return & EXC_RETURN_SPSEL_PROCESS))) {
		PR_EXC("SPSEL in thread mode does not indicate PSP");
		return NULL;
	}
#endif /* CONFIG_ARM_SECURE_FIRMWARE */

	if (!alternative_state_exc) {
		if (exc_return & EXC_RETURN_MODE_THREAD) {
			/* Returning to thread mode */
			ptr_esf =  (struct arch_esf *)psp;

		} else {
			/* Returning to handler mode */
			ptr_esf = (struct arch_esf *)msp;
			*nested_exc = true;
		}
	}

	return ptr_esf;
}

/**
 *
 * @brief ARM Fault handler
 *
 * This routine is called when fatal error conditions are detected by hardware
 * and is responsible for:
 * - resetting the processor fault status registers (for the case when the
 *   error handling policy allows the system to recover from the error),
 * - reporting the error information,
 * - determining the error reason to be provided as input to the user-
 *   provided routine, k_sys_fatal_error_handler().
 * The k_sys_fatal_error_handler() is invoked once the above operations are
 * completed, and is responsible for implementing the error handling policy.
 *
 * The function needs, first, to determine the exception stack frame.
 * Note that the current security state might not be the actual
 * state in which the processor was executing, when the exception occurred.
 * The actual state may need to be determined by inspecting the EXC_RETURN
 * value, which is provided as argument to the Fault handler.
 *
 * If the exception occurred in the same security state, the stack frame
 * will be pointed to by either MSP or PSP depending on the processor
 * execution state when the exception occurred. MSP and PSP values are
 * provided as arguments to the Fault handler.
 *
 * @param msp MSP value immediately after the exception occurred
 * @param psp PSP value immediately after the exception occurred
 * @param exc_return EXC_RETURN value present in LR after exception entry.
 * @param callee_regs Callee-saved registers (R4-R11, PSP)
 *
 */
void z_arm_fault(uint32_t msp, uint32_t psp, uint32_t exc_return,
	_callee_saved_t *callee_regs)
{
	uint32_t reason = K_ERR_CPU_EXCEPTION;
	int fault = SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk;
	bool recoverable, nested_exc;
	struct arch_esf *esf;

	/* Create a stack-ed copy of the ESF to be used during
	 * the fault handling process.
	 */
	struct arch_esf esf_copy;

	/* Force unlock interrupts */
	arch_irq_unlock(0);

	/* Retrieve the Exception Stack Frame (ESF) to be supplied
	 * as argument to the remainder of the fault handling process.
	 */
	 esf = get_esf(msp, psp, exc_return, &nested_exc);
	__ASSERT(esf != NULL,
		"ESF could not be retrieved successfully. Shall never occur.");

#ifdef CONFIG_DEBUG_COREDUMP
	z_arm_coredump_fault_sp = POINTER_TO_UINT(esf);
#endif

	reason = fault_handle(esf, fault, &recoverable);
	if (recoverable) {
		return;
	}

	/* Copy ESF */
#if !defined(CONFIG_EXTRA_EXCEPTION_INFO)
	memcpy(&esf_copy, esf, sizeof(struct arch_esf));
	ARG_UNUSED(callee_regs);
#else
	/* the extra exception info is not present in the original esf
	 * so we only copy the fields before those.
	 */
	memcpy(&esf_copy, esf, offsetof(struct arch_esf, extra_info));
	esf_copy.extra_info = (struct __extra_esf_info) {
		.callee = callee_regs,
		.exc_return = exc_return,
		.msp = msp
	};
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */

	/* Overwrite stacked IPSR to mark a nested exception,
	 * or a return to Thread mode. Note that this may be
	 * required, if the retrieved ESF contents are invalid
	 * due to, for instance, a stacking error.
	 */
	if (nested_exc) {
		if ((esf_copy.basic.xpsr & IPSR_ISR_Msk) == 0) {
			esf_copy.basic.xpsr |= IPSR_ISR_Msk;
		}
	} else {
		esf_copy.basic.xpsr &= ~(IPSR_ISR_Msk);
	}

	if (IS_ENABLED(CONFIG_SIMPLIFIED_EXCEPTION_CODES) && (reason >= K_ERR_ARCH_START)) {
		reason = K_ERR_CPU_EXCEPTION;
	}

	z_arm_fatal_error(reason, &esf_copy);
}

/**
 *
 * @brief Initialization of fault handling
 *
 * Turns on the desired hardware faults.
 *
 */
void z_arm_fault_init(void)
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
#ifdef CONFIG_TRAP_UNALIGNED_ACCESS
	SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
#endif /* CONFIG_TRAP_UNALIGNED_ACCESS */
}
