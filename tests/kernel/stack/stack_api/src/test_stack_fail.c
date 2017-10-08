/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_stack_api
 * @{
 * @defgroup t_stack_fail_pop test_stack_fail_pop
 * @brief TestPurpose: verify zephyr stack pop when empty
 * - API coverage
 *   -# k_stack_pop
 * @}
 */

#include <ztest.h>
#include <irq_offload.h>

#define TIMEOUT 100
#define STACK_LEN 2

static u32_t data[STACK_LEN];
extern struct k_stack stack;

/*test cases*/
void test_stack_pop_fail(void *p1, void *p2, void *p3)
{
	u32_t rx_data;

	k_stack_init(&stack, data, STACK_LEN);
	/**TESTPOINT: stack pop returns -EBUSY*/
	zassert_equal(k_stack_pop(&stack, &rx_data, K_NO_WAIT), -EBUSY, NULL);
	/**TESTPOINT: stack pop returns -EAGAIN*/
	zassert_equal(k_stack_pop(&stack, &rx_data, TIMEOUT), -EAGAIN, NULL);
}
