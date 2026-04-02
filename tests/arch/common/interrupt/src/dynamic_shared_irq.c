/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_shared_irq.h"

struct shared_irq_fixture {
	unsigned int irq1;
	unsigned int irq2;
	unsigned int irq1_table_idx;
	unsigned int irq2_table_idx;
	unsigned int irq_priority;
};

static struct shared_irq_fixture fixture;

static void reset_test_vector(void)
{
	int i;

	for (i = 0; i < TEST_VECTOR_SIZE; i++) {
		test_vector[i] = 0;
	}
}

static void dynamic_shared_irq_suite_after(void *data)
{
	ARG_UNUSED(data);

	/* note: no need to check the state of the SW ISR tables after
	 * all these disconnect operations. If there's something wrong
	 * it should be detected by dynamic_shared_irq_suite_before().
	 */
	arch_irq_disconnect_dynamic(fixture.irq1, fixture.irq_priority,
				    test_isr_0, 0, 0);
	arch_irq_disconnect_dynamic(fixture.irq1, fixture.irq_priority,
				    test_isr_1, (void *)1, 0);
	arch_irq_disconnect_dynamic(fixture.irq2, fixture.irq_priority,
				    test_isr_2, (void *)2, 0);
}

static void dummy_isr(const void *data)
{
	ARG_UNUSED(data);

	test_vector[0] = TEST_DUMMY_ISR_VAL;
}

static unsigned int get_irq_slot(unsigned int start)
{
	unsigned int i, table_idx;

	for (i = start; i <= CONFIG_GEN_IRQ_START_VECTOR + CONFIG_NUM_IRQS - 1; i++) {
		table_idx = i - CONFIG_GEN_IRQ_START_VECTOR;

		if (_sw_isr_table[table_idx].isr == &z_irq_spurious) {
			test_vector[0] = 0;

			/* check to see if we can trigger this IRQ */
			arch_irq_connect_dynamic(i, IRQ_PRIORITY, dummy_isr,
						 NULL, IRQ_FLAGS);
			irq_enable(i);
			trigger_irq(i);

			/* wait a bit */
			k_busy_wait(100);

			if (test_vector[0] == TEST_DUMMY_ISR_VAL) {
				/* found a valid INTID */
				irq_disable(i);

				arch_irq_disconnect_dynamic(i, IRQ_PRIORITY,
							    dummy_isr, NULL, 0);
				return i;
			}
		}
	}

	return TEST_INVALID_IRQ;
}

static void *dynamic_shared_irq_suite_setup(void)
{
	fixture.irq1 = get_irq_slot(CONFIG_GEN_IRQ_START_VECTOR);
	zassert_true(fixture.irq1 != TEST_INVALID_IRQ,
		     "no suitable value found for irq1");
	fixture.irq2 = get_irq_slot(fixture.irq1 + 1);
	zassert_true(fixture.irq2 != TEST_INVALID_IRQ,
		     "no suitable value found for irq2");
	fixture.irq_priority = IRQ_PRIORITY;

	fixture.irq1_table_idx = fixture.irq1 - CONFIG_GEN_IRQ_START_VECTOR;
	fixture.irq2_table_idx = fixture.irq2 - CONFIG_GEN_IRQ_START_VECTOR;

	return NULL;
}

#if defined(CONFIG_RISCV_RESERVED_IRQ_ISR_TABLES_OFFSET)
#define TABLE_OFFSET CONFIG_RISCV_RESERVED_IRQ_ISR_TABLES_OFFSET
#else
#define TABLE_OFFSET 0
#endif

