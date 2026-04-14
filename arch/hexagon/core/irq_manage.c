/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <hexagon_vm.h>
#include <hexagon_intc.h>
#include <irq.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* Event context saved by assembly handlers -- must match event_handlers.S */
struct event_context {
	uint32_t r0_r1[2];
	uint32_t r2_r3[2];
	uint32_t r4_r5[2];
	uint32_t r6_r7[2];
	uint32_t r8_r9[2];
	uint32_t r10_r11[2];
	uint32_t r12_r13[2];
	uint32_t r14_r15[2];
	uint32_t pred_regs;
	uint32_t link_reg;
	uint32_t gelr;          /* saved GELR (return PC) */
	uint32_t gsr;           /* saved GSR (guest status) */
	uint32_t sa0;
	uint32_t lc0;
	uint32_t sa1;
	uint32_t lc1;
	uint32_t m0;
	uint32_t m1;
	uint32_t usr;
};

/* ISR nesting counter -- read by arch_is_in_isr() in arch.h */
uint32_t z_hexagon_isr_nesting;

/* Forward declarations for handlers defined below */
static void z_hexagon_exception_handler(struct event_context *ctx);
static void z_hexagon_trap0_handler(struct event_context *ctx);
static void z_hexagon_interrupt_handler(struct event_context *ctx);

/* Main event handler called from assembly */
void z_hexagon_event_handler(unsigned int event_num, struct event_context *ctx)
{
	switch (event_num) {
	case HEXAGON_EVENT_MACHINE_CHECK:
		z_hexagon_fatal_error(K_ERR_CPU_EXCEPTION);
		break;

	case HEXAGON_EVENT_GENERAL_EXCEPTION:
		z_hexagon_exception_handler(ctx);
		break;

	case HEXAGON_EVENT_TRAP0:
		z_hexagon_trap0_handler(ctx);
		break;

	case HEXAGON_EVENT_INTERRUPT:
		z_hexagon_interrupt_handler(ctx);
		break;

	default:
		z_hexagon_fatal_error(K_ERR_SPURIOUS_IRQ);
		break;
	}
}

/* Handle general exceptions */
#define GSR_CAUSE_MASK 0xFF

static void z_hexagon_exception_handler(struct event_context *ctx)
{
	uint32_t cause = ctx->gsr & GSR_CAUSE_MASK;
	uint32_t pc = ctx->gelr;

	LOG_ERR("exception: cause=0x%x pc=0x%x", cause, pc);

	/* Fatal error for now */
	z_hexagon_fatal_error(K_ERR_CPU_EXCEPTION);
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
	uint32_t syscall_num = ctx->r6_r7[0]; /* r6 */

	ARG_UNUSED(syscall_num);
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
	const struct _isr_table_entry *entry = &_sw_isr_table[irq_num];

	entry->isr(entry->arg);

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
	/* Set up SW ISR table entry (only possible with dynamic interrupts) */
	_sw_isr_table[irq].isr = routine;
	_sw_isr_table[irq].arg = parameter;
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

/* Initialize interrupt handling */
void z_hexagon_irq_init(void)
{
	/* Initialize interrupt controller (disables all IRQs) */
	hexagon_intc_init();
}
