/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/interrupt_util.h>

#define ISR_DYN_ARG	0xab249cfd

static unsigned int handler_has_run;
static uintptr_t handler_test_result;

static void dyn_isr(const void *arg)
{
	ARG_UNUSED(arg);
	handler_test_result = (uintptr_t)arg;
	handler_has_run++;
}

#if defined(CONFIG_GEN_SW_ISR_TABLE)
extern struct _isr_table_entry __sw_isr_table _sw_isr_table[];

/**
 * @brief Test dynamic ISR installation
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details This routine locates an unused entry in the software ISR table,
 * installs a dynamic ISR to the unused entry by calling the dynamic
 * configured function, and verifies that the ISR is successfully installed
 * by checking the software ISR table entry.
 *
 * @see arch_irq_connect_dynamic()
 */
ZTEST(interrupt_feature, test_isr_dynamic)
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
/*
 * For testing arch such as x86, x86_64 and posix which support dynamic
 * interrupt but without SW ISR table, we test it by applying for a
 * dynamic interrupt and then trigger it to check if happened correctly.
 */
#if defined(CONFIG_X86)
#define IV_IRQS 32	/* start of vectors available for x86 IRQs */
#define TEST_IRQ_DYN_LINE 25

#elif defined(CONFIG_ARCH_POSIX)
#define TEST_IRQ_DYN_LINE 5
#endif

void test_isr_dynamic(void)
{
	int vector_num;

	/**TESTPOINT: configuration of interrupts dynamically at runtime */
	vector_num = arch_irq_connect_dynamic(TEST_IRQ_DYN_LINE, 1, dyn_isr,
				 (void *)ISR_DYN_ARG, 0);

#if defined(CONFIG_X86_64)
/* The isr table for x86_64 is visiable, so check it up here */
extern void (*x86_irq_funcs[])(const void *);
extern const void *x86_irq_args[];

	zassert_true(x86_irq_funcs[vector_num - IV_IRQS] == dyn_isr &&
		     x86_irq_args[vector_num - IV_IRQS] == (void *)ISR_DYN_ARG,
		     "dynamic isr did not install successfully");
#endif

	TC_PRINT("vector(%d)\n", vector_num);
	zassert_true(vector_num > 0,
			"irq connect dynamic failed");

	zassert_equal(handler_has_run, 0,
			"handler has run before interrupt trigger");

	irq_enable(TEST_IRQ_DYN_LINE);

	trigger_irq(vector_num);

	zassert_equal(handler_has_run, 1,
			"interrupt triggered but handler has not run(%d)",
			handler_has_run);

	/**TESTPOINT: pass word-sized parameter to interrupt */
	zassert_equal(handler_test_result, ISR_DYN_ARG,
			"parameter(0x%lx) in handler is not correct",
			handler_test_result);

	trigger_irq(vector_num);

	/**TESTPOINT: interrupt triggered again */
	zassert_equal(handler_has_run, 2,
			"interrupt triggered but handler has not run(%d)",
			handler_has_run);

}
#endif /* CONFIG_GEN_SW_ISR_TABLE */
