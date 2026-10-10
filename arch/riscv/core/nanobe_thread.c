/*
 * Copyright (c) 2016-2026 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zephyr threads as nanobes on RISC-V
 *
 * With CONFIG_USE_NANOBE_SWITCH, a Zephyr thread context switch is a nanobe
 * switch: arch_switch() saves only the callee-saved registers on the outgoing
 * thread's stack and swaps the stack pointer, in thread mode. The switch
 * handle of a switched out thread is its saved stack pointer.
 *
 * All scheduling decisions are taken in thread mode, no context switch is
 * done on interrupt exit nor through ecall. An interrupt service routine that
 * readies or halts threads only defers the decision (see update_cache()); on
 * interrupt exit, z_sched_deferred_reschedule() is injected into the
 * interrupted thread, which then decides and switches in thread mode.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel/nanobe.h>
#include <zephyr/arch/riscv/csr.h>
#include <kernel_internal.h>
#include <ksched.h>

#if defined(CONFIG_PMP_KERNEL_MODE_DYNAMIC)
#error "CONFIG_USE_NANOBE_SWITCH does not support the RISC-V dynamic kernel mode PMP"
#endif

#if defined(CONFIG_RISCV_SOC_HAS_ISR_STACKING) && !defined(CONFIG_NANOBE_HW_STACKING)
#error "CONFIG_USE_NANOBE_SWITCH supports hardware ISR stacking on the Nordic VPR only"
#endif

/* Context frame saved on a nanobe stack by nanobe_switch(), lowest address
 * first. Must match the store/load sequence in nanobe.S.
 */
struct nanobe_frame {
	unsigned long igrd;
	unsigned long ra;
	unsigned long s0, s1;
#if !defined(CONFIG_RISCV_ISA_RV32E)
	unsigned long s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
	unsigned long pad[2];
#endif
};

BUILD_ASSERT((sizeof(struct nanobe_frame) % NANOBE_STACK_ALIGN) == 0);

#if defined(CONFIG_NANOBE_HW_STACKING)
/* The syringe stacks its frame in the VPR hardware stacking layout */
#define NANOBE_VPR_ESF_OFFSET(member)                                                              \
	(offsetof(struct arch_esf, member) - offsetof(struct arch_esf, ra))

BUILD_ASSERT((NANOBE_VPR_ESF_OFFSET(ra) == 0) && (NANOBE_VPR_ESF_OFFSET(t2) == 4) &&
	     (NANOBE_VPR_ESF_OFFSET(t1) == 8) && (NANOBE_VPR_ESF_OFFSET(t0) == 12) &&
	     (NANOBE_VPR_ESF_OFFSET(a5) == 16) && (NANOBE_VPR_ESF_OFFSET(a4) == 20) &&
	     (NANOBE_VPR_ESF_OFFSET(a3) == 24) && (NANOBE_VPR_ESF_OFFSET(a2) == 28) &&
	     (NANOBE_VPR_ESF_OFFSET(a1) == 32) && (NANOBE_VPR_ESF_OFFSET(a0) == 36) &&
	     (NANOBE_VPR_ESF_OFFSET(_mcause) == 40) && (NANOBE_VPR_ESF_OFFSET(mepc) == 44),
	     "VPR hardware stack frame layout does not match the nanobe syringe");
#endif /* CONFIG_NANOBE_HW_STACKING */

/* Nanobe entry trampoline, calls the entry function held in s0 with the
 * argument held in s1.
 */
extern void z_nanobe_entry(void);

void *z_nanobe_frame_init(nanobe_arg_t entry, void *arg, void *stack_top)
{
	struct nanobe_frame *frame;
	uintptr_t top;

	__ASSERT_NO_MSG(entry != NULL);
	__ASSERT_NO_MSG(stack_top != NULL);

	top = ROUND_DOWN((uintptr_t)stack_top, NANOBE_STACK_ALIGN);
	frame = (struct nanobe_frame *)top - 1;

	(void)memset(frame, 0, sizeof(*frame));
	frame->s0 = (unsigned long)entry;
	frame->s1 = (unsigned long)arg;
	frame->ra = (unsigned long)z_nanobe_entry;

	return frame;
}

/* Interrupted thread's exception stack frame. _isr_wrapper saves the thread
 * stack pointer, i.e. the exception stack frame, at the top of the interrupt
 * stack when the interrupt nesting count goes from 0 to 1.
 */
static inline struct arch_esf *isr_esf(void)
{
	return *(struct arch_esf **)(_current_cpu->irq_stack - 16);
}

int z_nanobe_arch_isr_inject_check(void)
{
	/* Only when returning to thread mode, i.e. not a nested interrupt */
	if (_current_cpu->nested != 1U) {
		return -EPERM;
	}

	return 0;
}

void z_nanobe_arch_isr_inject_redirect(void)
{
	struct arch_esf *esf = isr_esf();

	/* The syringe returns to the interrupted code using mret. With VPR
	 * hardware stacking, the hardware stack alignment flag is kept
	 * separately in the exception stack frame, the stacked mepc is the
	 * plain address.
	 */
	z_nanobe_iret = esf->mepc;
	esf->mepc = (unsigned long)z_nanobe_syringe;
}

/* Interrupt exit, called by _isr_wrapper before the interrupt nesting count
 * is decremented. No scheduling here.
 */
void z_riscv_nanobe_int_exit(void)
{
	if (z_sched_deferred != 0U) {
		(void)z_nanobe_isr_inject(z_sched_deferred_reschedule);
	}
}

/* Thread entry and arguments, kept at the top of a new thread's stack */
struct nanobe_thread_entry {
	k_thread_entry_t entry;
	void *p1;
	void *p2;
	void *p3;
};

static void nanobe_thread_start(void *arg)
{
	struct nanobe_thread_entry *start = arg;

	/* First switched in by arch_switch(), with interrupts locked */
	arch_irq_unlock(MSTATUS_IEN);

	z_thread_entry(start->entry, start->p1, start->p2, start->p3);
}

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack, char *stack_ptr,
		     k_thread_entry_t entry, void *p1, void *p2, void *p3)
{
	struct nanobe_thread_entry *start;

	ARG_UNUSED(stack);

	start = (struct nanobe_thread_entry *)ROUND_DOWN((uintptr_t)stack_ptr - sizeof(*start),
							 NANOBE_STACK_ALIGN);
	start->entry = entry;
	start->p1 = p1;
	start->p2 = p2;
	start->p3 = p3;

	/* our switch handle is the saved nanobe stack pointer */
	thread->switch_handle = z_nanobe_frame_init(nanobe_thread_start, start, start);
}

void z_riscv_nanobe_switch(void *switch_to, void **switched_from)
{
	/* Nanobe scheduler lock of the outgoing thread, released by
	 * nanobe_switch() for the incoming one.
	 */
	uint8_t sgrd = z_nanobe_sgrd;

#if defined(CONFIG_THREAD_LOCAL_STORAGE)
	/* _current is already the incoming thread */
	__asm__ volatile("mv tp, %0" : : "r"(_current->tls) : "memory");
#endif

#if defined(CONFIG_INSTRUMENT_THREAD_SWITCHING)
	z_thread_mark_switched_in();
#endif

	nanobe_switch(switch_to, switched_from);

	/* Switched back in */
	z_nanobe_sgrd = sgrd;
}
