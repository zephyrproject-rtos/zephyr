/* Copyright 2025 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ZEPHYR_ARCH_ARM_M_SWITCH_H
#define _ZEPHYR_ARCH_ARM_M_SWITCH_H

#include <stdint.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/kernel/thread.h>

void *arm_m_new_stack(char *base, uint32_t sz, void *entry, void *arg0, void *arg1, void *arg2,
		      void *arg3);

bool arm_m_must_switch(uint32_t lr);

void arm_m_exc_exit(void);

extern uint32_t *arm_m_exc_lr_ptr;

void z_arm_configure_dynamic_mpu_regions(struct k_thread *thread);

extern uintptr_t z_arm_tls_ptr;

extern uint32_t arm_m_switch_stack_buffer;

static inline void arm_m_exc_tail(void)
{
	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return;
	}

	/* Dirty trickery: we load this ISR's LR register (which
	 * contains our interrupt return token) from the runtime stack
	 * frame pushed to the top of the interrupt stack on entry.
	 * Check it to see if we can/should return to a different
	 * thread (which will then have magically pickled our
	 * interrupted stack into "switch" format), and then if so:
	 * CLOBBER it with the address of our fixup code so that we
	 * can finish saving the interrupted r4-r11 registers before
	 * returning from the interrupt.
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
	 * Note that this needlessly duplicates work in the case of
	 * nested interrupts, which will inspect scheduler state at
	 * each exception layer as it returns.  Really we want to
	 * defer this to the last one, but that requires an entry
	 * hook.  Unlikely to be a serious problem in practice as apps
	 * need to be prepared, performance-wise, for nested
	 * interrupts to arrive sequentially, so optimizing for the
	 * former doesn't help worst-case latency.
	 *
	 * Note also that the use of the global variable requires an
	 * extra load vs. computing the stack location directly here
	 * at compile time.  The header tangle required to get the
	 * interrupt stack exposed this early wasn't something I could
	 * solve.  Nonetheless there are two cycles on the table here
	 * for someone enterprising.
	 */
	if (arm_m_must_switch(*arm_m_exc_lr_ptr)) {
		*arm_m_exc_lr_ptr = 1 | (uint32_t)arm_m_exc_exit; /* thumb bit! */
	}
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

#ifdef CONFIG_FPU_SHARING
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
		"msr apsr_nzcvq, r2;" /* bonkers syntax */
		"pop {r0-r12, lr};"
		"pop {pc};"

		"3:" /* Label for restore address */
		::"r"(r4),
		"r"(r5)
		: "r6", "r7", "r8", "r9", "r10", "r11");
}

#ifdef CONFIG_USE_SWITCH
static ALWAYS_INLINE void arch_switch(void *switch_to, void **switched_from)
{
	arm_m_switch(switch_to, switched_from);
}
#endif

#endif /* _ZEPHYR_ARCH_ARM_M_SWITCH_H */