static void dynamic_shared_irq_suite_before(void *data)
{
	ARG_UNUSED(data);

	arch_irq_connect_dynamic(fixture.irq1, fixture.irq_priority,
				 test_isr_0, 0, IRQ_FLAGS);

	zassert_true(_sw_isr_table[fixture.irq1_table_idx + TABLE_OFFSET].isr == test_isr_0,
		     "wrong _sw_isr_table ISR at irq1");
	zassert_true(!_sw_isr_table[fixture.irq1_table_idx + TABLE_OFFSET].arg,
		     "wrong _sw_isr_table argument at irq1");
	zassert_true(!z_shared_sw_isr_table[fixture.irq1_table_idx + TABLE_OFFSET].client_num,
		     "wrong client number at irq1");

	arch_irq_connect_dynamic(fixture.irq1, fixture.irq_priority,
				 test_isr_1, (void *)1, IRQ_FLAGS);

	zassert_true(_sw_isr_table[fixture.irq1_table_idx + TABLE_OFFSET].isr == z_shared_isr,
		     "wrong _sw_isr_table ISR at irq1");
	zassert_true(_sw_isr_table[fixture.irq1_table_idx + TABLE_OFFSET].arg ==
		     &z_shared_sw_isr_table[fixture.irq1_table_idx + TABLE_OFFSET],
		     "wrong _sw_isr_table argument at irq1");
	zassert_true(z_shared_sw_isr_table[fixture.irq1_table_idx + TABLE_OFFSET].client_num == 2,
		     "wrong client number at irq1");

	zassert_true(client_exists_at_index(test_isr_0, 0, fixture.irq1_table_idx + TABLE_OFFSET,
					    0),
		     "unexpected client data for irq1, index 0");
	zassert_true(client_exists_at_index(test_isr_1, (void *)1,
		     fixture.irq1_table_idx + TABLE_OFFSET, 1),
		     "unexpected client data for irq1, index 1");

	arch_irq_connect_dynamic(fixture.irq2, fixture.irq_priority,
				 test_isr_2, (void *)2, IRQ_FLAGS);

	zassert_true(_sw_isr_table[fixture.irq2_table_idx + TABLE_OFFSET].isr == test_isr_2,
		     "wrong _sw_isr_table ISR at irq2");
	zassert_true(_sw_isr_table[fixture.irq2_table_idx + TABLE_OFFSET].arg == (void *)2,
		     "wrong _sw_isr_table argument at irq2");
	zassert_true(!z_shared_sw_isr_table[fixture.irq2_table_idx + TABLE_OFFSET].client_num,
		     "wrong client number at irq2");

	reset_test_vector();
}

/**
 * @brief Test writing to a vector with a shared interrupt
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details This tests if interrupts are dynamically shared successfully
 * (i.e: multiple ISR/arg pairs are called whenever the interrupt
 * they were registered for is triggered).
 */
ZTEST(shared_irq_feature, test_dynamic_shared_irq_write)
{
	int i;

	irq_enable(fixture.irq1);
	irq_enable(fixture.irq2);

	trigger_irq(fixture.irq1);
	trigger_irq(fixture.irq2);

	/* wait 5ms before checking the results */
	k_busy_wait(5000);

	for (i = 0; i < TEST_VECTOR_SIZE; i++) {
		zassert_true(test_vector[i] == result_vector[i],
			     "wrong test_vector value at %d: 0x%x vs 0x%x",
			     i, test_vector[i], result_vector[i]);
	}

	irq_disable(fixture.irq1);
	irq_disable(fixture.irq2);
}

/**
 * @brief Test writing to a vector after an ISR/arg disconnect.
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details This tests if ISR/arg pairs are disconnected successfully
 * and the interrupts are "unshared" whenever a single ISR/arg pair is
 * left.
 */
ZTEST(shared_irq_feature, test_dynamic_shared_irq_disconnect_write)
{
	int i;

	/* remove test_isr_0/NULL pair. After this statement we expect
	 * irq1 to be unshared.
	 */
	arch_irq_disconnect_dynamic(fixture.irq1, fixture.irq_priority,
				    test_isr_0, 0, 0);

	zassert_true(_sw_isr_table[fixture.irq1_table_idx + TABLE_OFFSET].isr == test_isr_1,
		     "wrong _sw_isr_table ISR at irq1");
	zassert_true(_sw_isr_table[fixture.irq1_table_idx + TABLE_OFFSET].arg == (void *)1,
		     "wrong _sw_isr_table arg at irq1");
	zassert_true(!z_shared_sw_isr_table[fixture.irq1_table_idx + TABLE_OFFSET].client_num,
		     "wrong client number at irq1");

	irq_enable(fixture.irq1);
	trigger_irq(fixture.irq1);

	/* wait 5ms before checking the results */
	k_busy_wait(5000);

	for (i = 0; i < TEST_VECTOR_SIZE; i++) {
		if (i == 1) {
			zassert_true(test_vector[i] == result_vector[i],
				     "wrong test_vector at %d: 0x%x vs 0x%x",
				     i, test_vector[i], result_vector[i]);
			continue;
		}

		zassert_true(!test_vector[i],
			     "wrong test_vector value at %d: 0x%x vs 0x%x",
			     i, test_vector[i], result_vector[i]);
	}

	irq_disable(fixture.irq1);
}

ZTEST_SUITE(shared_irq_feature, NULL,
	    dynamic_shared_irq_suite_setup,
	    dynamic_shared_irq_suite_before,
	    dynamic_shared_irq_suite_after,
	    NULL);
