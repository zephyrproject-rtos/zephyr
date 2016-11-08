/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
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

/**
 * @file
 * @brief Floating point register sharing routines
 *
 * This module allows multiple preemptible threads to safely share the system's
 * floating point registers, by allowing the system to save FPU state info
 * in a thread's stack region when a preemptive context switch occurs.
 *
 * Note: If the kernel has been built without floating point register sharing
 * support (CONFIG_FP_SHARING), the floating point registers can still be used
 * safely by one or more cooperative threads OR by a single preemptive thread,
 * but not by both.
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

#include <kernel_structs.h>
#include <toolchain.h>
#include <asm_inline.h>

/* the entire library vanishes without the FP_SHARING option enabled */

#ifdef CONFIG_FP_SHARING

/* SSE control/status register default value (used by assembler code) */
extern uint32_t _sse_mxcsr_default_value;

/**
 *
 * @brief Save a thread's floating point context information.
 *
 * This routine saves the system's "live" floating point context into the
 * specified thread control block. The SSE registers are saved only if the
 * thread is actually using them.
 *
 * @param tcs Pointer to thread control block.
 *
 * @return N/A
 */
static void _FpCtxSave(struct tcs *tcs)
{
#ifdef CONFIG_SSE
	if (tcs->base.flags & K_SSE_REGS) {
		_do_fp_and_sse_regs_save(&tcs->arch.preempFloatReg);
		return;
	}
#endif
	_do_fp_regs_save(&tcs->arch.preempFloatReg);
}

/**
 *
 * @brief Initialize a thread's floating point context information.
 *
 * This routine initializes the system's "live" floating point context.
 * The SSE registers are initialized only if the thread is actually using them.
 *
 * @param tcs Pointer to thread control block.
 *
 * @return N/A
 */
static inline void _FpCtxInit(struct tcs *tcs)
{
	_do_fp_regs_init();
#ifdef CONFIG_SSE
	if (tcs->base.flags & K_SSE_REGS) {
		_do_sse_regs_init();
	}
#endif
}

/**
 *
 * @brief Enable preservation of floating point context information.
 *
 * This routine informs the kernel that the specified thread (which may be
 * the current thread) will be using the floating point registers.
 * The @a options parameter indicates which floating point register sets
 * will be used by the specified thread:
 *
 *  a) K_FP_REGS  indicates x87 FPU and MMX registers only
 *  b) K_SSE_REGS indicates SSE registers (and also x87 FPU and MMX registers)
 *
 * Invoking this routine initializes the thread's floating point context info
 * to that of an FPU that has been reset. The next time the thread is scheduled
 * by _Swap() it will either inherit an FPU that is guaranteed to be in a "sane"
 * state (if the most recent user of the FPU was cooperatively swapped out)
 * or the thread's own floating point context will be loaded (if the most
 * recent user of the FPU was pre-empted, or if this thread is the first user
 * of the FPU). Thereafter, the kernel will protect the thread's FP context
 * so that it is not altered during a preemptive context switch.
 *
 * @warning
 * This routine should only be used to enable floating point support for a
 * thread that does not currently have such support enabled already.
 *
 * @param tcs Pointer to thread control block.
 * @param options Registers to be preserved (K_FP_REGS or K_SSE_REGS).
 *
 * @return N/A
 *
 * @internal
 * The transition from "non-FP supporting" to "FP supporting" must be done
 * atomically to avoid confusing the floating point logic used by _Swap(), so
 * this routine locks interrupts to ensure that a context switch does not occur.
 * The locking isn't really needed when the routine is called by a cooperative
 * thread (since context switching can't occur), but it is harmless.
 */
