/* float.c - floating point resource sharing routines */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module allows multiple tasks and fibers to safely share the system's
floating point resources, by allowing the system to save FPU state information
in a task or fiber's stack region when a pre-emptive context switch occurs.

The floating point resource sharing mechanism is designed for minimal
intrusiveness.  Floating point context saving is only performed for tasks and
fibers that explicitly enable FP resource sharing, to avoid impacting the stack
size requirements of all other tasks and fibers.  For those tasks and fibers
that do require FP resource sharing, a "lazy save/restore" mechanism is employed
so that the FPU's register sets are only switched in and out when absolutely
necessary; this avoids wasting effort preserving them when there is no risk
that they will be altered, or when there is no need to preserve their contents.

The following APIs are provided to allow floating point resource sharing to be
enabled or disabled at run-time:

	void fiber_float_enable  (nano_context_id_t ctxId, unsigned int options)
	void task_float_enable   (nano_context_id_t ctxId, unsigned int options)
	void fiber_float_disable (nano_context_id_t ctxId)
	void task_float_disable  (nano_context_id_t ctxId)

The 'options' parameter is used to specify what non-integer capabilities are
being used.  The same options accepted by fiber_fiber_start() are used in the
aforementioned APIs, namely USE_FP and USE_SSE.

If the nanokernel has been built with support for automatic enabling
of floating point resource sharing (CONFIG_AUTOMATIC_FP_ENABLING) the
system automatically enables sharing for any non-enabled task or fiber as soon
as it begins using floating point instructions.  (Note: The task or fiber must
have enough room available on its stack to allow floating point state info
to be saved, otherwise stack corruption will occur.)

If the nanokernel has been built without SSE instruction support
(CONFIG_SSE), the system treats USE_SSE as if it was USE_FP.

If the nanokernel has been built without floating point resource
sharing support (CONFIG_FP_SHARING), the aforementioned APIs and
capabilities do not exist.

NOTE
It is possible for a single task or fiber to utilize floating instructions
_without_ enabling the FP resource sharing feature.  Since no other task or
fiber uses the FPU the FP registers won't change when the FP-capable task or
fiber isn't executing, meaning there is no need to save the registers.

WARNING
The use of floating point instructions by ISRs is not supported by the kernel.

INTERNAL
If automatic enabling of floating point resource sharing _is not_ configured
the system leaves CR0[TS] = 0 for all tasks and fibers.  This means that any
task or fiber can perform floating point operations at any time without causing
an exception, and the system won't stop a task or fiber that shouldn't be
doing FP stuff from doing it.

If automatic enabling of floating point resource sharing _is_ configured the
system leaves CR0[TS] = 0 only for tasks and fibers that are allowed to perform
FP operations.  All other tasks and fibers have CR0[TS] = 1 so that an attempt
to perform an FP operation will cause an exception, allowing the system to
enable FP resource sharing on its behalf.

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

/*******************************************************************************
*
* _FpCtxSave - save non-integer context information
*
* This routine saves the system's "live" non-integer context into the
* specified CCS.  If the specified task or fiber supports SSE then
* x87/MMX/SSEx context info is saved, otherwise only x87/MMX context is saved.
*
* RETURNS: N/A
*/

static void _FpCtxSave(tCCS *ccs)
{
	_do_fp_ctx_save(ccs->flags & USE_SSE, &ccs->preempFloatReg);
}

/*******************************************************************************
*
* _FpCtxInit - initialize non-integer context information
*
* This routine initializes the system's "live" non-integer context.
*
* RETURNS: N/A
*/

static inline void _FpCtxInit(tCCS *ccs)
{
	_do_fp_ctx_init(ccs->flags & USE_SSE);
}

