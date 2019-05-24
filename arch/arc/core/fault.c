/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common fault handler for ARCv2
 *
 * Common fault handler for ARCv2 processors.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <inttypes.h>

#include <kernel.h>
#include <kernel_structs.h>
#include <misc/printk.h>
#include <exc_handle.h>
#include <logging/log_ctrl.h>

u32_t arc_exc_saved_sp;

#ifdef CONFIG_USERSPACE
Z_EXC_DECLARE(z_arch_user_string_nlen);

static const struct z_exc_handle exceptions[] = {
	Z_EXC_HANDLE(z_arch_user_string_nlen)
};
#endif

#if defined(CONFIG_MPU_STACK_GUARD)

#define IS_MPU_GUARD_VIOLATION(guard_start, fault_addr, stack_ptr) \
	((fault_addr >= guard_start) && \
	(fault_addr < (guard_start + STACK_GUARD_SIZE)) && \
	(stack_ptr <= (guard_start + STACK_GUARD_SIZE)))

/**
 * @brief Assess occurrence of current thread's stack corruption
 *
 * This function performs an assessment whether a memory fault (on a
 * given memory address) is the result of stack memory corruption of
 * the current thread.
 *
 * Thread stack corruption for supervisor threads or user threads in
 * privilege mode (when User Space is supported) is reported upon an
 * attempt to access the stack guard area (if MPU Stack Guard feature
 * is supported). Additionally the current thread stack pointer
 * must be pointing inside or below the guard area.
 *
 * Thread stack corruption for user threads in user mode is reported,
 * if the current stack pointer is pointing below the start of the current
 * thread's stack.
 *
 * Notes:
 * - we assume a fully descending stack,
 * - we assume a stacking error has occurred,
 * - the function shall be called when handling MPU privilege violation
 *
 * If stack corruption is detected, the function returns the lowest
 * allowed address where the Stack Pointer can safely point to, to
 * prevent from errors when un-stacking the corrupted stack frame
 * upon exception return.
 *
 * @param fault_addr memory address on which memory access violation
 *                   has been reported.
 * @param sp stack pointer when exception comes out
 *
 * @return The lowest allowed stack frame pointer, if error is a
 *         thread stack corruption, otherwise return 0.
 */
static u32_t z_check_thread_stack_fail(const u32_t fault_addr, u32_t sp)
{
	const struct k_thread *thread = _current;

	if (!thread) {
		return 0;
	}
#if defined(CONFIG_USERSPACE)
	if (thread->arch.priv_stack_start) {
		/* User thread */
		if (z_arc_v2_aux_reg_read(_ARC_V2_ERSTATUS)
			& _ARC_V2_STATUS32_U) {
			/* Thread's user stack corruption */
#ifdef CONFIG_ARC_HAS_SECURE
			sp = z_arc_v2_aux_reg_read(_ARC_V2_SEC_U_SP);
#else
			sp = z_arc_v2_aux_reg_read(_ARC_V2_USER_SP);
#endif
			if (sp <= (u32_t)thread->stack_obj) {
				return (u32_t)thread->stack_obj;
			}
		} else {
			/* User thread in privilege mode */
			if (IS_MPU_GUARD_VIOLATION(
			thread->arch.priv_stack_start - STACK_GUARD_SIZE,
			fault_addr, sp)) {
				/* Thread's privilege stack corruption */
				return thread->arch.priv_stack_start;
			}
		}
	} else {
		/* Supervisor thread */
		if (IS_MPU_GUARD_VIOLATION((u32_t)thread->stack_obj,
			fault_addr, sp)) {
			/* Supervisor thread stack corruption */
			return (u32_t)thread->stack_obj + STACK_GUARD_SIZE;
		}
	}
#else /* CONFIG_USERSPACE */
	if (IS_MPU_GUARD_VIOLATION(thread->stack_info.start,
			fault_addr, sp)) {
		/* Thread stack corruption */
		return thread->stack_info.start + STACK_GUARD_SIZE;
	}
#endif /* CONFIG_USERSPACE */

	return 0;
}

#endif

#ifdef CONFIG_ARC_EXCEPTION_DEBUG
/* For EV_ProtV, the numbering/semantics of the parameter are consistent across
 * several codes, although not all combination will be reported.
 *
 * These codes and parameters do not have associated* names in
 * the technical manual, just switch on the values in Table 6-5
 */
