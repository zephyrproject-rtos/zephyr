/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <interrupt_util.h>

/*
 * Run the nested interrupt test for the supported platforms only.
 */
#if defined(CONFIG_CPU_CORTEX_M) || defined(CONFIG_ARC) || \
	defined(CONFIG_GIC)
#define TEST_NESTED_ISR
#endif

#define DURATION	5

#define ISR0_TOKEN	0xDEADBEEF
#define ISR1_TOKEN	0xCAFEBABE

/*
 * This test uses two IRQ lines selected within the range of available IRQs on
 * the target SoC.  These IRQs are platform and interrupt controller-specific,
 * and must be specified for every supported platform.
 *
 * In terms of priority, the IRQ1 is triggered from the ISR of the IRQ0;
 * therefore, the priority of IRQ1 must be greater than that of the IRQ0.
 */
#if defined(CONFIG_CPU_CORTEX_M)
/*
 * For Cortex-M NVIC, unused and available IRQs are automatically detected when
 * the test is run.
 *
 * The IRQ priorities start at 1 because the priority 0 is reserved for the
 * SVCall exception and Zero-Latency IRQs (see `_EXCEPTION_RESERVED_PRIO`).
 */
#define IRQ0_PRIO	2
#define IRQ1_PRIO	1
#elif defined(CONFIG_GIC)
/*
 * For the platforms that use the ARM GIC, use the SGI (software generated
 * interrupt) lines 14 and 15 for testing.
 */
#define IRQ0_LINE	14
#define IRQ1_LINE	15

/*
 * Choose lower prio for IRQ0 and higher priority for IRQ1
 * Minimum legal value of GICC BPR is '3' ie  <gggg.ssss>
 * Hence choosing default priority and highest possible priority
 * '0x0' as the priorities so that the preemption rule applies
 * generically to all GIC versions and security states.
 */
#define IRQ0_PRIO	IRQ_DEFAULT_PRIORITY
#define IRQ1_PRIO	0x0
#else
/*
 * For all the other platforms, use the last two available IRQ lines for
 * testing.
 */
#define IRQ0_LINE	(CONFIG_NUM_IRQS - 1)
#define IRQ1_LINE	(CONFIG_NUM_IRQS - 2)

#define IRQ0_PRIO	1
#define IRQ1_PRIO	0
#endif

#ifdef TEST_NESTED_ISR
static uint32_t irq_line_0;
static uint32_t irq_line_1;

static uint32_t isr0_result;
static uint32_t isr1_result;

void isr1(const void *param)
{
	ARG_UNUSED(param);

	printk("isr1: Enter\n");

	/* Set verification token */
	isr1_result = ISR1_TOKEN;

	printk("isr1: Leave\n");
}

void isr0(const void *param)
{
	ARG_UNUSED(param);

	printk("isr0: Enter\n");

	/* Set verification token */
	isr0_result = ISR0_TOKEN;

	/* Trigger nested IRQ 1 */
	trigger_irq(irq_line_1);

	/* Wait for interrupt */
	k_busy_wait(MS_TO_US(DURATION));

	/* Validate nested ISR result token */
	zassert_equal(isr1_result, ISR1_TOKEN, "isr1 did not execute");

	printk("isr0: Leave\n");
}

/**
 * @brief Test interrupt nesting
 *
 * @ingroup kernel_interrupt_tests
 *
 * This routine tests the interrupt nesting feature, which allows an ISR to be
 * preempted in mid-execution if a higher priority interrupt is signaled. The
 * lower priority ISR resumes execution once the higher priority ISR has
 * completed its processing.
 *
 * The expected control flow for this test is as follows:
 *
 * 1. [thread] Trigger IRQ 0 (lower priority)
 * 2. [isr0] Set ISR 0 result token and trigger IRQ 1 (higher priority)
 * 3. [isr1] Set ISR 1 result token and return
 * 4. [isr0] Validate ISR 1 result token and return
 * 5. [thread] Validate ISR 0 result token
 */
ZTEST(interrupt_feature, test_nested_isr)
{
	/* Resolve test IRQ line numbers */
#if defined(CONFIG_CPU_CORTEX_M)
	irq_line_0 = get_available_nvic_line(CONFIG_NUM_IRQS);
	irq_line_1 = get_available_nvic_line(irq_line_0);
#else
	irq_line_0 = IRQ0_LINE;
	irq_line_1 = IRQ1_LINE;
#endif

	/* Connect and enable test IRQs */
	arch_irq_connect_dynamic(irq_line_0, IRQ0_PRIO, isr0, NULL, 0);
	arch_irq_connect_dynamic(irq_line_1, IRQ1_PRIO, isr1, NULL, 0);

	irq_enable(irq_line_0);
	irq_enable(irq_line_1);

	/* Trigger test IRQ 0 */
	trigger_irq(irq_line_0);

	/* Wait for interrupt */
	k_busy_wait(MS_TO_US(DURATION));

	/* Validate ISR result token */
	zassert_equal(isr0_result, ISR0_TOKEN, "isr0 did not execute");
}
#else
ZTEST(interrupt_feature, test_nested_isr)
{
	ztest_test_skip();
}
#endif /* TEST_NESTED_ISR */
