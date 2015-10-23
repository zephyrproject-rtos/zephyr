/* float.c - floating point resource sharing routines */

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

/*
 * DESCRIPTION
 * This module allows multiple tasks and fibers to safely share the system's
 * floating point resources, by allowing the system to save FPU state
 * information in a task or fiber's stack region when a pre-emptive context
 * switch occurs.
 *
 * The floating point resource sharing mechanism is designed for minimal
 * intrusiveness.  Floating point thread saving is only performed for tasks and
 * fibers that explicitly enable FP resource sharing, to avoid impacting the
 * stack size requirements of all other tasks and fibers.  For those tasks and
 * fibers that do require FP resource sharing, a "lazy save/restore" mechanism
 * is employed so that the FPU's register sets are only switched in and out
 * when absolutely necessary; this avoids wasting effort preserving them when
 * there is no risk that they will be altered, or when there is no need to
 * preserve their contents.
 *
 * The following APIs are provided to allow floating point resource sharing to
 * be enabled or disabled at run-time:
 *
 * void fiber_float_enable  (nano_thread_id_t thread_id, unsigned int options)
 * void task_float_enable   (nano_thread_id_t thread_id, unsigned int options)
 * void fiber_float_disable (nano_thread_id_t thread_id)
 * void task_float_disable  (nano_thread_id_t thread_id)
 *
 * The 'options' parameter is used to specify what non-integer capabilities are
 * being used.  The same options accepted by fiber_fiber_start() are used in the
 * aforementioned APIs, namely USE_FP and USE_SSE.
 *
 * If the nanokernel has been built without SSE instruction support
 * (CONFIG_SSE), the system treats USE_SSE as if it was USE_FP.
 *
 * If the nanokernel has been built without floating point resource sharing
 * support (CONFIG_FP_SHARING), the aforementioned APIs and capabilities do not
 * exist.
 *
 * NOTE
 * It is possible for a single task or fiber to utilize floating instructions
 * _without_ enabling the FP resource sharing feature.  Since no other task or
 * fiber uses the FPU the FP registers won't change when the FP-capable task or
 * fiber isn't executing, meaning there is no need to save the registers.
 *
 * WARNING
 * The use of floating point instructions by ISRs is not supported by the
 * kernel.
 *
 * INTERNAL
 * If automatic enabling of floating point resource sharing _is not_ configured
 * the system leaves CR0[TS] = 0 for all tasks and fibers.  This means that any
 * task or fiber can perform floating point operations at any time without
 * causing an exception, and the system won't stop a task or fiber that
 * shouldn't be doing FP stuff from doing it.
 *
 * If automatic enabling of floating point resource sharing _is_ configured
 * the system leaves CR0[TS] = 0 only for tasks and fibers that are allowed to
 * perform FP operations.  All other tasks and fibers have CR0[TS] = 1 so that
 * an attempt to perform an FP operation will cause an exception, allowing the
 * system to enable FP resource sharing on its behalf.
 */

#ifdef CONFIG_MICROKERNEL
#include <microkernel.h>
#include <micro_private_types.h>
#endif /* CONFIG_MICROKERNEL */

#include <nano_private.h>
#include <toolchain.h>
#include <asm_inline.h>

/* the entire library vanishes without the FP_SHARING option enabled */

#ifdef CONFIG_FP_SHARING

#if defined(CONFIG_SSE)
extern uint32_t _sse_mxcsr_default_value; /* SSE control/status register default value */
#endif			/* CONFIG_SSE */

/**
 *
 * @brief Save non-integer context information
 *
 * This routine saves the system's "live" non-integer context into the
 * specified TCS.  If the specified task or fiber supports SSE then
 * x87/MMX/SSEx thread info is saved, otherwise only x87/MMX thread is saved.
 *
 * @param tcs TBD
 *
 * @return N/A
 */
static void _FpCtxSave(struct tcs *tcs)
{
	_do_fp_ctx_save(tcs->flags & USE_SSE, &tcs->preempFloatReg);
}

/**
 *
 * @brief Initialize non-integer context information
 *
 * This routine initializes the system's "live" non-integer context.
 *
 * @param tcs TBD
 *
 * @return N/A
 */
static inline void _FpCtxInit(struct tcs *tcs)
{
	_do_fp_ctx_init(tcs->flags & USE_SSE);
}

