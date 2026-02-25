/* Copyright 2025 The ChromiumOS Authors
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-M context-switch support helpers
 *
 * The routines in this header back Zephyr's Cortex-M context-switch path
 * when `CONFIG_USE_SWITCH` is enabled. They build and manipulate the stack
 * frames consumed by the hand-written assembly in `arm_m_switch()` and the
 * ISR tail-fixup logic, and expose a small interface to the scheduler and
 * fault handlers.
 */
#ifndef _ZEPHYR_ARCH_ARM_M_SWITCH_H
#define _ZEPHYR_ARCH_ARM_M_SWITCH_H

#include <stdint.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/kernel/thread.h>

/* GCC/gas has a code generation bugglet on thumb.  The R7 register is
 * the ABI-defined frame pointer, though it's usually unused in zephyr
 * due to -fomit-frame-pointer (and the fact the DWARF on ARM doesn't
 * really need it).  But when it IS enabled, which sometimes seems to
 * happen due to toolchain internals, GCC is unable to allow its use
 * in the clobber list of an asm() block (I guess it can't generate
 * spill/fill code without using the frame?).
 *
 * When absolutely needed, this kconfig unmasks a workaround where we
 * spill/fill R7 around the switch manually.
 */
#ifdef CONFIG_ARM_GCC_FP_WORKAROUND
#define _R7_CLOBBER_OPT(expr) expr
#else
#define _R7_CLOBBER_OPT(expr) /**/
#endif

/* Should probably be in kconfig, basically this is testing whether or
 * not the toolchain will allow a "g" flag (DSP state) to an "msr
 * adsp_" instruction.
 */
#if defined(CONFIG_CPU_CORTEX_M4) || defined(CONFIG_CPU_CORTEX_M7) || defined(CONFIG_ARMV8_M_DSP)
#define _ARM_M_SWITCH_HAVE_DSP
#endif

/**
 * @brief Create an initial switch frame on a new thread's stack.
 *
 * The stack contents are prepared so that the first invocation of
 * `arm_m_switch()` can restore directly into @p entry with arguments
 * @p arg0 through @p arg3. The stack base and size are aligned to the
 * 8-byte requirement mandated by the ARM EABI.
 *
 * @param base   Start address of the stack buffer (lowest address).
 * @param sz     Size of the stack buffer in bytes.
 * @param entry  Entry point the thread should begin executing.
 * @param arg0   First argument passed to @p entry.
 * @param arg1   Second argument passed to @p entry.
 * @param arg2   Third argument passed to @p entry.
 * @param arg3   Fourth argument passed to @p entry.
 *
 * @return Pointer to the synthesized switch handle to store in
 *         `struct k_thread::switch_handle`, or NULL if the stack is too
 *         small to hold the frame.
 */
void *arm_m_new_stack(char *base, uint32_t sz, void *entry, void *arg0, void *arg1, void *arg2,
		      void *arg3);

/**
 * @brief Evaluate whether an interrupt should trigger a context switch.
 *
 * Invoked from the ISR tail path to decide if the scheduler selected a new
 * thread. If a switch is needed, this saves the current callee-saved frame
 * pointers in ::arm_m_cs_ptrs and initiates the hand-off to
 * `arm_m_do_switch()`.
 *
 * @retval true  A switch was performed or scheduled.
 * @retval false No switch requested; continue returning from the interrupt.
 */
bool arm_m_must_switch(void);

/**
 * @brief Assembly stub that completes the Cortex-M context restore.
 *
 * This routine is patched into the LR saved on the ISR stack by
 * arm_m_exc_tail() so that callee-saved registers can be fixed up and control
 * returned to the correct thread context.
 */
void arm_m_exc_exit(void);

/**
 * @brief Recover an interrupted IT/ICI instruction after a context switch.
 *
 * The function is called from the fault handler that follows the deliberate
 * `UDF` in arm_m_iciit_stub(). It detects whether the undefined instruction
 * came from our stub and, if so, restores the saved PC/xPSR to re-execute the
 * original instruction.
 *
 * @param msp Exception entry stack pointer for MSP.
 * @param psp Exception entry stack pointer for PSP.
 * @param lr  EXC_RETURN value captured on exception entry.
 *
 * @retval true  The fault corresponded to the IT/ICI recovery stub and was
 *               handled.
 * @retval false The fault was unrelated and should be processed normally.
 */
bool arm_m_iciit_check(uint32_t msp, uint32_t psp, uint32_t lr);

/**
 * @brief Undefined-instruction stub used to force IT/ICI recovery.
 *
 * When an interrupt preempts certain conditional instructions inside an
 * IT/ICI block and a context switch occurs, returning directly can violate
 * architectural rules. The handler patches the stacked PC to this stub so the
 * subsequent fault can repair and resume the original instruction.
 */
void arm_m_iciit_stub(void);

/** Pointer to the stacked LR word used by the ISR tail fixup path. */
extern uint32_t *arm_m_exc_lr_ptr;

void z_arm_configure_dynamic_mpu_regions(struct k_thread *thread);

