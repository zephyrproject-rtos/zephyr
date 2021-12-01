/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define STACK_NUMS 5

#define SEM_TIMEOUT (K_MSEC(100))
#define SEM_INIT_VAL (0U)

/* Common macro declarations for testing semaphore */
#define expect_k_sem_take(sem, timeout, exp, str) do { \
	int _act = k_sem_take((sem), (timeout)); \
	int _exp = (exp); \
	zassert_equal(_act, _exp, (str), _act, _exp); \
} while (false)

#define expect_k_sem_init(sem, init, max, exp, str) do { \
	int _act = k_sem_init((sem), (init), (max)); \
	int _exp = (exp); \
	zassert_equal(_act, _exp, (str), _act, _exp); \
} while (false)

#define expect_k_sem_count_get(sem, exp, str) do { \
	unsigned int _act = k_sem_count_get(sem); \
	unsigned int _exp = (exp); \
	zassert_equal(_act, _exp, (str), _act, _exp); \
} while (false)

#define expect_k_sem_take_nomsg(sem, timeout, exp) \
	expect_k_sem_take((sem), (timeout), (exp), "k_sem_take incorrect return value: %d != %d")
#define expect_k_sem_init_nomsg(sem, init, max, exp) \
	expect_k_sem_init((sem), (init), (max), (exp), \
					  "k_sem_init incorrect return value: %d != %d")
#define expect_k_sem_count_get_nomsg(sem, exp) \
	expect_k_sem_count_get((sem), (exp), "k_sem_count_get incorrect return value: %u != %u")
