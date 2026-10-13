/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/arch/hexagon/exception.h>
#include <hexagon_vm.h>
#include <hexagon_intc.h>
#include <hexagon_fatal.h>
#include <irq.h>
#include <event_context.h>
#if defined(CONFIG_GDBSTUB)
#include <zephyr/arch/hexagon/gdbstub.h>
#endif
#ifdef CONFIG_USERSPACE
extern void z_hexagon_syscall_handler(struct arch_esf *esf);
extern void z_hexagon_user_mode_sync(void);
#endif

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* ISR nesting counter -- read by arch_is_in_isr() in arch.h */
uint32_t z_hexagon_isr_nesting;

/* Forward declarations for handlers defined below */
static void z_hexagon_exception_handler(unsigned int event_num,
					struct event_context *ctx);
static void z_hexagon_trap0_handler(struct event_context *ctx);
static void z_hexagon_interrupt_handler(struct event_context *ctx);

/* Main event handler called from assembly */
void z_hexagon_event_handler(unsigned int event_num, struct event_context *ctx)
{
#ifdef CONFIG_USERSPACE
	/* We're in kernel mode now; clear so arch_is_user_context()=false. */
	_hexagon_user_mode_active = 0;
#endif

	/*
	 * Re-enable guest interrupts for syscall handling: H2 disables IE on
	 * event entry, but kernel syscall code expects arch_irq_lock() to
	 * report IE was enabled. Nested interrupts are safe since EVENT_ENTRY
	 * saves all volatile state.
	 */
	if (event_num == HEXAGON_EVENT_TRAP0) {
#ifdef CONFIG_GDBSTUB
		uint32_t trap_pc = ctx->gelr - 4U;
		bool is_gdb_breakpoint = z_hexagon_is_gdb_breakpoint(trap_pc);

		if (!is_gdb_breakpoint) {
			hexagon_vm_setie(VM_INT_ENABLE);
		}
#else
		hexagon_vm_setie(VM_INT_ENABLE);
#endif
#ifdef CONFIG_USERSPACE
		/*
		 * Mark this thread's trap0 handling in flight, so a nested
		 * event's z_hexagon_event_exit_user_sync() knows it is
		 * resuming kernel-mode code, not user mode -- see
		 * arch.trap0_active's comment in thread.h.
		 */
		_current->arch.trap0_active = 1;
#endif
	}

	switch (event_num) {
	case HEXAGON_EVENT_MACHINE_CHECK:
		z_hexagon_fatal_error_ctx(K_ERR_CPU_EXCEPTION, event_num, ctx);
		break;

	case HEXAGON_EVENT_GENERAL_EXCEPTION:
		z_hexagon_exception_handler(event_num, ctx);
		break;

	case HEXAGON_EVENT_DEBUG:
#ifdef CONFIG_GDBSTUB
		z_hexagon_gdb_entry(ctx);
		break;
#else
		z_hexagon_fatal_error_ctx(K_ERR_CPU_EXCEPTION, event_num, ctx);
		break;
#endif

	case HEXAGON_EVENT_TRAP0:
		z_hexagon_trap0_handler(ctx);
		break;

	case HEXAGON_EVENT_INTERRUPT:
		z_hexagon_interrupt_handler(ctx);
		break;

	default:
		z_hexagon_fatal_error_ctx(K_ERR_SPURIOUS_IRQ, event_num, ctx);
		break;
	}

#ifdef CONFIG_USERSPACE
	if (event_num == HEXAGON_EVENT_TRAP0) {
		/* This thread's own trap0 handling has finished normally. */
		_current->arch.trap0_active = 0;
	}
#endif

	/*
	 * Disable interrupts before returning to the EVENT_EXIT assembly
	 * path.  EVENT_EXIT expects IE=0 for the preemption check and
	 * vmrte sequence.
	 */
	hexagon_vm_setie(VM_INT_DISABLE);
}

#ifdef CONFIG_USERSPACE
/*
 * Called from EVENT_EXIT after any context switch, with _current already
 * the thread about to resume. Skipped when trap0_active is set: that
 * means this resumption point is still the thread's own kernel-mode trap0
 * handler (preempted, not returning to real user mode) -- see
 * arch.trap0_active's comment in thread.h.
 */
void z_hexagon_event_exit_user_sync(void)
{
	if (!_current->arch.trap0_active) {
		z_hexagon_user_mode_sync();
	}
}
#endif

/* Handle general exceptions */
#define GSR_CAUSE_MASK 0xFF

static void z_hexagon_exception_handler(unsigned int event_num,
					struct event_context *ctx)
{
	uint32_t cause = ctx->gsr & GSR_CAUSE_MASK;
	uint32_t pc = ctx->gelr;

	LOG_ERR("exception: cause=0x%x pc=0x%x", cause, pc);

#ifdef CONFIG_GDBSTUB
	/* A software breakpoint may be reported as a general exception. */
	if (z_hexagon_is_gdb_breakpoint(pc)) {
		z_hexagon_gdb_entry(ctx);
		return;
	}
#endif

