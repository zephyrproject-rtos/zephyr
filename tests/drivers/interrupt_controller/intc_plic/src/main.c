/*
 * Copyright (c) 2023 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/ztest.h>

uint32_t local_irq_to_reg_index(uint32_t local_irq);
uint32_t local_irq_to_reg_offset(uint32_t local_irq);

ZTEST_SUITE(intc_plic, NULL, NULL, NULL, NULL, NULL);

/* Test calculating the register index from a local IRQ number */
ZTEST(intc_plic, test_local_irq_to_reg_index)
{
	zassert_equal(0, local_irq_to_reg_index(0x1f));
	zassert_equal(1, local_irq_to_reg_index(0x20));
	zassert_equal(1, local_irq_to_reg_index(0x3f));
	zassert_equal(2, local_irq_to_reg_index(0x40));
}

/* Test calculating the register offset from a local IRQ number */
ZTEST(intc_plic, test_local_irq_to_reg_offset)
{
	zassert_equal(0, local_irq_to_reg_offset(0x1f));
	zassert_equal(4, local_irq_to_reg_offset(0x20));
	zassert_equal(4, local_irq_to_reg_offset(0x3f));
	zassert_equal(8, local_irq_to_reg_offset(0x40));
}

ZTEST(intc_plic, test_hart_context_mapping)
{
	extern const uint32_t plic_hart_contexts_0[];

	if (!IS_ENABLED(CONFIG_TEST_INTC_PLIC_ALT_MAPPING)) {
		/* Based on the default qemu_riscv64 devicetree */
		zassert_equal(plic_hart_contexts_0[0], 0);
		zassert_equal(plic_hart_contexts_0[1], 2);
		zassert_equal(plic_hart_contexts_0[2], 4);
		zassert_equal(plic_hart_contexts_0[3], 6);
		zassert_equal(plic_hart_contexts_0[4], 8);
		zassert_equal(plic_hart_contexts_0[5], 10);
		zassert_equal(plic_hart_contexts_0[6], 12);
		zassert_equal(plic_hart_contexts_0[7], 14);
	} else {
		/* Based on the definition in the `alt_mapping.overlay` */
		zassert_equal(plic_hart_contexts_0[0], 0);
		zassert_equal(plic_hart_contexts_0[1], 1);
		zassert_equal(plic_hart_contexts_0[2], 3);
		zassert_equal(plic_hart_contexts_0[3], 5);
		zassert_equal(plic_hart_contexts_0[4], 7);
		zassert_equal(plic_hart_contexts_0[5], 9);
		zassert_equal(plic_hart_contexts_0[6], 11);
		zassert_equal(plic_hart_contexts_0[7], 13);
	}
}
