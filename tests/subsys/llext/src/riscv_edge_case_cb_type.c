/*
 * Copyright (c) 2024 CISPA Helmholtz Center for Information Security
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This extension tests a relocation edge case in RISC-V:
 * Immediates in branch/jump-type instructions are signed-extended.
 * Thus, a jump with a negative offset can have a greater jump target than
 * a jump with a positive offset.
 * A compressed branch (cb-type) instruction is used to trigger the edge case.
 * It has a 9-bit immediate (with an implicit LSB of 0), allowing it to jump
 * 256 bytes backward and 254 bytes forward.
 */

#include <stdbool.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/ztest_assert.h>

extern int _riscv_edge_case_cb_trigger_forward(void);
extern int _riscv_edge_case_cb_trigger_backward(void);

void test_entry(void)
{
	int test_ok;

	test_ok = _riscv_edge_case_cb_trigger_forward();
	zassert_equal(test_ok, 0x1);

	test_ok = _riscv_edge_case_cb_trigger_backward();
	zassert_equal(test_ok, 0x1);
}
EXPORT_SYMBOL(test_entry);
