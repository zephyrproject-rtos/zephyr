/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ztest.h>
#include <atomic.h>

void atomic_test(void)
{
	int i;

	atomic_t target, orig;
	atomic_val_t value;
	atomic_val_t oldvalue;

	target = 4;
	value = 5;
	oldvalue = 6;

	/* atomic_cas() */
	assert_true((atomic_cas(&target, oldvalue, value) == 0), "atomic_cas");
	target = 6;
	assert_true((atomic_cas(&target, oldvalue, value) == 1), "atomic_cas");
	assert_true((target == value), "atomic_cas");

	/* atomic_add() */
	target = 1;
	value = 2;
	assert_true((atomic_add(&target, value) == 1), "atomic_add");
	assert_true((target == 3), "atomic_add");

	/* atomic_sub() */
	target = 10;
	value = 2;
	assert_true((atomic_sub(&target, value) == 10), "atomic_sub");
	assert_true((target == 8), "atomic_sub");

	/* atomic_inc() */
	target = 5;
	assert_true((atomic_inc(&target) == 5), "atomic_inc");
	assert_true((target == 6), "atomic_inc");

	/* atomic_dec() */
	target = 2;
	assert_true((atomic_dec(&target) == 2), "atomic_dec");
	assert_true((target == 1), "atomic_dec");

	/* atomic_get() */
	target = 50;
	assert_true((atomic_get(&target) == 50), "atomic_get");

	/* atomic_set() */
	target = 42;
	value = 77;
	assert_true((atomic_set(&target, value) == 42), "atomic_set");
	assert_true((target == value), "atomic_set");

	/* atomic_clear() */
	target = 100;
	assert_true((atomic_clear(&target) == 100), "atomic_clear");
	assert_true((target == 0), "atomic_clear");

	/* atomic_or() */
	target = 0xFF00;
	value  = 0x0F0F;
	assert_true((atomic_or(&target, value) == 0xFF00), "atomic_or");
	assert_true((target == 0xFF0F), "atomic_or");

	/* atomic_xor() */
	target = 0xFF00;
	value  = 0x0F0F;
	assert_true((atomic_xor(&target, value) == 0xFF00), "atomic_xor");
	assert_true((target == 0xF00F), "atomic_xor");

	/* atomic_and() */
	target = 0xFF00;
	value  = 0x0F0F;
	assert_true((atomic_and(&target, value) == 0xFF00), "atomic_and");
	assert_true((target == 0x0F00), "atomic_and");


	/* atomic_nand() */
	target = 0xFF00;
	value  = 0x0F0F;
	assert_true((atomic_nand(&target, value) == 0xFF00), "atomic_nand");
	assert_true((target == 0xFFFFF0FF), "atomic_nand");

	/* atomic_test_bit() */
	for (i = 0; i < 32; i++) {
		target = 0x0F0F0F0F;
		assert_true(!!(atomic_test_bit(&target, i) == !!(target & (1 << i))),
			    "atomic_test_bit");
	}

	/* atomic_test_and_clear_bit() */
	for (i = 0; i < 32; i++) {
		orig = 0x0F0F0F0F;
		target = orig;
		assert_true(!!(atomic_test_and_clear_bit(&target, i)) == !!(orig & (1 << i)),
			    "atomic_test_and_clear_bit");
		assert_true(target == (orig & ~(1 << i)), "atomic_test_and_clear_bit");
	}

	/* atomic_test_and_set_bit() */
	for (i = 0; i < 32; i++) {
		orig = 0x0F0F0F0F;
		target = orig;
		assert_true(!!(atomic_test_and_set_bit(&target, i)) == !!(orig & (1 << i)),
			    "atomic_test_and_set_bit");
		assert_true(target == (orig | (1 << i)), "atomic_test_and_set_bit");
	}

	/* atomic_clear_bit() */
	for (i = 0; i < 32; i++) {
		orig = 0x0F0F0F0F;
		target = orig;
		atomic_clear_bit(&target, i);
		assert_true(target == (orig & ~(1 << i)), "atomic_clear_bit");
	}

	/* atomic_set_bit() */
	for (i = 0; i < 32; i++) {
		orig = 0x0F0F0F0F;
		target = orig;
		atomic_set_bit(&target, i);
		assert_true(target == (orig | (1 << i)), "atomic_set_bit");
	}

}
