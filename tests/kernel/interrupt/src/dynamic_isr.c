/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#if defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_SW_ISR_TABLE)
extern struct _isr_table_entry __sw_isr_table _sw_isr_table[];
extern void z_irq_spurious(const void *unused);

static void dyn_isr(const void *arg)
{
	ARG_UNUSED(arg);
}

/**
 * @brief Test dynamic ISR installation
 *
 * @ingroup kernel_interrupt_tests
 *
 * This routine locates an unused entry in the software ISR table, installs a
 * dynamic ISR to the unused entry by calling the `arch_irq_connect_dynamic`
 * function, and verifies that the ISR is successfully installed by checking
 * the software ISR table entry.
 */
void test_isr_dynamic(void)
{
	int i;
	const void *argval;

	for (i = 0; i < (CONFIG_NUM_IRQS - CONFIG_GEN_IRQ_START_VECTOR); i++) {
		if (_sw_isr_table[i].isr == z_irq_spurious) {
			break;
		}
	}

	zassert_true(_sw_isr_table[i].isr == z_irq_spurious,
		     "could not find slot for dynamic isr");

	printk("installing dynamic ISR for IRQ %d\n",
	       CONFIG_GEN_IRQ_START_VECTOR + i);

	argval = (const void *)&i;
	arch_irq_connect_dynamic(i + CONFIG_GEN_IRQ_START_VECTOR, 0, dyn_isr,
				 argval, 0);

	zassert_true(_sw_isr_table[i].isr == dyn_isr &&
		     _sw_isr_table[i].arg == argval,
		     "dynamic isr did not install successfully");
}
#else
/* Skip the dynamic interrupt test for the platforms that do not support it */
void test_isr_dynamic(void)
{
	ztest_test_skip();
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS && CONFIG_GEN_SW_ISR_TABLE */
