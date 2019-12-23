/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>

#define TIMEOUT 100
#define STACK_LEN 2

static ZTEST_BMEM stack_data_t data[STACK_LEN];
extern struct k_stack stack;

static void stack_pop_fail(struct k_stack *stack)
{
	stack_data_t rx_data;

	/**TESTPOINT: stack pop returns -EBUSY*/
	zassert_equal(k_stack_pop(stack, &rx_data, K_NO_WAIT), -EBUSY, NULL);
	/**TESTPOINT: stack pop returns -EAGAIN*/
	zassert_equal(k_stack_pop(stack, &rx_data, TIMEOUT), -EAGAIN, NULL);
}

/**
 * @addtogroup kernel_stack_tests
 * @{
 */

/**
 * @brief Verifies stack pop functionality
 * @see k_stack_init(), k_stack_pop()
 */
void test_stack_pop_fail(void)
{
	k_stack_init(&stack, data, STACK_LEN);

	stack_pop_fail(&stack);
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Verifies stack pop from a user thread
 * @see k_stack_init(), k_stack_pop()
 */
void test_stack_user_pop_fail(void)
{
	struct k_stack *alloc_stack = k_object_alloc(K_OBJ_STACK);

	zassert_not_null(alloc_stack, "couldn't allocate stack object");
	zassert_false(k_stack_alloc_init(alloc_stack, STACK_LEN),
		      "stack init failed");

	stack_pop_fail(alloc_stack);
}
#endif
/**
 * @}
 */