void k_float_enable(struct tcs *tcs, unsigned int options)
{
	unsigned int imask;
	struct tcs *fp_owner;

	/* Ensure a preemptive context switch does not occur */

	imask = irq_lock();

	/* Indicate thread requires floating point context saving */

	tcs->base.flags |= options;

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
	if (fp_owner) {
		if (fp_owner->base.flags & INT_OR_EXC_MASK) {
			_FpCtxSave(fp_owner);
		}
	}

	/* Now create a virgin FP context */

	_FpCtxInit(tcs);

	/* Associate the new FP context with the specified thread */

	if (tcs == _current) {
		/*
		 * When enabling FP support for the current thread, just claim
		 * ownership of the FPU and leave CR0[TS] unset.
		 *
		 * (The FP context is "live" in hardware, not saved in TCS.)
		 */

		_kernel.current_fp = tcs;
	} else {
		/*
		 * When enabling FP support for someone else, assign ownership
		 * of the FPU to them (unless we need it ourselves).
		 */

		if ((_current->base.flags & _FP_USER_MASK) == 0) {
			/*
			 * We are not FP-capable, so mark FPU as owned by the
			 * thread we've just enabled FP support for, then
			 * disable our own FP access by setting CR0[TS] back
			 * to its original state.
			 */

			_kernel.current_fp = tcs;
			_FpAccessDisable();
		} else {
			/*
			 * We are FP-capable (and thus had FPU ownership on
			 * entry), so save the new FP context in their TCS,
			 * leave FPU ownership with self, and leave CR0[TS]
			 * unset.
			 *
			 * The saved FP context is needed in case the thread
			 * we enabled FP support for is currently pre-empted,
			 * since _Swap() uses it to restore FP context when
			 * the thread re-activates.
			 *
			 * Saving the FP context reinits the FPU, and thus
			 * our own FP context, but that's OK since it didn't
			 * need to be preserved. (i.e. We aren't currently
			 * handling an interrupt or exception.)
			 */

			_FpCtxSave(tcs);
		}
	}

	irq_unlock(imask);
}

/**
 *
 * @brief Disable preservation of floating point context information.
 *
 * This routine informs the kernel that the specified thread (which may be
 * the current thread) will no longer be using the floating point registers.
 *
 * @warning
 * This routine should only be used to disable floating point support for
 * a thread that currently has such support enabled.
 *
 * @param tcs Pointer to thread control block.
 *
 * @return N/A
 *
 * @internal
 * The transition from "FP supporting" to "non-FP supporting" must be done
 * atomically to avoid confusing the floating point logic used by _Swap(), so
 * this routine locks interrupts to ensure that a context switch does not occur.
 * The locking isn't really needed when the routine is called by a cooperative
 * thread (since context switching can't occur), but it is harmless.
 */
void k_float_disable(struct tcs *tcs)
{
	unsigned int imask;

	/* Ensure a preemptive context switch does not occur */

	imask = irq_lock();

	/* Disable all floating point capabilities for the thread */

	tcs->base.flags &= ~_FP_USER_MASK;

	if (tcs == _current) {
		_FpAccessDisable();
		_kernel.current_fp = (struct tcs *)0;
	} else {
		if (_kernel.current_fp == tcs)
			_kernel.current_fp = (struct tcs *)0;
	}

	irq_unlock(imask);
}

/**
 *
 * @brief Handler for "device not available" exception.
 *
 * This routine is registered to handle the "device not available" exception
 * (vector = 7).
 *
 * The processor will generate this exception if any x87 FPU, MMX, or SSEx
 * instruction is executed while CR0[TS]=1. The handler then enables the
 * current thread to use all supported floating point registers.
 *
 * @param pEsf This value is not used.
 *
 * @return N/A
 */
void _FpNotAvailableExcHandler(NANO_ESF *pEsf)
{
	ARG_UNUSED(pEsf);

	/*
	 * Assume the exception did not occur in an ISR.
	 * (In other words, CPU cycles will not be consumed to perform
	 * error checking to ensure the exception was not generated in an ISR.)
	 */

	PRINTK("_FpNotAvailableExcHandler() exception handler has been "
	       "invoked\n");

	/* Enable highest level of FP capability configured into the kernel */

	k_float_enable(_current, _FP_USER_MASK);
}
_EXCEPTION_CONNECT_NOCODE(_FpNotAvailableExcHandler, IV_DEVICE_NOT_AVAILABLE);

#endif /* CONFIG_FP_SHARING */
