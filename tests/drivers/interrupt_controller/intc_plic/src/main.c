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
