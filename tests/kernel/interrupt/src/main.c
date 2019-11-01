/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_nested_isr(void);
extern void test_prevent_interruption(void);

extern void z_irq_spurious(void *unused);

/* Simple whitebox test of dynamic interrupt handlers as implemented
 * by the GEN_SW_ISR feature.
 */
#if (defined(CONFIG_DYNAMIC_INTERRUPTS)		\
     && defined(CONFIG_GEN_SW_ISR_TABLE))
#define DYNTEST 1
#endif

#ifdef DYNTEST
static void dyn_isr(void *arg)
{
	ARG_UNUSED(arg);
}

extern struct _isr_table_entry __sw_isr_table _sw_isr_table[];

static void do_isr_dynamic(void)
{
	int i;
	void *argval;

	for (i = 0; i < (CONFIG_NUM_IRQS - CONFIG_GEN_IRQ_START_VECTOR); i++) {
		if (_sw_isr_table[i].isr == z_irq_spurious) {
			break;
		}
	}

	zassert_true(_sw_isr_table[i].isr == z_irq_spurious,
		     "could not find slot for dynamic isr");

	argval = &i;
	z_arch_irq_connect_dynamic(i + CONFIG_GEN_IRQ_START_VECTOR, 0, dyn_isr,
				   argval, 0);

	zassert_true(_sw_isr_table[i].isr == dyn_isr &&
		     _sw_isr_table[i].arg == argval,
		     "dynamic isr did not install successfully");
}
#endif /* DYNTEST */

void test_isr_dynamic(void)
{
#ifdef DYNTEST
	do_isr_dynamic();
#else
	ztest_test_skip();
#endif
}

void test_main(void)
{
	ztest_test_suite(interrupt_feature,
			ztest_unit_test(test_isr_dynamic),
			ztest_unit_test(test_nested_isr),
			ztest_unit_test(test_prevent_interruption)
			);
	ztest_run_test_suite(interrupt_feature);
}