static void dump_protv_access_err(u32_t parameter)
{
	switch (parameter) {
	case 0x1:
		printk("code protection scheme");
		break;
	case 0x2:
		printk("stack checking scheme");
		break;
	case 0x4:
		printk("MPU");
		break;
	case 0x8:
		printk("MMU");
		break;
	case 0x10:
		printk("NVM");
		break;
	case 0x24:
		printk("Secure MPU");
		break;
	case 0x44:
		printk("Secure MPU with SID mismatch");
		break;
	default:
		printk("unknown");
		break;
	}
}

static void dump_protv_exception(u32_t cause, u32_t parameter)
{
	switch (cause) {
	case 0x0:
		printk("Instruction fetch violation: ");
		dump_protv_access_err(parameter);
		break;
	case 0x1:
		printk("Memory read protection violation: ");
		dump_protv_access_err(parameter);
		break;
	case 0x2:
		printk("Memory write protection violation: ");
		dump_protv_access_err(parameter);
		break;
	case 0x3:
		printk("Memory read-modify-write violation: ");
		dump_protv_access_err(parameter);
		break;
	case 0x10:
		printk("Normal vector table in secure memory");
		break;
	case 0x11:
		printk("NS handler code located in S memory");
		break;
	case 0x12:
		printk("NSC Table Range Violation");
		break;
	default:
		printk("unknown");
		break;
	}
}

static void dump_machine_check_exception(u32_t cause, u32_t parameter)
{
	switch (cause) {
	case 0x0:
		printk("double fault");
		break;
	case 0x1:
		printk("overlapping TLB entries");
		break;
	case 0x2:
		printk("fatal TLB error");
		break;
	case 0x3:
		printk("fatal cache error");
		break;
	case 0x4:
		printk("internal memory error on instruction fetch");
		break;
	case 0x5:
		printk("internal memory error on data fetch");
		break;
	case 0x6:
		printk("illegal overlapping MPU entries");
		if (parameter == 0x1) {
			printk(" (jump and branch target)");
		}
		break;
	case 0x10:
		printk("secure vector table not located in secure memory");
		break;
	case 0x11:
		printk("NSC jump table not located in secure memory");
		break;
	case 0x12:
		printk("secure handler code not located in secure memory");
		break;
	case 0x13:
		printk("NSC target address not located in secure memory");
		break;
	case 0x80:
		printk("uncorrectable ECC or parity error in vector memory");
		break;
	default:
		printk("unknown");
		break;
	}
}

static void dump_privilege_exception(u32_t cause, u32_t parameter)
{
	switch (cause) {
	case 0x0:
		printk("Privilege violation");
		break;
	case 0x1:
		printk("disabled extension");
		break;
	case 0x2:
		printk("action point hit");
		break;
	case 0x10:
		switch (parameter) {
		case 0x1:
			printk("N to S return using incorrect return mechanism");
			break;
		case 0x2:
			printk("N to S return with incorrect operating mode");
			break;
		case 0x3:
			printk("IRQ/exception return fetch from wrong mode");
			break;
		case 0x4:
			printk("attempt to halt secure processor in NS mode");
			break;
		case 0x20:
			printk("attempt to access secure resource from normal mode");
			break;
		case 0x40:
			printk("SID violation on resource access (APEX/UAUX/key NVM)");
			break;
		default:
			printk("unknown");
			break;
		}
		break;
	case 0x13:
		switch (parameter) {
		case 0x20:
			printk("attempt to access secure APEX feature from NS mode");
			break;
		case 0x40:
			printk("SID violation on access to APEX feature");
			break;
		default:
			printk("unknown");
			break;
		}
		break;
	default:
		printk("unknown");
		break;
	}
}