/*******************************************************************************
*
* _FpEnable - enable preservation of non-integer context information
*
* This routine allows the specified task/fiber (which may be the active
* task/fiber) to safely share the system's floating point registers with
* other tasks/fibers.  The <options> parameter indicates which floating point
* register sets will be used by the specified task/fiber:
*
*  a) USE_FP  indicates x87 FPU and MMX registers only
*  b) USE_SSE indicates x87 FPU and MMX and SSEx registers
*
* Invoking this routine creates a floating point context for the task/fiber
* that corresponds to an FPU that has been reset.  The system will thereafter
* protect the task/fiber's FP context so that it is not altered during
* a pre-emptive context switch.
*
* WARNING
* This routine should only be used to enable floating point support for a
* task/fiber that does not currently have such support enabled already.
*
* RETURNS: N/A
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

void _FpEnable(tCCS *ccs,
			     unsigned int options /* USE_FP or USE_SSE */
			     )
{
	unsigned int imask;
	tCCS *fp_owner;

	/* Lock interrupts to prevent a pre-emptive context switch from occuring
	 */

	imask = irq_lock_inline();

	/* Indicate task/fiber requires non-integer context saving */

	ccs->flags |= options | USE_FP; /* USE_FP is treated as a "dirty bit" */

#ifdef CONFIG_AUTOMATIC_FP_ENABLING
	/*
	 * Current task/fiber might not allow FP instructions, so clear CR0[TS]
	 * so we can use them. (CR0[TS] gets restored later on, if necessary.)
	 */

	__asm__ volatile("clts\n\t");
#endif /* CONFIG_AUTOMATIC_FP_ENABLING */

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

	_FpCtxInit(ccs);

	/* Associate the new FP context with the specified task/fiber */

	if (ccs == _nanokernel.current) {
		/*
		 * When enabling FP support for self, just claim ownership of
		 *the FPU
		 * and leave CR0[TS] unset.
		 *
		 * (Note: the FP context is "live" in hardware, not saved in
		 *CCS.)
		 */

		_nanokernel.current_fp = ccs;
	} else {
		/*
		 * When enabling FP support for someone else, assign ownership
		 * of the FPU to them (unless we need it ourselves).
		 */

		if ((_nanokernel.current->flags & USE_FP) != USE_FP) {
			/*
			 * We are not FP-capable, so mark FPU as owned by the
			 * context
			 * we've just enabled FP support for, then disable our
			 * own
			 * FP access by setting CR0[TS] to its original state.
			 */

			_nanokernel.current_fp = ccs;
#ifdef CONFIG_AUTOMATIC_FP_ENABLING
			_FpAccessDisable();
#endif /* CONFIG_AUTOMATIC_FP_ENABLING */
		} else {
			/*
			 * We are FP-capable (and thus had FPU ownership on
			 *entry), so save
			 * the new FP context in their CCS, leave FPU ownership
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

			_FpCtxSave(ccs);
		}
	}

	irq_unlock_inline(imask);
}

/*******************************************************************************
*
* fiber_float_enable - enable preservation of non-integer context information
*
* This routine allows a fiber to permit a task/fiber (including itself) to
* safely share the system's floating point registers with other tasks/fibers.
*
* See the description of _FpEnable() for further details.
*
* RETURNS: N/A
*/

FUNC_ALIAS(_FpEnable, fiber_float_enable, void);

/*******************************************************************************
*
* task_float_enable - enable preservation of non-integer context information
*
* This routine allows a task to permit a task/fiber (including itself) to
* safely share the system's floating point registers with other tasks/fibers.
*
* See the description of _FpEnable() for further details.
*
* RETURNS: N/A
*/

FUNC_ALIAS(_FpEnable, task_float_enable, void);

/*******************************************************************************
*
* _FpDisable - disable preservation of non-integer context information
*
* This routine prevents the specified task/fiber (which may be the active
* task/fiber) from safely sharing any of the system's floating point registers
* with other tasks/fibers.
*
* WARNING
* This routine should only be used to disable floating point support for
* a task/fiber that currently has such support enabled.
*
* RETURNS: N/A
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

void _FpDisable(tCCS *ccs)
{
	unsigned int imask;

	/* Lock interrupts to prevent a pre-emptive context switch from occuring
	 */

	imask = irq_lock_inline();

	/*
	 * Disable _all_ floating point capabilities for the task/fiber,
	 * regardless
	 * of the options specified at the time support was enabled.
	 */

	ccs->flags &= ~(USE_FP | USE_SSE);

	if (ccs == _nanokernel.current) {
#ifdef CONFIG_AUTOMATIC_FP_ENABLING
		_FpAccessDisable();
#endif /* CONFIG_AUTOMATIC_FP_ENABLING */

		_nanokernel.current_fp = (tCCS *)0;
	} else {
		if (_nanokernel.current_fp == ccs)
			_nanokernel.current_fp = (tCCS *)0;
	}

	irq_unlock_inline(imask);
}

/*******************************************************************************
*
* fiber_float_disable - disable preservation of non-integer context
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
* RETURNS: N/A
*/

FUNC_ALIAS(_FpDisable, fiber_float_disable, void);

/*******************************************************************************
*
* task_float_disable - disable preservation of non-integer context information
*
* This routine allows a task to disallow a task/fiber (including itself) from
* safely sharing any of the system's floating point registers with other
* tasks/fibers.
*
* WARNING
* This routine should only be used to disable floating point support for
* a task/fiber that currently has such support enabled.
*
* RETURNS: N/A
*/

FUNC_ALIAS(_FpDisable, task_float_disable, void);

#ifdef CONFIG_AUTOMATIC_FP_ENABLING

/*******************************************************************************
*
* _FpNotAvailableExcHandler - handler for "device not available" exception
*
* This routine is registered to handle the "device not available" exception
* (vector = 7) when the AUTOMATIC_FP_ENABLING configuration option has been
* been selected.
*
* The processor will generate this exception if any x87 FPU, MMX, or SSEx
* instruction is executed while CR0[TS]=1.  The handler then enables the
* current task or fiber with the USE_FP option (or the USE_SSE option if the
* SSE configuration option has been enabled).
*
* RETURNS: N/A
*/

void _FpNotAvailableExcHandler(NANO_ESF * pEsf /* not used */
			       )
{
	unsigned int enableOption;

	ARG_UNUSED(pEsf);

	/*
	 * Assume the exception did not occur in the context of an ISR.
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

#endif /* CONFIG_AUTOMATIC_FP_ENABLING */

#endif /* CONFIG_FP_SHARING */
