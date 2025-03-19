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

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <inttypes.h>

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/common/exc_handle.h>
#include <zephyr/logging/log.h>
#include <err_dump_handling.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_USERSPACE
Z_EXC_DECLARE(z_arc_user_string_nlen);

static const struct z_exc_handle exceptions[] = {
	Z_EXC_HANDLE(z_arc_user_string_nlen)
};
#endif

#if defined(CONFIG_MPU_STACK_GUARD)
/**
 * @brief Assess occurrence of current thread's stack corruption
 *
 * This function performs an assessment whether a memory fault (on a given
 * memory address) is the result of a stack overflow of the current thread.
 *
 * When called, we know at this point that we received an ARC
 * protection violation, with any cause code, with the protection access
 * error either "MPU" or "Secure MPU". In other words, an MPU fault of
 * some kind. Need to determine whether this is a general MPU access
 * exception or the specific case of a stack overflow.
 *
 * @param fault_addr memory address on which memory access violation
 *                   has been reported.
 * @param sp stack pointer when exception comes out
 * @retval True if this appears to be a stack overflow
 * @retval False if this does not appear to be a stack overflow
 */
static bool z_check_thread_stack_fail(const uint32_t fault_addr, uint32_t sp)
{
#if defined(CONFIG_MULTITHREADING)
	uint32_t guard_end, guard_start;
	const struct k_thread *thread = _current;

	if (!thread) {
		/* TODO: Under what circumstances could we get here ? */
		return false;
	}

#ifdef CONFIG_USERSPACE
	if ((thread->base.user_options & K_USER) != 0) {
		if ((z_arc_v2_aux_reg_read(_ARC_V2_ERSTATUS) &
		     _ARC_V2_STATUS32_U) != 0) {
			/* Normal user mode context. There is no specific
			 * "guard" installed in this case, instead what's
			 * happening is that the stack pointer is crashing
			 * into the privilege mode stack buffer which
			 * immediately precedes it.
			 */
			guard_end = thread->stack_info.start;
			guard_start = (uint32_t)thread->stack_obj;
		} else {
			/* Special case: handling a syscall on privilege stack.
			 * There is guard memory reserved immediately before
			 * it.
			 */
			guard_end = thread->arch.priv_stack_start;
			guard_start = guard_end - Z_ARC_STACK_GUARD_SIZE;
		}
	} else
#endif /* CONFIG_USERSPACE */
	{
		/* Supervisor thread */
		guard_end = thread->stack_info.start;
		guard_start = guard_end - Z_ARC_STACK_GUARD_SIZE;
	}

	 /* treat any MPU exceptions within the guard region as a stack
	  * overflow.As some instrustions
	  * (like enter_s {r13-r26, fp, blink}) push a collection of
	  * registers on to the stack. In this situation, the fault_addr
	  * will less than guard_end, but sp will greater than guard_end.
	  */
	if (fault_addr < guard_end && fault_addr >= guard_start) {
		return true;
	}
#endif /* CONFIG_MULTITHREADING */

	return false;
}
#endif

#ifdef CONFIG_EXCEPTION_DEBUG
/* For EV_ProtV, the numbering/semantics of the parameter are consistent across
 * several codes, although not all combination will be reported.
 *
 * These codes and parameters do not have associated* names in
 * the technical manual, just switch on the values in Table 6-5
 */
static const char *get_protv_access_err(uint32_t parameter)
{
	switch (parameter) {
	case 0x1:
		return "code protection scheme";
	case 0x2:
		return "stack checking scheme";
	case 0x4:
		return "MPU";
	case 0x8:
		return "MMU";
	case 0x10:
		return "NVM";
	case 0x24:
		return "Secure MPU";
	case 0x44:
		return "Secure MPU with SID mismatch";
	default:
		return "unknown";
	}
}

static void dump_protv_exception(uint32_t cause, uint32_t parameter)
{
	switch (cause) {
	case 0x0:
		ARC_EXCEPTION_DUMP("Instruction fetch violation (%s)",
			get_protv_access_err(parameter));
		break;
	case 0x1:
		ARC_EXCEPTION_DUMP("Memory read protection violation (%s)",
			get_protv_access_err(parameter));
		break;
	case 0x2:
		ARC_EXCEPTION_DUMP("Memory write protection violation (%s)",
			get_protv_access_err(parameter));
		break;
	case 0x3:
		ARC_EXCEPTION_DUMP("Memory read-modify-write violation (%s)",
			get_protv_access_err(parameter));
		break;
	case 0x10:
		ARC_EXCEPTION_DUMP("Normal vector table in secure memory");
		break;
	case 0x11:
		ARC_EXCEPTION_DUMP("NS handler code located in S memory");
		break;
	case 0x12:
		ARC_EXCEPTION_DUMP("NSC Table Range Violation");
		break;
	default:
		ARC_EXCEPTION_DUMP("unknown");
		break;
	}
}

