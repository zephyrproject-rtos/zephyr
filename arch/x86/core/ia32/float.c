/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Floating point register sharing routines
 *
 * This module allows multiple preemptible threads to safely share the system's
 * floating point registers, by allowing the system to save FPU state info
 * in a thread's stack region when a preemptive context switch occurs.
 *
 * Note: If the kernel has been built without floating point register sharing
 * support (CONFIG_FPU_SHARING), the floating point registers can still be used
 * safely by one or more cooperative threads OR by a single preemptive thread,
 * but not by both.
 *
 * This code is not necessary for systems with CONFIG_EAGER_FPU_SHARING, as
 * the floating point context is unconditionally saved/restored with every
 * context switch.
 *
 * The floating point register sharing mechanism is designed for minimal
 * intrusiveness.  Floating point state saving is only performed for threads
 * that explicitly indicate they are using FPU registers, to avoid impacting
 * the stack size requirements of all other threads. Also, the SSE registers
 * are only saved for threads that actually used them. For those threads that
 * do require floating point state saving, a "lazy save/restore" mechanism
 * is employed so that the FPU's register sets are only switched in and out
 * when absolutely necessary; this avoids wasting effort preserving them when
 * there is no risk that they will be altered, or when there is no need to
 * preserve their contents.
 *
 * WARNING
 * The use of floating point instructions by ISRs is not supported by the
 * kernel.
 *
 * INTERNAL
 * The kernel sets CR0[TS] to 0 only for threads that require FP register
 * sharing. All other threads have CR0[TS] set to 1 so that an attempt
 * to perform an FP operation will cause an exception, allowing the kernel
 * to enable FP register sharing on its behalf.
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>

/* SSE control/status register default value (used by assembler code) */
extern uint32_t _sse_mxcsr_default_value;

/**
 * @brief Disallow use of floating point capabilities
 *
 * This routine sets CR0[TS] to 1, which disallows the use of FP instructions
 * by the currently executing thread.
 */
static inline void z_FpAccessDisable(void)
{
	void *tempReg;

	__asm__ volatile(
		"movl %%cr0, %0;\n\t"
		"orl $0x8, %0;\n\t"
		"movl %0, %%cr0;\n\t"
		: "=r"(tempReg)
		:
		: "memory");
}


/**
 * @brief Save non-integer context information
 *
 * This routine saves the system's "live" non-integer context into the
 * specified area.  If the specified thread supports SSE then
 * x87/MMX/SSEx thread info is saved, otherwise only x87/MMX thread is saved.
 * Function is invoked by FpCtxSave(struct k_thread *thread)
 */
static inline void z_do_fp_regs_save(void *preemp_float_reg)
{
	__asm__ volatile("fnsave (%0);\n\t"
			 :
			 : "r"(preemp_float_reg)
			 : "memory");
}

/**
 * @brief Save non-integer context information
 *
 * This routine saves the system's "live" non-integer context into the
 * specified area.  If the specified thread supports SSE then
 * x87/MMX/SSEx thread info is saved, otherwise only x87/MMX thread is saved.
 * Function is invoked by FpCtxSave(struct k_thread *thread)
 */
static inline void z_do_fp_and_sse_regs_save(void *preemp_float_reg)
{
	__asm__ volatile("fxsave (%0);\n\t"
			 :
			 : "r"(preemp_float_reg)
			 : "memory");
}

/**
 * @brief Initialize floating point register context information.
 *
 * This routine initializes the system's "live" floating point registers.
 */
static inline void z_do_fp_regs_init(void)
{
	__asm__ volatile("fninit\n\t");
}

/**
 * @brief Initialize SSE register context information.
 *
 * This routine initializes the system's "live" SSE registers.
 */
static inline void z_do_sse_regs_init(void)
{
	__asm__ volatile("ldmxcsr _sse_mxcsr_default_value\n\t");
}

/*
 * Save a thread's floating point context information.
 *
 * This routine saves the system's "live" floating point context into the
 * specified thread control block. The SSE registers are saved only if the
 * thread is actually using them.
 */
static void FpCtxSave(struct k_thread *thread)
{
#ifdef CONFIG_X86_SSE
	if ((thread->base.user_options & K_SSE_REGS) != 0) {
		z_do_fp_and_sse_regs_save(&thread->arch.preempFloatReg);
		return;
	}
#endif
	z_do_fp_regs_save(&thread->arch.preempFloatReg);
}

/*
 * Initialize a thread's floating point context information.
 *
 * This routine initializes the system's "live" floating point context.
 * The SSE registers are initialized only if the thread is actually using them.
 */
static inline void FpCtxInit(struct k_thread *thread)
{
	z_do_fp_regs_init();
#ifdef CONFIG_X86_SSE
	if ((thread->base.user_options & K_SSE_REGS) != 0) {
		z_do_sse_regs_init();
	}
#endif
}

