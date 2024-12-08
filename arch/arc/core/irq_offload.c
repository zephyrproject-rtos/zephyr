/*
 * Copyright (c) 2015 Intel corporation
 * Copyright (c) 2022 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Software interrupts utility code - ARC implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <zephyr/init.h>

/* Choose a reasonable default for interrupt line which is used for irq_offload with the option
 * to override it by setting interrupt line via device tree.
 */
#if DT_NODE_EXISTS(DT_NODELABEL(test_irq_offload_line_0))
#define IRQ_OFFLOAD_LINE	DT_IRQN(DT_NODELABEL(test_irq_offload_line_0))
#else
/* Last two lined are already used in the IRQ tests, so we choose 3rd from the end line */
#define IRQ_OFFLOAD_LINE	(CONFIG_NUM_IRQS - 3)
#endif

#define IRQ_OFFLOAD_PRIO	0

#define CURR_CPU (IS_ENABLED(CONFIG_SMP) ? arch_curr_cpu()->id : 0)

static struct {
	volatile irq_offload_routine_t fn;
	const void *volatile arg;
} offload_params[CONFIG_MP_MAX_NUM_CPUS];

static void arc_irq_offload_handler(const void *unused)
{
	ARG_UNUSED(unused);

	offload_params[CURR_CPU].fn(offload_params[CURR_CPU].arg);
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	offload_params[CURR_CPU].fn = routine;
	offload_params[CURR_CPU].arg = parameter;
	compiler_barrier();

	z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_HINT, IRQ_OFFLOAD_LINE);

	__asm__ volatile("sync");

	/* If arch_current_thread() was aborted in the offload routine, we shouldn't be here */
	__ASSERT_NO_MSG((arch_current_thread()->base.thread_state & _THREAD_DEAD) == 0);
}

/* need to be executed on every core in the system */
void arch_irq_offload_init(void)
{

	IRQ_CONNECT(IRQ_OFFLOAD_LINE, IRQ_OFFLOAD_PRIO, arc_irq_offload_handler, NULL, 0);

	/* The line is triggered and controlled with core private interrupt controller,
	 * so even in case common (IDU) interrupt line usage on SMP we need to enable it not
	 * with generic irq_enable() but via z_arc_v2_irq_unit_int_enable().
	 */
	z_arc_v2_irq_unit_int_enable(IRQ_OFFLOAD_LINE);
}