/**
 *
 * @brief Enable preservation of non-integer context information
 *
 * This routine allows the specified task/fiber (which may be the active
 * task/fiber) to safely share the system's floating point registers with
 * other tasks/fibers.  The <options> parameter indicates which floating point
 * register sets will be used by the specified task/fiber:
 *
 *  a) USE_FP  indicates x87 FPU and MMX registers only
 *  b) USE_SSE indicates x87 FPU and MMX and SSEx registers
 *
 * Invoking this routine creates a floating point thread for the task/fiber
 * that corresponds to an FPU that has been reset.  The system will thereafter
 * protect the task/fiber's FP context so that it is not altered during
 * a pre-emptive context switch.
 *
 * WARNING
 * This routine should only be used to enable floating point support for a
 * task/fiber that does not currently have such support enabled already.
 *
 * @param tcs  TDB
 * @param options set to either USE_FP or USE_SSE
 *
 * @return N/A
 *
 * INTERNAL
 * Since the transition from "non-FP supporting" to "FP supporting" must be done
 * atomically to avoid confusing the floating point logic used by _Swap(),
 * this routine locks interrupts to ensure that a context switch does not occur,
 * The locking isn't really needed when the routine is called by a fiber
 * (since context switching can't occur), but it is harmless and allows a single
 * routine to be called by both tasks and fibers (thus saving code space).
 *
 * If necessary, the interrupt latency impact of calling this routine from a
 * fiber could be lessened by re-designing things so that only task-type callers
 * locked interrupts (i.e. move the locking to task_float_enable()). However,
 * all calls to fiber_float_enable() would need to be reviewed to ensure they
 * are only used from a fiber, rather than from "generic" code used by both
 * tasks and fibers.
 */
void _FpEnable(struct tcs *tcs, unsigned int options)
{
	unsigned int imask;
	struct tcs *fp_owner;

	/* Lock interrupts to prevent a pre-emptive context switch from occuring
	 */

	imask = irq_lock();

	/* Indicate task/fiber requires non-integer context saving */

	tcs->flags |= options | USE_FP; /* USE_FP is treated as a "dirty bit" */

	/*
	 * Current task/fiber might not allow FP instructions, so clear CR0[TS]
	 * so we can use them. (CR0[TS] gets restored later on, if necessary.)
	 */

	__asm__ volatile("clts\n\t");

	/*
	 * Save the existing non-integer context (since it is about to change),
	 * but only if the FPU is "owned" by an FP-capable task that is
	 * currently
	 * handling an interrupt or exception (meaning it's FP context must be
	 * preserved).
	 */

	fp_owner = _nanokernel.current_fp;
	if (fp_owner) {
		if (fp_owner->flags & INT_OR_EXC_MASK) {
			_FpCtxSave(fp_owner);
		}
	}

	/* Now create a virgin FP context */

	_FpCtxInit(tcs);

	/* Associate the new FP context with the specified task/fiber */

	if (tcs == _nanokernel.current) {
		/*
		 * When enabling FP support for self, just claim ownership of
		 *the FPU
		 * and leave CR0[TS] unset.
		 *
		 * (Note: the FP context is "live" in hardware, not saved in TCS.)
		 */

		_nanokernel.current_fp = tcs;
	} else {
		/*
		 * When enabling FP support for someone else, assign ownership
		 * of the FPU to them (unless we need it ourselves).
		 */

		if ((_nanokernel.current->flags & USE_FP) != USE_FP) {
			/*
			 * We are not FP-capable, so mark FPU as owned by the
			 * thread
			 * we've just enabled FP support for, then disable our
			 * own
			 * FP access by setting CR0[TS] to its original state.
			 */

			_nanokernel.current_fp = tcs;
			_FpAccessDisable();
		} else {
			/*
			 * We are FP-capable (and thus had FPU ownership on
			 *entry), so save
			 * the new FP context in their TCS, leave FPU ownership
			 *with self,
			 * and leave CR0[TS] unset.
			 *
			 * Note: The saved FP context is needed in case the task
			 *or fiber
			 * we enabled FP support for is currently pre-empted,
			 *since _Swap()
			 * uses it to restore FP context when the task/fiber
			 *re-activates.
			 *
			 * Note: Saving the FP context reinits the FPU, and thus
			 *our own
			 * FP context, but that's OK since it didn't need to be
			 *preserved.
			 * (i.e. We aren't currently handling an interrupt or
			 *exception.)
			 */

			_FpCtxSave(tcs);
		}
	}

	irq_unlock(imask);
}

/**
 *
 * @brief Enable preservation of non-integer context information
 *
 * This routine allows a fiber to permit a task/fiber (including itself) to
 * safely share the system's floating point registers with other tasks/fibers.
 *
 * See the description of _FpEnable() for further details.
 *
 * @return N/A
 */
