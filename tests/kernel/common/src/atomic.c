/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/atomic.h>

/* an example of the number of atomic bit in an array */
#define NUM_FLAG_BITS 100

/**
 * @addtogroup kernel_common_tests
 * @{
 */

/**
 * @brief Verify atomic functionalities
 * @details
 * Test Objective:
 * - Test the function of the atomic operation API is correct.
 *
 * Test techniques:
 * - Dynamic analysis and testing
 * - Functional and black box testing
 * - Interface testing
 *
 * Prerequisite Conditions:
 * - N/A
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Call the API interface of the following atomic operations in turn,
 * judge the change of function return value and target operands.
 * - atomic_cas()
 * - atomic_ptr_cas()
 * - atomic_add()
 * - atomic_sub()
 * - atomic_inc()
 * - atomic_dec()
 * - atomic_get()
 * - atomic_ptr_get()
 * - atomic_set()
 * - atomic_ptr_set()
 * - atomic_clear()
 * - atomic_ptr_clear()
 * - atomic_or()
 * - atomic_xor()
 * - atomic_and()
 * - atomic_nand()
 * - atomic_test_bit()
 * - atomic_test_and_clear_bit()
 * - atomic_test_and_set_bit()
 * - atomic_clear_bit()
 * - atomic_set_bit()
 * - atomic_set_bit_to()
 * - ATOMIC_DEFINE
 *
 * Expected Test Result:
 * - The change of function return value and target operands is correct.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed, otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @see atomic_cas(), atomic_add(), atomic_sub(),
 * atomic_inc(), atomic_dec(), atomic_get(), atomic_set(),
 * atomic_clear(), atomic_or(), atomic_and(), atomic_xor(),
 * atomic_nand(), atomic_test_bit(), atomic_test_and_clear_bit(),
 * atomic_test_and_set_bit(), atomic_clear_bit(), atomic_set_bit(),
 * ATOMIC_DEFINE
 *
 * @ingroup kernel_common_tests
 */