static void dump_machine_check_exception(uint32_t cause, uint32_t parameter)
{
	switch (cause) {
	case 0x0:
		ARC_EXCEPTION_DUMP("double fault");
		break;
	case 0x1:
		ARC_EXCEPTION_DUMP("overlapping TLB entries");
		break;
	case 0x2:
		ARC_EXCEPTION_DUMP("fatal TLB error");
		break;
	case 0x3:
		ARC_EXCEPTION_DUMP("fatal cache error");
		break;
	case 0x4:
		ARC_EXCEPTION_DUMP("internal memory error on instruction fetch");
		break;
	case 0x5:
		ARC_EXCEPTION_DUMP("internal memory error on data fetch");
		break;
	case 0x6:
		ARC_EXCEPTION_DUMP("illegal overlapping MPU entries");
		if (parameter == 0x1) {
			ARC_EXCEPTION_DUMP(" - jump and branch target");
		}
		break;
	case 0x10:
		ARC_EXCEPTION_DUMP("secure vector table not located in secure memory");
		break;
	case 0x11:
		ARC_EXCEPTION_DUMP("NSC jump table not located in secure memory");
		break;
	case 0x12:
		ARC_EXCEPTION_DUMP("secure handler code not located in secure memory");
		break;
	case 0x13:
		ARC_EXCEPTION_DUMP("NSC target address not located in secure memory");
		break;
	case 0x80:
		ARC_EXCEPTION_DUMP("uncorrectable ECC or parity error in vector memory");
		break;
	default:
		ARC_EXCEPTION_DUMP("unknown");
		break;
	}
}

static void dump_privilege_exception(uint32_t cause, uint32_t parameter)
{
	switch (cause) {
	case 0x0:
		ARC_EXCEPTION_DUMP("Privilege violation");
		break;
	case 0x1:
		ARC_EXCEPTION_DUMP("disabled extension");
		break;
	case 0x2:
		ARC_EXCEPTION_DUMP("action point hit");
		break;
	case 0x10:
		switch (parameter) {
		case 0x1:
			ARC_EXCEPTION_DUMP("N to S return using incorrect return mechanism");
			break;
		case 0x2:
			ARC_EXCEPTION_DUMP("N to S return with incorrect operating mode");
			break;
		case 0x3:
			ARC_EXCEPTION_DUMP("IRQ/exception return fetch from wrong mode");
			break;
		case 0x4:
			ARC_EXCEPTION_DUMP("attempt to halt secure processor in NS mode");
			break;
		case 0x20:
			ARC_EXCEPTION_DUMP("attempt to access secure resource from normal mode");
			break;
		case 0x40:
			ARC_EXCEPTION_DUMP("SID violation on resource access (APEX/UAUX/key NVM)");
			break;
		default:
			ARC_EXCEPTION_DUMP("unknown");
			break;
		}
		break;
	case 0x13:
		switch (parameter) {
		case 0x20:
			ARC_EXCEPTION_DUMP("attempt to access secure APEX feature from NS mode");
			break;
		case 0x40:
			ARC_EXCEPTION_DUMP("SID violation on access to APEX feature");
			break;
		default:
			ARC_EXCEPTION_DUMP("unknown");
			break;
		}
		break;
	default:
		ARC_EXCEPTION_DUMP("unknown");
		break;
	}
}