	/* Fatal error for now */
	z_hexagon_fatal_error_ctx(K_ERR_CPU_EXCEPTION, event_num, ctx);
}

/* Handle trap0 (syscall) events.
 *
 * Under H2, trap0 generates GEVB entry 5.  GELR points to the instruction
 * AFTER the trap0, so the trap0 itself is at GELR-4.
 *
 * The event_context r6_r7[0] holds r6 which is the syscall number by
 * convention.
 */
static void z_hexagon_trap0_handler(struct event_context *ctx)
{
#ifdef CONFIG_GDBSTUB
	/* GELR points past trap0, so inspect the preceding instruction. */
	uint32_t trap_pc = ctx->gelr - 4U;

	if (z_hexagon_is_gdb_breakpoint(trap_pc)) {
		/* Resume at the breakpoint address after GDB restores the instruction. */
		ctx->gelr = trap_pc;
		z_hexagon_gdb_entry(ctx);
		return;
	}
#endif

#ifdef CONFIG_USERSPACE
	{
		/* Minimal arch_esf: only args and syscall number matter. */
		struct arch_esf esf = { 0 };

		esf.r0 = ctx->r0_r1[0];
		esf.r1 = ctx->r0_r1[1];
		esf.r2 = ctx->r2_r3[0];
		esf.r3 = ctx->r2_r3[1];
		esf.r4 = ctx->r4_r5[0];
		esf.r5 = ctx->r4_r5[1];
		esf.r6 = ctx->r6_r7[0]; /* syscall number */
		esf.r7 = ctx->r6_r7[1];

		z_hexagon_syscall_handler(&esf);

		/* Write return value back to the event context */
		ctx->r0_r1[0] = esf.r0;
	}
#else
	/* trap0 should never fire without CONFIG_USERSPACE. */
	ARG_UNUSED(ctx);
	z_hexagon_fatal_error(K_ERR_CPU_EXCEPTION);
#endif
}

/* Handle interrupts */
static void z_hexagon_interrupt_handler(struct event_context *ctx)
{
	uint32_t irq_num;
	uint32_t cause = ctx->gsr & GSR_CAUSE_MASK;

	/*
	 * GSR.CAUSE (bits 7:0) contains the virtual IRQ number
	 * delivered by H2 (e.g. 12 for the timer).  The interrupt
	 * has already been consumed from the pending bitmap, so
	 * vmintop GET would not find it.
	 */
	irq_num = cause;

	if (irq_num >= ARCH_IRQ_COUNT) {
		return; /* Out of range -- spurious */
	}

	/* Track ISR nesting so arch_is_in_isr() works correctly */
	z_hexagon_isr_nesting++;

	/* Call ISR from SW ISR table */
#if defined(CONFIG_GEN_SW_ISR_TABLE)
	const struct _isr_table_entry *entry = &_sw_isr_table[irq_num];

	entry->isr(entry->arg);
#else
	ARG_UNUSED(irq_num);
	z_irq_spurious(NULL);
#endif

	z_hexagon_isr_nesting--;

	/* Re-enable the interrupt (H2 disables it on delivery) */
	hexagon_intc_ack(irq_num);
}

/* Enable an IRQ */
void arch_irq_enable(unsigned int irq)
{
	if (irq >= ARCH_IRQ_COUNT) {
		return;
	}

	/* Use interrupt controller abstraction */
	hexagon_intc_enable(irq);
}

/* Disable an IRQ */
void arch_irq_disable(unsigned int irq)
{
	if (irq >= ARCH_IRQ_COUNT) {
		return;
	}

	/* Use interrupt controller abstraction */
	hexagon_intc_disable(irq);
}

/* Check if IRQ is enabled */
int arch_irq_is_enabled(unsigned int irq)
{
	uint32_t status;

	if (irq >= ARCH_IRQ_COUNT) {
		return 0;
	}

	/* Use vmintop to query interrupt state */
	status = hexagon_vm_intop_status(irq);

	return status & 1;
}

/* Connect IRQ at runtime */
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter), const void *parameter,
			     uint32_t flags)
{
	if (irq >= ARCH_IRQ_COUNT) {
		return -EINVAL;
	}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
	/* Set up SW ISR table entry atomically with respect to the IRQ */
	unsigned int key = arch_irq_lock();

	_sw_isr_table[irq].isr = routine;
	_sw_isr_table[irq].arg = parameter;
	arch_irq_unlock(key);
#endif

	/* Set interrupt priority */
	hexagon_irq_priority_set(irq, priority);

	return 0;
}

/* Set interrupt priority */
void hexagon_irq_priority_set(unsigned int irq, unsigned int priority)
{
	if (irq >= ARCH_IRQ_COUNT || priority > HEXAGON_IRQ_PRIORITY_LOWEST) {
		return;
	}

	/* Use interrupt controller abstraction */
	hexagon_intc_set_priority(irq, priority);
}

/* Spurious interrupt handler */
FUNC_NORETURN void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);

	LOG_ERR("Spurious interrupt detected!");
	z_hexagon_fatal_error(K_ERR_SPURIOUS_IRQ);

	CODE_UNREACHABLE;
}