void test_atomic(void)
{
	int i;

	atomic_t target, orig;
	atomic_ptr_t ptr_target;
	atomic_val_t value;
	atomic_val_t oldvalue;
	void *ptr_value, *old_ptr_value;

	ATOMIC_DEFINE(flag_bits, NUM_FLAG_BITS) = {0};

	target = 4;
	value = 5;
	oldvalue = 6;

	/* atomic_cas() */
	zassert_false(atomic_cas(&target, oldvalue, value), "atomic_cas");
	target = 6;
	zassert_true(atomic_cas(&target, oldvalue, value), "atomic_cas");
	zassert_true((target == value), "atomic_cas");

	/* atomic_ptr_cas() */
	ptr_target = (atomic_ptr_t)4;
	ptr_value = (void *)5;
	old_ptr_value = (void *)6;
	zassert_false(atomic_ptr_cas(&ptr_target, old_ptr_value, ptr_value),
		      "atomic_ptr_cas");
	ptr_target = (void *)6;
	zassert_true(atomic_ptr_cas(&ptr_target, old_ptr_value, ptr_value),
		     "atomic_ptr_cas");
	zassert_true((ptr_target == ptr_value), "atomic_ptr_cas");

	/* atomic_add() */
	target = 1;
	value = 2;
	zassert_true((atomic_add(&target, value) == 1), "atomic_add");
	zassert_true((target == 3), "atomic_add");
	/* Test the atomic_add() function parameters can be negative */
	target = 2;
	value = -4;
	zassert_true((atomic_add(&target, value) == 2), "atomic_add");
	zassert_true((target == -2), "atomic_add");

	/* atomic_sub() */
	target = 10;
	value = 2;
	zassert_true((atomic_sub(&target, value) == 10), "atomic_sub");
	zassert_true((target == 8), "atomic_sub");
	/* Test the atomic_sub() function parameters can be negative */
	target = 5;
	value = -4;
	zassert_true((atomic_sub(&target, value) == 5), "atomic_sub");
	zassert_true((target == 9), "atomic_sub");

	/* atomic_inc() */
	target = 5;
	zassert_true((atomic_inc(&target) == 5), "atomic_inc");
	zassert_true((target == 6), "atomic_inc");

	/* atomic_dec() */
	target = 2;
	zassert_true((atomic_dec(&target) == 2), "atomic_dec");
	zassert_true((target == 1), "atomic_dec");

	/* atomic_get() */
	target = 50;
	zassert_true((atomic_get(&target) == 50), "atomic_get");

	/* atomic_ptr_get() */
	ptr_target = (atomic_ptr_t)50;
	zassert_true((atomic_ptr_get(&ptr_target) == (void *)50),
		     "atomic_ptr_get");

	/* atomic_set() */
	target = 42;
	value = 77;
	zassert_true((atomic_set(&target, value) == 42), "atomic_set");
	zassert_true((target == value), "atomic_set");

	/* atomic_ptr_set() */
	ptr_target = (atomic_ptr_t)42;
	ptr_value = (void *)77;
	zassert_true((atomic_ptr_set(&ptr_target, ptr_value) == (void *)42),
		     "atomic_ptr_set");
	zassert_true((ptr_target == ptr_value), "atomic_ptr_set");

	/* atomic_clear() */
	target = 100;
	zassert_true((atomic_clear(&target) == 100), "atomic_clear");
	zassert_true((target == 0), "atomic_clear");

	/* atomic_ptr_clear() */
	ptr_target = (atomic_ptr_t)100;
	zassert_true((atomic_ptr_clear(&ptr_target) == (void *)100),
		     "atomic_ptr_clear");
	zassert_true((ptr_target == NULL), "atomic_ptr_clear");

	/* atomic_or() */
	target = 0xFF00;
	value  = 0x0F0F;
	zassert_true((atomic_or(&target, value) == 0xFF00), "atomic_or");
	zassert_true((target == 0xFF0F), "atomic_or");

	/* atomic_xor() */
	target = 0xFF00;
	value  = 0x0F0F;
	zassert_true((atomic_xor(&target, value) == 0xFF00), "atomic_xor");
	zassert_true((target == 0xF00F), "atomic_xor");

	/* atomic_and() */
	target = 0xFF00;
	value  = 0x0F0F;
	zassert_true((atomic_and(&target, value) == 0xFF00), "atomic_and");
	zassert_true((target == 0x0F00), "atomic_and");


	/* atomic_nand() */
	target = 0xFF00;
	value  = 0x0F0F;
	zassert_true((atomic_nand(&target, value) == 0xFF00), "atomic_nand");
	zassert_true((target == 0xFFFFF0FF), "atomic_nand");

	/* atomic_test_bit() */
	for (i = 0; i < 32; i++) {
		target = 0x0F0F0F0F;
		zassert_true(!!(atomic_test_bit(&target, i) == !!(target & (1 << i))),
			     "atomic_test_bit");
	}

	/* atomic_test_and_clear_bit() */
	for (i = 0; i < 32; i++) {
		orig = 0x0F0F0F0F;
		target = orig;
		zassert_true(!!(atomic_test_and_clear_bit(&target, i)) == !!(orig & (1 << i)),
			     "atomic_test_and_clear_bit");
		zassert_true(target == (orig & ~(1 << i)), "atomic_test_and_clear_bit");
	}

	/* atomic_test_and_set_bit() */
	for (i = 0; i < 32; i++) {
		orig = 0x0F0F0F0F;
		target = orig;
		zassert_true(!!(atomic_test_and_set_bit(&target, i)) == !!(orig & (1 << i)),
			     "atomic_test_and_set_bit");
		zassert_true(target == (orig | (1 << i)), "atomic_test_and_set_bit");
	}

	/* atomic_clear_bit() */
	for (i = 0; i < 32; i++) {
		orig = 0x0F0F0F0F;
		target = orig;
		atomic_clear_bit(&target, i);
		zassert_true(target == (orig & ~(1 << i)), "atomic_clear_bit");
	}

	/* atomic_set_bit() */
	for (i = 0; i < 32; i++) {
		orig = 0x0F0F0F0F;
		target = orig;
		atomic_set_bit(&target, i);
		zassert_true(target == (orig | (1 << i)), "atomic_set_bit");
	}

	/* atomic_set_bit_to(&target, i, false) */
	for (i = 0; i < 32; i++) {
		orig = 0x0F0F0F0F;
		target = orig;
		atomic_set_bit_to(&target, i, false);
		zassert_true(target == (orig & ~(1 << i)), "atomic_set_bit_to");
	}

	/* atomic_set_bit_to(&target, i, true) */
	for (i = 0; i < 32; i++) {
		orig = 0x0F0F0F0F;
		target = orig;
		atomic_set_bit_to(&target, i, true);
		zassert_true(target == (orig | (1 << i)), "atomic_set_bit_to");
	}

	/* ATOMIC_DEFINE */
	for (i = 0; i < NUM_FLAG_BITS; i++) {
		atomic_set_bit(flag_bits, i);
		zassert_true(!!atomic_test_bit(flag_bits, i) == !!(1),
			"Failed to set a single bit in an array of atomic variables");
		atomic_clear_bit(flag_bits, i);
		zassert_true(!!atomic_test_bit(flag_bits, i) == !!(0),
			"Failed to clear a single bit in an array of atomic variables");
	}
}
/**
 * @}
 */
