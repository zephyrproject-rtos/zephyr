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

#include <zephyr.h>
#include <atomic.h>

#include <tc_util.h>

#define CHECK_OUTPUT(expr, val) do { \
	atomic_val_t x = (expr); \
	if (x != (val)) { \
		failed++; \
		TC_ERROR(#expr" != " #val " got %d\n", x); \
	} \
} while (0)

#define CHECK_TRUTH(expr, val) CHECK_OUTPUT(!!(expr), !!(val))

void main(void)
{
	int failed, rv, i;

	atomic_t target, orig;
	atomic_val_t value;
	atomic_val_t oldvalue;

	failed = 0;
	TC_START("Test atomic operation primitives");

	TC_PRINT("Test atomic_cas()\n");
	target = 4;
	value = 5;
	oldvalue = 6;

	CHECK_OUTPUT(atomic_cas(&target, oldvalue, value), 0);
	target = 6;
	CHECK_OUTPUT(atomic_cas(&target, oldvalue, value), 1);
	CHECK_OUTPUT(target, value);

	TC_PRINT("Test atomic_add()\n");
	target = 1;
	value = 2;
	CHECK_OUTPUT(atomic_add(&target, value), 1);
	CHECK_OUTPUT(target, 3);

	TC_PRINT("Test atomic_sub()\n");
	target = 10;
	value = 2;
	CHECK_OUTPUT(atomic_sub(&target, value), 10);
	CHECK_OUTPUT(target, 8);

	TC_PRINT("Test atomic_inc()\n");
	target = 5;
	CHECK_OUTPUT(atomic_inc(&target), 5);
	CHECK_OUTPUT(target, 6);

	TC_PRINT("Test atomic_dec()\n");
	target = 2;
	CHECK_OUTPUT(atomic_dec(&target), 2);
	CHECK_OUTPUT(target, 1);

	TC_PRINT("Test atomic_get()\n");
	target = 50;
	CHECK_OUTPUT(atomic_get(&target), 50);

	TC_PRINT("Test atomic_set()\n");
	target = 42;
	value = 77;
	CHECK_OUTPUT(atomic_set(&target, value), 42);
	CHECK_OUTPUT(target, value);

	TC_PRINT("Test atomic_clear()\n");
	target = 100;
	CHECK_OUTPUT(atomic_clear(&target), 100);
	CHECK_OUTPUT(target, 0);

	TC_PRINT("Test atomic_or()\n");
	target = 0xFF00;
	value  = 0x0F0F;
	CHECK_OUTPUT(atomic_or(&target, value), 0xFF00);
	CHECK_OUTPUT(target, 0xFF0F);

	TC_PRINT("Test atomic_xor()\n");
	target = 0xFF00;
	value  = 0x0F0F;
	CHECK_OUTPUT(atomic_xor(&target, value), 0xFF00);
	CHECK_OUTPUT(target, 0xF00F);

	TC_PRINT("Test atomic_and()\n");
	target = 0xFF00;
	value  = 0x0F0F;
	CHECK_OUTPUT(atomic_and(&target, value), 0xFF00);
	CHECK_OUTPUT(target, 0x0F00);

	TC_PRINT("Test atomic_nand()\n");
	target = 0xFF00;
	value  = 0x0F0F;
	CHECK_OUTPUT(atomic_nand(&target, value), 0xFF00);
	CHECK_OUTPUT(target, 0xFFFFF0FF);

	TC_PRINT("Test atomic_test_bit()\n");
	for (i = 0; i < 32; i++) {
		target = 0x0F0F0F0F;
		CHECK_TRUTH(atomic_test_bit(&target, i),
			    (target & (1 << i)));
	}

	TC_PRINT("Test atomic_test_and_clear_bit()\n");
	for (i = 0; i < 32; i++) {
		orig = 0x0F0F0F0F;
		target = orig;
		CHECK_TRUTH(atomic_test_and_clear_bit(&target, i),
			    (orig & (1 << i)));
		CHECK_OUTPUT(target, orig & ~(1 << i));
	}

	TC_PRINT("Test atomic_test_and_set_bit()\n");
	for (i = 0; i < 32; i++) {
		orig = 0x0F0F0F0F;
		target = orig;
		CHECK_TRUTH(atomic_test_and_set_bit(&target, i),
			    (orig & (1 << i)));
		CHECK_OUTPUT(target, orig | (1 << i));
	}

	TC_PRINT("Test atomic_clear_bit()\n");
	for (i = 0; i < 32; i++) {
		orig = 0x0F0F0F0F;
		target = orig;
		atomic_clear_bit(&target, i);
		CHECK_OUTPUT(target, orig & ~(1 << i));
	}

	TC_PRINT("Test atomic_set_bit()\n");
	for (i = 0; i < 32; i++) {
		orig = 0x0F0F0F0F;
		target = orig;
		atomic_set_bit(&target, i);
		CHECK_OUTPUT(target, orig | (1 << i));
	}

	if (failed) {
		TC_PRINT("%d tests failed\n", failed);
		rv = TC_FAIL;
	} else {
		rv = TC_PASS;
	}

	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