/** Thread-local storage pointer for the currently running thread. */
extern uintptr_t z_arm_tls_ptr;

/** Backing storage used when relocating stacks during switch operations. */
extern uint32_t arm_m_switch_stack_buffer;

/** @cond INTERNAL_HIDDEN */
/* Global pointers to the frame locations for the callee-saved
 * registers.  Set in arm_m_must_switch(), and used by the fixup
 * assembly in arm_m_exc_exit.
 */
struct arm_m_cs_ptrs {
	/** Pointer to the callee-saved block being written by the outgoing thread */
	void *out, *in, *lr_save, *lr_fixup;
};
/** @endcond */

/** Global instance with current callee-saved frame pointers. */
extern struct arm_m_cs_ptrs arm_m_cs_ptrs;

/**
 * @brief ISR-tail helper that patches the stacked LR for deferred switch fixup.
 *
 * Called near the end of interrupt handling, this routine optionally rewrites
 * the topmost LR on the active stack to branch to arm_m_exc_exit() instead of
 * the original return site. Doing so defers the expensive callee-saved
 * register handling until just before returning to thread mode. Stack
 * sentinel checking is also performed here when enabled.
 */
static inline void arm_m_exc_tail(void)
{
#ifdef CONFIG_MULTITHREADING
	/* Dirty trickery.  We defer as much interrupt-exit work until
	 * the very last moment, when the top-level ISR returns back
	 * into user code.  We do this by replacing the topmost (!) LR
	 * return address in the stack frame with our fixup code at
	 * arm_m_exc_exit().  By running after the ISR return, it
	 * knows that the callee-save registers r4-r11 (which need to
	 * be saved to the outgoing thread) are restored.
	 *
	 * Obviously this only works if the ISR is "ABI-compliant
	 * enough".  It doesn't have to have pushed a complete frame,
	 * but it does have to have put LR into its standard location.
	 * In practice generated code does (because it has to store LR
	 * somewhere so it can call other functions and then pop it to
	 * return), so this works even on code built with
	 * -fomit-frame-pointer.  If an app needs a direct interrupt
	 * and can't meet these requirents, it can always skip this
	 * call and return directly (reschedule is optional for direct
	 * interrupts anyway).
	 *
	 * Finally note the call to check_stack_sentinel here: that is
	 * normally called from context switch at the end, but will
	 * toss an exception, which we can't allow (without hardship)
	 * on the path from here to interrupt exit.  It will mess up
	 * our bookeeping around EXC_RETURN, so do it early.
	 */
	void z_check_stack_sentinel(void);
	void *isr_lr = (void *)*arm_m_exc_lr_ptr;

	if (IS_ENABLED(CONFIG_STACK_SENTINEL)) {
		z_check_stack_sentinel();
	}

	if (isr_lr != arm_m_cs_ptrs.lr_fixup) {
		/* We need to return to arm_m_exc_exit only if an exception is returning to thread
		 * mode with PSP. Note that it is possible to get an exception in arm_m_exc_exit
		 * after interrupts are enabled but, before branching to lr (0xFFFFFFFD) and, at
		 * this point the exception pushes an ESF on MSP. If we write arm_m_exc_exit at top
		 * of MSP at this point, we are corrupting the XPSR of the ESF which will result in
		 * a usage fault. So, make sure that we do this only if we are returning to thread
		 * mode and using PSP to do so.
		 */
		if ((((uint32_t)isr_lr & 0xFFFFFF00U) == 0xFFFFFF00U)
				&& (((uint32_t)isr_lr & 0xC) == 0xC)) {
			arm_m_cs_ptrs.lr_save = isr_lr;
			*arm_m_exc_lr_ptr = (uint32_t)arm_m_cs_ptrs.lr_fixup;
		}
	}
#endif
}

/**
 * @brief Core Cortex-M context switch routine.
 *
 * Performs the low-level swap between the outgoing and incoming thread switch
 * handles. Implemented with inline assembly to manage stacked frames, optional
 * FPU/DSP state, privilege level, and stack guards. Called by arch_switch()
 * and scheduler paths only.
 *
 * @param switch_to     Switch handle (typically PSP) for the next thread.
 * @param switched_from Storage location to write the outgoing switch handle.
 */