static void dump_exception_info(uint32_t vector, uint32_t cause, uint32_t parameter)
{
	if (vector >= 0x10 && vector <= 0xFF) {
		ARC_EXCEPTION_DUMP("interrupt %u", vector);
		return;
	}

	/* Names are exactly as they appear in Designware ARCv2 ISA
	 * Programmer's reference manual for easy searching
	 */
	switch (vector) {
	case ARC_EV_RESET:
		ARC_EXCEPTION_DUMP("Reset");
		break;
	case ARC_EV_MEM_ERROR:
		ARC_EXCEPTION_DUMP("Memory Error");
		break;
	case ARC_EV_INS_ERROR:
		ARC_EXCEPTION_DUMP("Instruction Error");
		break;
	case ARC_EV_MACHINE_CHECK:
		ARC_EXCEPTION_DUMP("EV_MachineCheck");
		dump_machine_check_exception(cause, parameter);
		break;
	case ARC_EV_TLB_MISS_I:
		ARC_EXCEPTION_DUMP("EV_TLBMissI");
		break;
	case ARC_EV_TLB_MISS_D:
		ARC_EXCEPTION_DUMP("EV_TLBMissD");
		break;
	case ARC_EV_PROT_V:
		ARC_EXCEPTION_DUMP("EV_ProtV");
		dump_protv_exception(cause, parameter);
		break;
	case ARC_EV_PRIVILEGE_V:
		ARC_EXCEPTION_DUMP("EV_PrivilegeV");
		dump_privilege_exception(cause, parameter);
		break;
	case ARC_EV_SWI:
		ARC_EXCEPTION_DUMP("EV_SWI");
		break;
	case ARC_EV_TRAP:
		ARC_EXCEPTION_DUMP("EV_Trap");
		break;
	case ARC_EV_EXTENSION:
		ARC_EXCEPTION_DUMP("EV_Extension");
		break;
	case ARC_EV_DIV_ZERO:
		ARC_EXCEPTION_DUMP("EV_DivZero");
		break;
	case ARC_EV_DC_ERROR:
		ARC_EXCEPTION_DUMP("EV_DCError");
		break;
	case ARC_EV_MISALIGNED:
		ARC_EXCEPTION_DUMP("EV_Misaligned");
		break;
	case ARC_EV_VEC_UNIT:
		ARC_EXCEPTION_DUMP("EV_VecUnit");
		break;
	default:
		ARC_EXCEPTION_DUMP("unknown");
		break;
	}
}
#endif /* CONFIG_EXCEPTION_DEBUG */

/*
 * @brief Fault handler
 *
 * This routine is called when fatal error conditions are detected by hardware
 * and is responsible only for reporting the error. Once reported, it then
 * invokes the user provided routine k_sys_fatal_error_handler() which is
 * responsible for implementing the error handling policy.
 */
void z_arc_fault(struct arch_esf *esf, uint32_t old_sp)
{
	uint32_t vector, cause, parameter;
	uint32_t exc_addr = z_arc_v2_aux_reg_read(_ARC_V2_EFA);
	uint32_t ecr = z_arc_v2_aux_reg_read(_ARC_V2_ECR);

#ifdef CONFIG_USERSPACE
	for (int i = 0; i < ARRAY_SIZE(exceptions); i++) {
		uint32_t start = (uint32_t)exceptions[i].start;
		uint32_t end = (uint32_t)exceptions[i].end;

		if (esf->pc >= start && esf->pc < end) {
			esf->pc = (uint32_t)(exceptions[i].fixup);
			return;
		}
	}
#endif

	vector = Z_ARC_V2_ECR_VECTOR(ecr);
	cause =  Z_ARC_V2_ECR_CODE(ecr);
	parameter = Z_ARC_V2_ECR_PARAMETER(ecr);

	/* exception raised by kernel */
	if (vector == ARC_EV_TRAP && parameter == _TRAP_S_CALL_RUNTIME_EXCEPT) {
		/*
		 * in user mode software-triggered system fatal exceptions only allow
		 * K_ERR_KERNEL_OOPS and K_ERR_STACK_CHK_FAIL
		 */
#ifdef CONFIG_USERSPACE
		if ((esf->status32 & _ARC_V2_STATUS32_U) &&
			esf->r0 != K_ERR_STACK_CHK_FAIL) {
			esf->r0 = K_ERR_KERNEL_OOPS;
		}
#endif

		z_arc_fatal_error(esf->r0, esf);
		return;
	}

#ifdef CONFIG_EXCEPTION_DEBUG
	ARC_EXCEPTION_DUMP("***** Exception vector: 0x%x, cause code: 0x%x, parameter 0x%x",
		vector, cause, parameter);
	ARC_EXCEPTION_DUMP("Address 0x%x", exc_addr);

	dump_exception_info(vector, cause, parameter);
#endif

#ifdef CONFIG_ARC_STACK_CHECKING
	/* Vector 6 = EV_ProV. Regardless of cause, parameter 2 means stack
	 * check violation
	 * stack check and mpu violation can come out together, then
	 * parameter = 0x2 | [0x4 | 0x8 | 0x1]
	 */
	if (vector == ARC_EV_PROT_V && parameter & 0x2) {
		z_arc_fatal_error(K_ERR_STACK_CHK_FAIL, esf);
		return;
	}
#endif

#ifdef CONFIG_MPU_STACK_GUARD
	if (vector == ARC_EV_PROT_V && ((parameter == 0x4) ||
					(parameter == 0x24))) {
		if (z_check_thread_stack_fail(exc_addr, old_sp)) {
			z_arc_fatal_error(K_ERR_STACK_CHK_FAIL, esf);
			return;
		}
	}
#endif
	z_arc_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}
