/* Copyright 2025 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
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

void *arm_m_new_stack(char *base, uint32_t sz, void *entry, void *arg0, void *arg1, void *arg2,
		      void *arg3);

bool arm_m_must_switch(void);

void arm_m_exc_exit(void);

bool arm_m_iciit_check(uint32_t msp, uint32_t psp, uint32_t lr);

void arm_m_iciit_stub(void);

extern uint32_t *arm_m_exc_lr_ptr;

void z_arm_configure_dynamic_mpu_regions(struct k_thread *thread);

extern uintptr_t z_arm_tls_ptr;

extern uint32_t arm_m_switch_stack_buffer;

/* Global pointers to the frame locations for the callee-saved
 * registers.  Set in arm_m_must_switch(), and used by the fixup
 * assembly in arm_m_exc_exit.
 */
struct arm_m_cs_ptrs {
	void *out, *in, *lr_save, *lr_fixup;
};
extern struct arm_m_cs_ptrs arm_m_cs_ptrs;

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
	void *isr_lr = (void *) *arm_m_exc_lr_ptr;

	if (IS_ENABLED(CONFIG_STACK_SENTINEL)) {
		z_check_stack_sentinel();
	}
	if (isr_lr != arm_m_cs_ptrs.lr_fixup) {
		arm_m_cs_ptrs.lr_save = isr_lr;
		*arm_m_exc_lr_ptr = (uint32_t) arm_m_cs_ptrs.lr_fixup;
	}
#endif
}

static ALWAYS_INLINE void arm_m_switch(void *switch_to, void **switched_from)
{
#if defined(CONFIG_USERSPACE) || defined(CONFIG_MPU_STACK_GUARD)
	z_arm_configure_dynamic_mpu_regions(_current);
#endif

#ifdef CONFIG_THREAD_LOCAL_STORAGE
	z_arm_tls_ptr = _current->tls;
#endif

#if defined(CONFIG_USERSPACE) && defined(CONFIG_USE_SWITCH)
	/* Need to manage CONTROL.nPRIV bit.  We know the outgoing
	 * thread is in privileged mode (because you can't reach a
	 * context switch unless you're in the kernel!).
	 */
	extern uint32_t arm_m_switch_control;
	uint32_t control;
	struct k_thread *old = CONTAINER_OF(switched_from, struct k_thread, switch_handle);

	old->arch.mode &= ~1;
	__asm__ volatile("mrs %0, control" : "=r"(control));
	__ASSERT_NO_MSG((control & 1) == 0);
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
	__asm__ volatile(
		 _R7_CLOBBER_OPT("push {r7};")
		/* Construct and push a {r12, lr, pc} group at the top
		 * of the frame, where PC points to the final restore location
		 * at the end of this sequence.
		 */
		"mov r6, r12;"
		"mov r7, lr;"
		"ldr r8, =3f;"    /* address of restore PC */
		"add r8, r8, #1;" /* set thumb bit */
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
		"  msr control, r8;"
#endif

		/* Save the outgoing switch handle (which is SP), swap stacks,
		 * and enable interrupts.  The restore process is
		 * interruptible code (running in the incoming thread) once
		 * the stack is valid.
		 */
		"str sp, [r5];"
		"mov sp, r4;"
		"msr basepri, r0;"

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
		 _R7_CLOBBER_OPT("pop {r7};")
		::"r"(r4), "r"(r5)
		 : "r6", "r8", "r9", "r10",
#ifndef CONFIG_ARM_GCC_FP_WORKAROUND
		    "r7",
#endif
		    "r11");
}

#ifdef CONFIG_USE_SWITCH
static ALWAYS_INLINE void arch_switch(void *switch_to, void **switched_from)
{
	arm_m_switch(switch_to, switched_from);
}
#endif

#endif /* _ZEPHYR_ARCH_ARM_M_SWITCH_H */
