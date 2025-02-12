/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/atomic.h>

/* convenience macro - return either 64-bit or 32-bit value */
#define ATOMIC_WORD(val_if_64, val_if_32)                                                          \
	((atomic_t)((sizeof(void *) == sizeof(uint64_t)) ? (val_if_64) : (val_if_32)))

/* an example of the number of atomic bit in an array */
#define NUM_FLAG_BITS 100

/* set test_cycle 1000us * 20 = 20ms */
#define TEST_CYCLE 20

#define THREADS_NUM 2

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_THREAD_STACK_ARRAY_DEFINE(stack, THREADS_NUM, STACK_SIZE);

static struct k_thread thread[THREADS_NUM];

atomic_t total_atomic;

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
ZTEST_USER(atomic, test_atomic)
{
	int i;

	atomic_t target, orig;
	atomic_ptr_t ptr_target;
	atomic_val_t value;
	atomic_val_t oldvalue;
	void *ptr_value, *old_ptr_value;

	ATOMIC_DEFINE(flag_bits, NUM_FLAG_BITS) = {0};

	zassert_equal(sizeof(atomic_t), ATOMIC_WORD(sizeof(uint64_t), sizeof(uint32_t)),
		      "sizeof(atomic_t)");

	target = 4;
	value = 5;
	oldvalue = 6;

	/* atomic_cas() */
	zassert_false(atomic_cas(&target, oldvalue, value), "atomic_cas");
	target = 6;
	zassert_true(atomic_cas(&target, oldvalue, value), "atomic_cas");
	zassert_true((target == value), "atomic_cas");

	/* atomic_ptr_cas() */
	ptr_target = ATOMIC_PTR_INIT((void *)4);
	ptr_value = (atomic_ptr_val_t)5;
	old_ptr_value = (atomic_ptr_val_t)6;
	zassert_false(atomic_ptr_cas(&ptr_target, old_ptr_value, ptr_value),
		      "atomic_ptr_cas");
	ptr_target = (atomic_ptr_val_t)6;
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
	ptr_target = ATOMIC_PTR_INIT((void *)50);
	zassert_true((atomic_ptr_get(&ptr_target) == (atomic_ptr_val_t)50),
		     "atomic_ptr_get");

	/* atomic_set() */
	target = 42;
	value = 77;
	zassert_true((atomic_set(&target, value) == 42), "atomic_set");
	zassert_true((target == value), "atomic_set");

	/* atomic_ptr_set() */
	ptr_target = ATOMIC_PTR_INIT((void *)42);
	ptr_value = (atomic_ptr_val_t)77;
	zassert_true((atomic_ptr_set(&ptr_target, ptr_value) == (atomic_ptr_val_t)42),
		     "atomic_ptr_set");
	zassert_true((ptr_target == ptr_value), "atomic_ptr_set");

	/* atomic_clear() */
	target = 100;
	zassert_true((atomic_clear(&target) == 100), "atomic_clear");
	zassert_true((target == 0), "atomic_clear");

	/* atomic_ptr_clear() */
	ptr_target = ATOMIC_PTR_INIT((void *)100);
	zassert_true((atomic_ptr_clear(&ptr_target) == (atomic_ptr_val_t)100),
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
	zassert_true((target == ATOMIC_WORD(0xFFFFFFFFFFFFF0FF, 0xFFFFF0FF)), "atomic_nand");

	/* atomic_test_bit() */
	for (i = 0; i < ATOMIC_BITS; i++) {
		target = ATOMIC_WORD(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F);
		zassert_true(!!(atomic_test_bit(&target, i) == !!(target & BIT(i))),
			     "atomic_test_bit");
	}

	/* atomic_test_and_clear_bit() */
	for (i = 0; i < ATOMIC_BITS; i++) {
		orig = ATOMIC_WORD(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F);
		target = orig;
		zassert_true(!!(atomic_test_and_clear_bit(&target, i)) == !!(orig & BIT(i)),
			     "atomic_test_and_clear_bit");
		zassert_true(target == (orig & ~BIT(i)), "atomic_test_and_clear_bit");
	}

	/* atomic_test_and_set_bit() */
	for (i = 0; i < ATOMIC_BITS; i++) {
		orig = ATOMIC_WORD(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F);
		target = orig;
		zassert_true(!!(atomic_test_and_set_bit(&target, i)) == !!(orig & BIT(i)),
			     "atomic_test_and_set_bit");
		zassert_true(target == (orig | BIT(i)), "atomic_test_and_set_bit");
	}

	/* atomic_clear_bit() */
	for (i = 0; i < ATOMIC_BITS; i++) {
		orig = ATOMIC_WORD(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F);
		target = orig;
		atomic_clear_bit(&target, i);
		zassert_true(target == (orig & ~BIT(i)), "atomic_clear_bit");
	}

	/* atomic_set_bit() */
	for (i = 0; i < ATOMIC_BITS; i++) {
		orig = ATOMIC_WORD(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F);
		target = orig;
		atomic_set_bit(&target, i);
		zassert_true(target == (orig | BIT(i)), "atomic_set_bit");
	}

	/* atomic_set_bit_to(&target, i, false) */
	for (i = 0; i < ATOMIC_BITS; i++) {
		orig = ATOMIC_WORD(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F);
		target = orig;
		atomic_set_bit_to(&target, i, false);
		zassert_true(target == (orig & ~BIT(i)), "atomic_set_bit_to");
	}

	/* atomic_set_bit_to(&target, i, true) */
	for (i = 0; i < ATOMIC_BITS; i++) {
		orig = ATOMIC_WORD(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F);
		target = orig;
		atomic_set_bit_to(&target, i, true);
		zassert_true(target == (orig | BIT(i)), "atomic_set_bit_to");
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

/* This helper function will run more the one slice */
void atomic_handler(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (int i = 0; i < TEST_CYCLE; i++) {
		atomic_inc(&total_atomic);
		/* Do 1000us busywait to longer the handler execute time */
		k_busy_wait(1000);
	}
}

/**
 * @brief Verify atomic operation with threads
 *
 * @details Creat two preempt threads with equal priority to
 * atomically access the same atomic value. Because these preempt
 * threads are of equal priority, so enable time slice to make
 * them scheduled. The thread will execute for some time.
 * In this time, the two sub threads will be scheduled separately
 * according to the time slice.
 *
 * @ingroup kernel_common_tests
 */
ZTEST(atomic, test_threads_access_atomic)
{
	k_tid_t tid[THREADS_NUM];

	/* enable time slice 1ms at priority 10 */
	k_sched_time_slice_set(1, K_PRIO_PREEMPT(10));

	for (int i = 0; i < THREADS_NUM; i++) {
		tid[i] = k_thread_create(&thread[i], stack[i], STACK_SIZE,
				atomic_handler, NULL, NULL, NULL,
				K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
	}

	for (int i = 0; i < THREADS_NUM; i++) {
		k_thread_join(tid[i], K_FOREVER);
	}

	/* disable time slice */
	k_sched_time_slice_set(0, K_PRIO_PREEMPT(10));

	zassert_true(total_atomic == (TEST_CYCLE * THREADS_NUM),
		"atomic counting failure");
}

/**
 * @brief Checks that the value of atomic_t will be the same in case of overflow
 *		if incremented in atomic and non-atomic manner
 *
 * @details According to C standard the value of a signed variable
 *	is undefined in case of overflow. This test checks that the value
 *	of atomic_t will be the same in case of overflow if incremented in atomic
 *	and non-atomic manner. This allows us to increment an atomic variable
 *	in a non-atomic manner (as long as it is logically safe)
 *	and expect its value to match the result of the similar atomic increment.
 *
 * @ingroup kernel_common_tests
 */
ZTEST(atomic, test_atomic_overflow)
{
	/* Check overflow over max signed value */
	uint64_t overflowed_value = (uint64_t)1 << (ATOMIC_BITS - 1);
	atomic_val_t atomic_value = overflowed_value - 1;
	atomic_t atomic_var = ATOMIC_INIT(atomic_value);

	atomic_value++;
	atomic_inc(&atomic_var);

	zassert_true(atomic_value == atomic_get(&atomic_var),
		"max signed overflow mismatch: %lx/%lx",
		atomic_value, atomic_get(&atomic_var));
	zassert_true(atomic_value == (atomic_val_t)overflowed_value,
		"unexpected value after overflow: %lx, expected: %lx",
		atomic_value, (atomic_val_t)overflowed_value);

	/* Check overflow over max unsigned value */
	atomic_value = -1;
	atomic_var = ATOMIC_INIT(atomic_value);

	atomic_value++;
	atomic_inc(&atomic_var);

	zassert_true(atomic_value == atomic_get(&atomic_var),
		"max unsigned overflow mismatch: %lx/%lx",
		atomic_value, atomic_get(&atomic_var));
	zassert_true(atomic_value == 0,
		"unexpected value after overflow: %lx, expected: 0",
		atomic_value);
}

extern void *common_setup(void);
ZTEST_SUITE(atomic, NULL, common_setup, NULL, NULL, NULL);
/**
 * @}
 */