FUNC_ALIAS(_FpEnable, fiber_float_enable, void);

/**
 *
 * @brief Enable preservation of non-integer context information
 *
 * This routine allows a task to permit a task/fiber (including itself) to
 * safely share the system's floating point registers with other tasks/fibers.
 *
 * See the description of _FpEnable() for further details.
 *
 * @return N/A
 */
FUNC_ALIAS(_FpEnable, task_float_enable, void);

/**
 *
 * @brief Disable preservation of non-integer context information
 *
 * This routine prevents the specified task/fiber (which may be the active
 * task/fiber) from safely sharing any of the system's floating point registers
 * with other tasks/fibers.
 *
 * WARNING
 * This routine should only be used to disable floating point support for
 * a task/fiber that currently has such support enabled.
 *
 * @param tcs TBD
 *
 * @return N/A
 *
 * INTERNAL
 * Since the transition from "FP supporting" to "non-FP supporting" must be done
 * atomically to avoid confusing the floating point logic used by _Swap(),
 * this routine locks interrupts to ensure that a context switch does not occur,
 * The locking isn't really needed when the routine is called by a fiber
 * (since context switching can't occur), but it is harmless and allows a single
 * routine to be called by both tasks and fibers (thus saving code space).
 *
 * If necessary, the interrupt latency impact of calling this routine from a
 * fiber could be lessened by re-designing things so that only task-type callers
 * locked interrupts (i.e. move the locking to task_float_disable()). However,
 * all calls to fiber_float_disable() would need to be reviewed to ensure they
 * are only used from a fiber, rather than from "generic" code used by both
 * tasks and fibers.
 */
void _FpDisable(struct tcs *tcs)
{
	unsigned int imask;

	/* Lock interrupts to prevent a pre-emptive context switch from occuring
	 */

	imask = irq_lock();

	/*
	 * Disable _all_ floating point capabilities for the task/fiber,
	 * regardless
	 * of the options specified at the time support was enabled.
	 */

	tcs->flags &= ~(USE_FP | USE_SSE);

	if (tcs == _nanokernel.current) {
		_FpAccessDisable();
		_nanokernel.current_fp = (struct tcs *)0;
	} else {
		if (_nanokernel.current_fp == tcs)
			_nanokernel.current_fp = (struct tcs *)0;
	}

	irq_unlock(imask);
}

/**
 *
 * @brief Disable preservation of non-integer context
 *information
 *
 * This routine allows a fiber to disallow a task/fiber (including itself) from
 * safely sharing any of the system's floating point registers with other
 * tasks/fibers.
 *
 * WARNING
 * This routine should only be used to disable floating point support for
 * a task/fiber that currently has such support enabled.
 *
 * @return N/A
 */
FUNC_ALIAS(_FpDisable, fiber_float_disable, void);

/**
 *
 * @brief Disable preservation of non-integer context information
 *
 * This routine allows a task to disallow a task/fiber (including itself) from
 * safely sharing any of the system's floating point registers with other
 * tasks/fibers.
 *
 * WARNING
 * This routine should only be used to disable floating point support for
 * a task/fiber that currently has such support enabled.
 *
 * @return N/A
 */
FUNC_ALIAS(_FpDisable, task_float_disable, void);


/**
 *
 * @brief Handler for "device not available" exception
 *
 * This routine is registered to handle the "device not available" exception
 * (vector = 7)
 *
 * The processor will generate this exception if any x87 FPU, MMX, or SSEx
 * instruction is executed while CR0[TS]=1.  The handler then enables the
 * current task or fiber with the USE_FP option (or the USE_SSE option if the
 * SSE configuration option has been enabled).
 *
 * @param pEsf this value is not used for this architecture
 *
 * @return N/A
 */
void _FpNotAvailableExcHandler(NANO_ESF * pEsf)
{
	unsigned int enableOption;

	ARG_UNUSED(pEsf);

	/*
	 * Assume the exception did not occur in the thread of an ISR.
	 * (In other words, CPU cycles will not be consumed to perform
	 * error checking to ensure the exception was not generated in an ISR.)
	 */

	PRINTK("_FpNotAvailableExcHandler() exception handler has been "
	       "invoked\n");

/* Enable the highest level of FP capability configured into the kernel */

#ifdef CONFIG_SSE
	enableOption = USE_SSE;
#else
	enableOption = USE_FP;
#endif

	_FpEnable(_nanokernel.current, enableOption);
}

#endif /* CONFIG_FP_SHARING */