static void dump_exception_info(u32_t vector, u32_t cause, u32_t parameter)
{
	if (vector >= 0x10 && vector <= 0xFF) {
		printk("interrupt %u\n", vector);
		return;
	}

	/* Names are exactly as they appear in Designware ARCv2 ISA
	 * Programmer's reference manual for easy searching
	 */
	switch (vector) {
	case ARC_EV_RESET:
		printk("Reset");
		break;
	case ARC_EV_MEM_ERROR:
		printk("Memory Error");
		break;
	case ARC_EV_INS_ERROR:
		printk("Instruction Error");
		break;
	case ARC_EV_MACHINE_CHECK:
		printk("EV_MachineCheck: ");
		dump_machine_check_exception(cause, parameter);
		break;
	case ARC_EV_TLB_MISS_I:
		printk("EV_TLBMissI");
		break;
	case ARC_EV_TLB_MISS_D:
		printk("EV_TLBMissD");
		break;
	case ARC_EV_PROT_V:
		printk("EV_ProtV: ");
		dump_protv_exception(cause, parameter);
		break;
	case ARC_EV_PRIVILEGE_V:
		printk("EV_PrivilegeV: ");
		dump_privilege_exception(cause, parameter);
		break;
	case ARC_EV_SWI:
		printk("EV_SWI");
		break;
	case ARC_EV_TRAP:
		printk("EV_Trap");
		break;
	case ARC_EV_EXTENSION:
		printk("EV_Extension");
		break;
	case ARC_EV_DIV_ZERO:
		printk("EV_DivZero");
		break;
	case ARC_EV_DC_ERROR:
		printk("EV_DCError");
		break;
	case ARC_EV_MISALIGNED:
		printk("EV_Misaligned");
		break;
	case ARC_EV_VEC_UNIT:
		printk("EV_VecUnit");
		break;
	default:
		printk("unknown");
		break;
	}

	printk("\n");
}
#endif /* CONFIG_ARC_EXCEPTION_DEBUG */

/*
 * @brief Fault handler
 *
 * This routine is called when fatal error conditions are detected by hardware
 * and is responsible only for reporting the error. Once reported, it then
 * invokes the user provided routine z_SysFatalErrorHandler() which is
 * responsible for implementing the error handling policy.
 */
void _Fault(NANO_ESF *esf)
{
	u32_t vector, cause, parameter;
	u32_t exc_addr = z_arc_v2_aux_reg_read(_ARC_V2_EFA);
	u32_t ecr = z_arc_v2_aux_reg_read(_ARC_V2_ECR);

#ifdef CONFIG_USERSPACE
	for (int i = 0; i < ARRAY_SIZE(exceptions); i++) {
		u32_t start = (u32_t)exceptions[i].start;
		u32_t end = (u32_t)exceptions[i].end;

		if (esf->pc >= start && esf->pc < end) {
			esf->pc = (u32_t)(exceptions[i].fixup);
			return;
		}
	}
#endif
	LOG_PANIC();

	vector = Z_ARC_V2_ECR_VECTOR(ecr);
	cause =  Z_ARC_V2_ECR_CODE(ecr);
	parameter = Z_ARC_V2_ECR_PARAMETER(ecr);

	/* exception raised by kernel */
	if (vector == ARC_EV_TRAP && parameter == _TRAP_S_CALL_RUNTIME_EXCEPT) {
		z_NanoFatalErrorHandler(esf->r0, esf);
		return;
	}

	printk("***** Exception vector: 0x%x, cause code: 0x%x, parameter 0x%x\n",
	       vector, cause, parameter);
	printk("Address 0x%x\n", exc_addr);
#ifdef CONFIG_ARC_EXCEPTION_DEBUG
	dump_exception_info(vector, cause, parameter);
#endif

#ifdef CONFIG_ARC_STACK_CHECKING
	/* Vector 6 = EV_ProV. Regardless of cause, parameter 2 means stack
	 * check violation
	 * stack check and mpu violation can come out together, then
	 * parameter = 0x2 | [0x4 | 0x8 | 0x1]
	 */
	if (vector == ARC_EV_PROT_V && parameter & 0x2) {
		z_NanoFatalErrorHandler(_NANO_ERR_STACK_CHK_FAIL, esf);
		return;
	}
#endif

#ifdef CONFIG_MPU_STACK_GUARD
	if (vector == ARC_EV_PROT_V && ((parameter == 0x4) ||
					(parameter == 0x24))) {
		if (z_check_thread_stack_fail(exc_addr, arc_exc_saved_sp)) {
			z_NanoFatalErrorHandler(_NANO_ERR_STACK_CHK_FAIL, esf);
			return;
		}
	}
#endif
	z_NanoFatalErrorHandler(_NANO_ERR_HW_EXCEPTION, esf);
}
