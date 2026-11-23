/*
 * Copyright (c) 2024 NextSilicon LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/arch/riscv/atomic.h>

/**
 * @brief Verify RISC-V specific atomic functionalities
 * @details
 * Test Objective:
 * - Test if the RISC-V atomic instructions API is correct.
 *
 * Test Procedure:
 * - Call the API interface of the following atomic operations in turn,
 * decision is based on function return value and target operands.
 * - atomic_swap()
 * - atomic_max()
 * - atomic_min()
 *
 * Expected Test Result:
 * - The function return value and target operands are correct.
 *
 * Pass/Fail Criteria:
 * - Successful if all check points in the test procedure have passed, failure otherwise.
 */

#define ATOMIC_WORD(val_if_64, val_if_32)                                                          \
	((atomic_t)((sizeof(void *) == sizeof(uint64_t)) ? (val_if_64) : (val_if_32)))

ZTEST_USER(riscv_atomic, test_atomic)
{
	atomic_t target;
	atomic_val_t value;
	unsigned long utarget, uvalue;

	zassert_equal(sizeof(atomic_t), ATOMIC_WORD(sizeof(uint64_t), sizeof(uint32_t)),
		      "sizeof(atomic_t)");

	/* atomic_swap */
	target = 21;
	value = 7;
	zassert_true((atomic_swap(&target, value) == 21), "atomic_swap");
	zassert_true((target == 7), "atomic_swap");

	/* atomic_max */
	target = 5;
	value = -8;
	zassert_true((atomic_max(&target, value) == 5), "atomic_max");
	zassert_true((target == 5), "atomic_max");

	/* atomic_min */
	target = 5;
	value = -8;
	zassert_true((atomic_min(&target, value) == 5), "atomic_min");
	zassert_true((target == -8), "atomic_min");

	/* atomic_max unsigned */
	utarget = 5;
	uvalue = ATOMIC_WORD(0xffffffff00000000, 0xffff0000);
	zassert_true((atomic_maxu(&utarget, uvalue) == 5), "atomic_maxu");
	zassert_true((utarget == ATOMIC_WORD(0xffffffff00000000, 0xffff0000)), "atomic_maxu");

	/* atomic_min unsigned */
	utarget = 5;
	uvalue = ATOMIC_WORD(0xffffffff00000000, 0xffff0000);
	zassert_true((atomic_minu(&utarget, uvalue) == 5), "atomic_minu");
	zassert_true((utarget == 5), "atomic_minu");
}

ZTEST_SUITE(riscv_atomic, NULL, NULL, NULL, NULL, NULL);
