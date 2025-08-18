/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_shared_irq.h"

#define GIC_IRQ1 10
#define GIC_IRQ2 11

/**
 * @brief Test writing to a vector using static shared interrupts.
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details This tests if interrupts are statically shared successfully
 * (i.e: multiple ISR/arg pairs are called whenever the interrupt they
 * were registered for is triggered).
 */
ZTEST(interrupt_feature, test_static_shared_irq_write)
{
	/* note: this test is very brittle since it requires that
	 * the chosen interrupt lines be unused for all of the
	 * testing platforms. Failing to meet this requirement
	 * leads to build failures due to the number of clients
	 * exceeding the limit. Still, it's important to test that
	 * the static shared interrupts work properly. As such,
	 * this test shall be restricted to a single platform, thus
	 * decreasing the risk of build errors appearing due to the
	 * chosen interrupts being used.
	 */
#ifndef CONFIG_BOARD_QEMU_CORTEX_A53
	ztest_test_skip();
#else
	int i;

	IRQ_CONNECT(GIC_IRQ1, IRQ_PRIORITY, test_isr_0, 0, IRQ_FLAGS);
	IRQ_CONNECT(GIC_IRQ1, IRQ_PRIORITY, test_isr_1, (void *)1, IRQ_FLAGS);
	IRQ_CONNECT(GIC_IRQ2, IRQ_PRIORITY, test_isr_2, (void *)2, IRQ_FLAGS);

	zassert_true(_sw_isr_table[GIC_IRQ1].isr == z_shared_isr,
		     "wrong _sw_isr_table ISR at GIC_IRQ1");
	zassert_true(_sw_isr_table[GIC_IRQ2].isr == test_isr_2,
		     "wrong _sw_isr_table ISR at GIC_IRQ1");

	zassert_true(_sw_isr_table[GIC_IRQ1].arg ==
		     &z_shared_sw_isr_table[GIC_IRQ1],
		     "wrong _sw_isr_table arg at GIC_IRQ1");
	zassert_true(_sw_isr_table[GIC_IRQ2].arg == (void *)2,
		     "wrong _sw_isr_table arg at GIC_IRQ2");

	zassert_true(z_shared_sw_isr_table[GIC_IRQ1].client_num == 2,
		     "wrong client number for GIC_IRQ1");
	zassert_true(!z_shared_sw_isr_table[GIC_IRQ2].client_num,
		     "wrong client number for GIC_IRQ2");

	zassert_true(client_exists_at_index(test_isr_0, 0, GIC_IRQ1,
					    TEST_INVALID_IDX),
		     "test_isr_0 not a client for GIC_IRQ1");
	zassert_true(client_exists_at_index(test_isr_1, (void *)1, GIC_IRQ1,
					    TEST_INVALID_IDX),
		     "test_isr_1 not a client for GIC_IRQ1");

	irq_enable(GIC_IRQ1);
	irq_enable(GIC_IRQ2);

	trigger_irq(GIC_IRQ1);
	trigger_irq(GIC_IRQ2);

	/* wait 5ms before checking the results */
	k_busy_wait(5000);

	for (i = 0; i < TEST_VECTOR_SIZE; i++) {
		zassert_true(test_vector[i] == result_vector[i],
			     "wrong test_vector value at %d: 0x%x vs 0x%x",
			     i, test_vector[i], result_vector[i]);
	}

	irq_disable(GIC_IRQ1);
	irq_disable(GIC_IRQ2);

#ifdef CONFIG_DYNAMIC_INTERRUPTS
	/* if dynamic interrupts are enabled this will restore the _sw_isr_table
	 * entries for GIC_IRQ1 and GIC_IRQ2 to their default values (NULL,
	 * z_irq_spurious). In turn, this will increase the probability of
	 * dynamic_shared_irq.c's get_irq_slot() being able to find an available
	 * slot.
	 */
	arch_irq_disconnect_dynamic(GIC_IRQ1, IRQ_PRIORITY, test_isr_0, 0, 0);
	arch_irq_disconnect_dynamic(GIC_IRQ1, IRQ_PRIORITY, test_isr_1, (void *)1, 0);
	arch_irq_disconnect_dynamic(GIC_IRQ2, IRQ_PRIORITY, test_isr_2, (void *)2, 0);
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

#endif /* CONFIG_BOARD_QEMU_CORTEX_A53 */
}
