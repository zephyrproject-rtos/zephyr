/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/atomic.h>

/**
 * @addtogroup kernel_common_tests
 * @{
 */

/**
 * @brief Verify automic functionalities
 *
 * @see atomic_cas(), atomic_add(), atomic_sub(),
 * atomic_inc(), atomic_dec(), atomic_get(), atomic_set(),
 * atomic_clear(), atomic_or(), atomic_and(), atomic_xor(),
 * atomic_nand(), atomic_test_bit(), atomic_test_and_clear_bit(),
 * atomic_test_and_set_bit(), atomic_clear_bit(), atomic_set_bit()
 */
void test_atomic(void)
{
	int i;

	atomic_t target, orig;
	atomic_val_t value;
	atomic_val_t oldvalue;

	target = 4;
	value = 5;
	oldvalue = 6;

	/* atomic_cas() */
	zassert_true((atomic_cas(&target, oldvalue, value) == 0), "atomic_cas");
	target = 6;
	zassert_true((atomic_cas(&target, oldvalue, value) == 1), "atomic_cas");
	zassert_true((target == value), "atomic_cas");

	/* atomic_add() */
	target = 1;
	value = 2;
	zassert_true((atomic_add(&target, value) == 1), "atomic_add");
	zassert_true((target == 3), "atomic_add");

	/* atomic_sub() */
	target = 10;
	value = 2;
	zassert_true((atomic_sub(&target, value) == 10), "atomic_sub");
	zassert_true((target == 8), "atomic_sub");

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

	/* atomic_set() */
	target = 42;
	value = 77;
	zassert_true((atomic_set(&target, value) == 42), "atomic_set");
	zassert_true((target == value), "atomic_set");

	/* atomic_clear() */
	target = 100;
	zassert_true((atomic_clear(&target) == 100), "atomic_clear");
	zassert_true((target == 0), "atomic_clear");

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
}
/**
 * @}
 */
