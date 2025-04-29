/*
 * Copyright (c) 2024 CISPA Helmholtz Center for Information Security
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This extension tests a relocation edge case in RISC-V:
 * U-type instructions in conjunction with I-type/S-type instructions can be used to
 * relocate symbols within a 32-bit range from the PC (medany code model) or 0
 * (medlow code model).
 * The compiler usually emits the U-type instructions and I-type/S-type instructions in sequence.
 * However, this is not guaranteed.
 * The accompanying assembly listing generates a scenario in which this assumption does NOT hold
 * and tests that the llext loader can handle it.
 */

#include <stdbool.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/ztest_assert.h>

extern int _riscv_edge_case_non_paired_hi20_lo12(void);


#define DATA_SEGMENT_SYMBOL_INITIAL 21
#define DATA_SEGMENT_SYMBOL_EXPECTED (DATA_SEGMENT_SYMBOL_INITIAL+42)

/* changed from the assembly script */
volatile int _data_segment_symbol = DATA_SEGMENT_SYMBOL_INITIAL;


void test_entry(void)
{
	int ret_value;

	ret_value = _riscv_edge_case_non_paired_hi20_lo12();

	zassert_equal(ret_value, DATA_SEGMENT_SYMBOL_INITIAL);

	zassert_equal(_data_segment_symbol, DATA_SEGMENT_SYMBOL_EXPECTED);
}
EXPORT_SYMBOL(test_entry);