/*
 * Enable preservation of floating point context information.
 *
 * The transition from "non-FP supporting" to "FP supporting" must be done
 * atomically to avoid confusing the floating point logic used by z_swap(), so
 * this routine locks interrupts to ensure that a context switch does not occur.
 * The locking isn't really needed when the routine is called by a cooperative
 * thread (since context switching can't occur), but it is harmless.
 */
void z_float_enable(struct k_thread *thread, unsigned int options)
{
	unsigned int imask;
	struct k_thread *fp_owner;

	/* Ensure a preemptive context switch does not occur */

	imask = irq_lock();

	/* Indicate thread requires floating point context saving */

	thread->base.user_options |= (uint8_t)options;

	/*
	 * The current thread might not allow FP instructions, so clear CR0[TS]
	 * so we can use them. (CR0[TS] gets restored later on, if necessary.)
	 */

	__asm__ volatile("clts\n\t");

	/*
	 * Save existing floating point context (since it is about to change),
	 * but only if the FPU is "owned" by an FP-capable task that is
	 * currently handling an interrupt or exception (meaning its FP context
	 * must be preserved).
	 */

	fp_owner = _kernel.current_fp;
	if (fp_owner != NULL) {
		if ((fp_owner->arch.flags & X86_THREAD_FLAG_ALL) != 0) {
			FpCtxSave(fp_owner);
		}
	}

	/* Now create a virgin FP context */

	FpCtxInit(thread);

	/* Associate the new FP context with the specified thread */

	if (thread == _current) {
		/*
		 * When enabling FP support for the current thread, just claim
		 * ownership of the FPU and leave CR0[TS] unset.
		 *
		 * (The FP context is "live" in hardware, not saved in TCS.)
		 */

		_kernel.current_fp = thread;
	} else {
		/*
		 * When enabling FP support for someone else, assign ownership
		 * of the FPU to them (unless we need it ourselves).
		 */

		if ((_current->base.user_options & _FP_USER_MASK) == 0) {
			/*
			 * We are not FP-capable, so mark FPU as owned by the
			 * thread we've just enabled FP support for, then
			 * disable our own FP access by setting CR0[TS] back
			 * to its original state.
			 */

			_kernel.current_fp = thread;
			z_FpAccessDisable();
		} else {
			/*
			 * We are FP-capable (and thus had FPU ownership on
			 * entry), so save the new FP context in their TCS,
			 * leave FPU ownership with self, and leave CR0[TS]
			 * unset.
			 *
			 * The saved FP context is needed in case the thread
			 * we enabled FP support for is currently pre-empted,
			 * since z_swap() uses it to restore FP context when
			 * the thread re-activates.
			 *
			 * Saving the FP context reinits the FPU, and thus
			 * our own FP context, but that's OK since it didn't
			 * need to be preserved. (i.e. We aren't currently
			 * handling an interrupt or exception.)
			 */

			FpCtxSave(thread);
		}
	}

	irq_unlock(imask);
}

/**
 * Disable preservation of floating point context information.
 *
 * The transition from "FP supporting" to "non-FP supporting" must be done
 * atomically to avoid confusing the floating point logic used by z_swap(), so
 * this routine locks interrupts to ensure that a context switch does not occur.
 * The locking isn't really needed when the routine is called by a cooperative
 * thread (since context switching can't occur), but it is harmless.
 */
int z_float_disable(struct k_thread *thread)
{
	unsigned int imask;

	/* Ensure a preemptive context switch does not occur */

	imask = irq_lock();

	/* Disable all floating point capabilities for the thread */

	thread->base.user_options &= ~_FP_USER_MASK;

	if (thread == _current) {
		z_FpAccessDisable();
		_kernel.current_fp = (struct k_thread *)0;
	} else {
		if (_kernel.current_fp == thread) {
			_kernel.current_fp = (struct k_thread *)0;
		}
	}

	irq_unlock(imask);

	return 0;
}

/*
 * Handler for "device not available" exception.
 *
 * This routine is registered to handle the "device not available" exception
 * (vector = 7).
 *
 * The processor will generate this exception if any x87 FPU, MMX, or SSEx
 * instruction is executed while CR0[TS]=1. The handler then enables the
 * current thread to use all supported floating point registers.
 */
void _FpNotAvailableExcHandler(z_arch_esf_t *pEsf)
{
	ARG_UNUSED(pEsf);

	/*
	 * Assume the exception did not occur in an ISR.
	 * (In other words, CPU cycles will not be consumed to perform
	 * error checking to ensure the exception was not generated in an ISR.)
	 */

	/* Enable highest level of FP capability configured into the kernel */

	k_float_enable(_current, _FP_USER_MASK);
}
_EXCEPTION_CONNECT_NOCODE(_FpNotAvailableExcHandler,
		IV_DEVICE_NOT_AVAILABLE, 0);