static ALWAYS_INLINE void arm_m_switch(void *switch_to, void **switched_from)
{
#if defined(CONFIG_USERSPACE) || defined(CONFIG_MPU_STACK_GUARD)
	z_arm_configure_dynamic_mpu_regions(_current);
#endif

#ifdef CONFIG_THREAD_LOCAL_STORAGE
	z_arm_tls_ptr = _current->tls;
#endif

#if defined(CONFIG_USERSPACE) && defined(CONFIG_USE_SWITCH)
	/* Set things up to write the CONTROL.nPRIV bit.  We know the outgoing
	 * thread is in privileged mode (because you can't reach a
	 * context switch unless you're in the kernel!).
	 */
	extern uint32_t arm_m_switch_control;
	uint32_t control;

	__asm__ volatile("mrs %0, control" : "=r"(control));
	arm_m_switch_control = (control & ~1) | (_current->arch.mode & 1);
#endif

	/* new switch handle in r4, old switch handle pointer in r5.
	 * r6-r8 are used by the code here, and r9-r11 are
	 * unsaved/clobbered (they are very likely to be caller-saved
	 * registers in the enclosing function that the compiler can
	 * avoid using, i.e. we can let it make the call and avoid a
	 * double-spill).  But all registers are restored fully
	 * (because we might be switching to an interrupt-saved frame)
	 */
	register uint32_t r4 __asm__("r4") = (uint32_t)switch_to;
	register uint32_t r5 __asm__("r5") = (uint32_t)switched_from;
	__asm__ volatile(_R7_CLOBBER_OPT("push {r7};")
			 /* Construct and push a {r12, lr, pc} group at the top
			  * of the frame, where PC points to the final restore location
			  * at the end of this sequence.
			  */
			 "mov r6, r12;"
			 "mov r7, lr;"
			 "ldr r8, =3f;"    /* address of restore PC */
			 "orr r8, r8, #1;" /* set thumb bit */
			 "push {r6-r8};"
			 "sub sp, sp, #24;" /* skip over space for r6-r11 */
			 "push {r0-r5};"
			 "mov r2, #0x01000000;" /* APSR (only care about thumb bit) */
			 "mov r0, #0;"          /* Leave r0 zero for code blow */
#ifdef CONFIG_BUILTIN_STACK_GUARD
			 "mrs r1, psplim;"
			 "push {r1-r2};"
			 "msr psplim, r0;" /* zero it so we can move the stack */
#else
			 "push {r2};"
#endif

#ifdef CONFIG_FPU
			 /* Push FPU state (if active) to our outgoing stack */
			 "   mrs r8, control;" /* read CONTROL.FPCA */
			 "   and r7, r8, #4;"  /* r7 == have_fpu */
			 "   cbz r7, 1f;"
			 "   bic r8, r8, #4;" /* clear CONTROL.FPCA */
			 "   msr control, r8;"
			 "   vmrs r6, fpscr;"
			 "   push {r6};"
			 "   vpush {s0-s31};"
			 "1: push {r7};" /* have_fpu word */

			 /* Pop FPU state (if present) from incoming frame in r4 */
			 "   ldm r4!, {r7};" /* have_fpu word */
			 "   cbz r7, 2f;"
			 "   vldm r4!, {s0-s31};" /* (note: sets FPCA bit for us) */
			 "   ldm r4!, {r6};"
			 "   vmsr fpscr, r6;"
			 "2:;"
#endif

#if defined(CONFIG_USERSPACE) && defined(CONFIG_USE_SWITCH)
			 "  ldr r8, =arm_m_switch_control;"
			 "  ldr r8, [r8];"
#endif

			 /* Save the outgoing switch handle (which is SP), swap stacks,
			  * and enable interrupts.  The restore process is
			  * interruptible code (running in the incoming thread) once
			  * the stack is valid.
			  */
			 "str sp, [r5];"
			 "mov sp, r4;"
			 "msr basepri, r0;"

#if defined(CONFIG_USERSPACE) && defined(CONFIG_USE_SWITCH)
			 "  msr control, r8;" /* Now we can drop privilege */
#endif

	/* Restore is super simple: pop the flags (and stack limit if
	 * enabled) then slurp in the whole GPR set in two
	 * instructions. (The instruction encoding disallows popping
	 * both LR and PC in a single instruction)
	 */
#ifdef CONFIG_BUILTIN_STACK_GUARD
			 "pop {r1-r2};"
			 "msr psplim, r1;"
#else
			 "pop {r2};"
#endif
#ifdef _ARM_M_SWITCH_HAVE_DSP
			 "msr apsr_nzcvqg, r2;" /* bonkers syntax */
#else
			 "msr apsr_nzcvq, r2;" /* not even source-compatible! */
#endif
			 "pop {r0-r12, lr};"
			 "pop {pc};"

			 "3:" /* Label for restore address */
			 _R7_CLOBBER_OPT("pop {r7};")::"r"(r4),
			 "r"(r5)
			 : "r6", "r8", "r9", "r10",
#ifndef CONFIG_ARM_GCC_FP_WORKAROUND
			   "r7",
#endif
			   "r11");
}

#ifdef CONFIG_USE_SWITCH
/**
 * @brief Public arch-level wrapper for the Cortex-M switch routine.
 *
 * Thin inline that forwards to arm_m_switch() when the architecture uses
 * the `CONFIG_USE_SWITCH` mechanism.
 *
 * @param switch_to     Switch handle for the next thread.
 * @param switched_from Storage for the outgoing switch handle.
 */
static ALWAYS_INLINE void arch_switch(void *switch_to, void **switched_from)
{
	arm_m_switch(switch_to, switched_from);
}
#endif

#endif /* _ZEPHYR_ARCH_ARM_M_SWITCH_H */
