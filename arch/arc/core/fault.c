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
	u32_t vector, code, parameter;
	u32_t exc_addr = z_arc_v2_aux_reg_read(_ARC_V2_EFA);
	u32_t ecr = z_arc_v2_aux_reg_read(_ARC_V2_ECR);

	LOG_PANIC();

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

	vector = _ARC_V2_ECR_VECTOR(ecr);
	code =  _ARC_V2_ECR_CODE(ecr);
	parameter = _ARC_V2_ECR_PARAMETER(ecr);


	/* exception raised by kernel */
	if (vector == 0x9 && parameter == _TRAP_S_CALL_RUNTIME_EXCEPT) {
		z_NanoFatalErrorHandler(esf->r0, esf);
		return;
	}

	printk("Exception vector: 0x%x, cause code: 0x%x, parameter 0x%x\n",
	       vector, code, parameter);
	printk("Address 0x%x\n", exc_addr);
#ifdef CONFIG_ARC_STACK_CHECKING
	/* Vector 6 = EV_ProV. Regardless of code, parameter 2 means stack
	 * check violation
	 * stack check and mpu violation can come out together, then
	 * parameter = 0x2 | [0x4 | 0x8 | 0x1]
	 */
	if (vector == 6U && parameter & 0x2) {
		z_NanoFatalErrorHandler(_NANO_ERR_STACK_CHK_FAIL, esf);
		return;
	}
#endif

#ifdef CONFIG_MPU_STACK_GUARD
	if (vector == 6 && ((parameter == 4) || (parameter == 24))) {
		if (z_check_thread_stack_fail(exc_addr, arc_exc_saved_sp)) {
			z_NanoFatalErrorHandler(_NANO_ERR_STACK_CHK_FAIL, esf);
			return;
		}
	}
#endif
	z_NanoFatalErrorHandler(_NANO_ERR_HW_EXCEPTION, esf);
}
