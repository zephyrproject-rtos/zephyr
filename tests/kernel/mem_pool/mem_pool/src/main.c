/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Test memory pool and heap APIs
 *
 * This modules tests the following memory pool routines:
 *
 * - k_mem_pool_alloc(),
 * - k_mem_pool_free(),
 * - k_malloc(),
 * - k_free()
 */

#include <zephyr.h>
#include <ztest.h>
#include <tc_util.h>
#include <misc/util.h>

#define  ONE_SECOND     (sys_clock_ticks_per_sec)
#define  TENTH_SECOND   (sys_clock_ticks_per_sec / 10)

#define  NUM_BLOCKS     64

/* size of stack area used by each thread */
#define STACKSIZE 512

K_SEM_DEFINE(ALTERNATE_SEM, 0, 1);
K_SEM_DEFINE(REGRESS_SEM, 0, 1);
K_SEM_DEFINE(HELPER_SEM, 0, 1);

K_MEM_POOL_DEFINE(POOL_ID, 64, 4096, 1, 4);
K_MEM_POOL_DEFINE(SECOND_POOL_ID, 16, 1024, 5, 4);

struct TEST_CASE {
	struct k_mem_block *block;      /* pointer to block data */
	struct k_mem_pool *pool_id;     /* pool ID */
	int size;                       /* request size in bytes */
	s32_t timeout;                  /* # of ticks to wait */
	int rcode;                      /* expected return code */
};

typedef int (*pool_block_get_func_t)(struct k_mem_block *, struct k_mem_pool *,
				     int, s32_t);
typedef int (*pool_move_block_func_t)(struct k_mem_block *, struct k_mem_pool *);

static volatile int evidence;

static struct k_mem_block block_list[NUM_BLOCKS];
static struct k_mem_block helper_block;

static struct TEST_CASE get_set[] = {
	{ &block_list[0], &POOL_ID, 0, 0, 0 },
	{ &block_list[1], &POOL_ID, 1, 0, 0 },
	{ &block_list[2], &POOL_ID, 32, 0, 0 },
	{ &block_list[3], &POOL_ID, 64, 0, 0 },
	{ &block_list[4], &POOL_ID, 128, 0, 0 },
	{ &block_list[5], &POOL_ID, 256, 0, 0 },
	{ &block_list[6], &POOL_ID, 512, 0, 0 },
	{ &block_list[7], &POOL_ID, 1024, 0, 0 },
	{ &block_list[8], &POOL_ID, 2048, 0, -ENOMEM },
	{ &block_list[9], &POOL_ID, 4096, 0, -ENOMEM }
};

static struct TEST_CASE get_set2[] = {
	{ &block_list[0], &POOL_ID, 4096, 0, 0 },
	{ &block_list[1], &POOL_ID, 2048, 0, -ENOMEM },
	{ &block_list[2], &POOL_ID, 1024, 0, -ENOMEM },
	{ &block_list[3], &POOL_ID, 512, 0, -ENOMEM },
	{ &block_list[4], &POOL_ID, 256, 0, -ENOMEM }
};

static struct TEST_CASE getwt_set[] = {
	{ &block_list[0], &POOL_ID, 4096, TENTH_SECOND, 0 },
	{ &block_list[1], &POOL_ID, 2048, TENTH_SECOND, -EAGAIN },
	{ &block_list[2], &POOL_ID, 1024, TENTH_SECOND, -EAGAIN },
	{ &block_list[3], &POOL_ID, 512, TENTH_SECOND, -EAGAIN },
	{ &block_list[4], &POOL_ID, 256, TENTH_SECOND, -EAGAIN }
};


/**
 * @brief Wrapper for k_mem_pool_alloc()
 *
 * @return k_mem_pool_alloc() return value
 */
static int pool_block_get_func(struct k_mem_block *block, struct k_mem_pool *pool,
			int size, s32_t unused)
{
	ARG_UNUSED(unused);

	return k_mem_pool_alloc(pool, block, size, K_NO_WAIT);
}


/**
 *
 * @brief Wrapper for k_mem_pool_alloc(timeout)
 *
 * @return k_mem_pool_alloc(timeout) return value
 */
static int pool_block_get_wt_func(struct k_mem_block *block, struct k_mem_pool *pool,
			   int size, s32_t timeout)
{
	return k_mem_pool_alloc(pool, block, size, timeout);
}

/**
 *
 * @brief Free any blocks allocated in the test set
 *
 * @return N/A
 */
static void free_blocks(struct TEST_CASE *tests, int n_tests)
{
	int i;

	for (i = 0; i < n_tests; i++) {
		if (tests[i].rcode == 0) {
			k_mem_pool_free(tests[i].block);
		}
	}
}

/**
 * @brief Perform the work of getting blocks
 *
 */
static void pool_block_get_work(char *string, pool_block_get_func_t func,
			 struct TEST_CASE *tests, int n_tests)
{
	int i;
	int rv;

	for (i = 0; i < n_tests; i++) {
		rv = func(tests[i].block, tests[i].pool_id, tests[i].size,
			  tests[i].timeout);
		zassert_equal(rv, tests[i].rcode, "%s() expected %d, got %d\n"
			      "size: %d, timeout: %d\n", string, tests[i].rcode, rv,
			      tests[i].size, tests[i].timeout);
	}

}

/**
 * @ingroup kernel_memory_pool_tests
 * @brief Test the k_mem_pool_alloc(K_NO_WAIT) API
 *
 * The pool is 4 k_b in size.
 *
 * @see k_mem_pool_alloc()
 */
static void test_pool_block_get(void)
{
	int j;          /* loop counter */

	for (j = 0; j < 8; j++) {
		pool_block_get_work("k_mem_pool_alloc", pool_block_get_func,
				    get_set, ARRAY_SIZE(get_set));

		free_blocks(get_set, ARRAY_SIZE(get_set));

		pool_block_get_work("k_mem_pool_alloc", pool_block_get_func,
				    get_set2, ARRAY_SIZE(get_set2));

		free_blocks(get_set2, ARRAY_SIZE(get_set2));
	}
}

/**
 * @brief Helper task to test_pool_block_get_timeout()
 *
 * @return N/A
 */
void helper_task(void)
{
	k_sem_take(&HELPER_SEM, K_FOREVER);

	k_sem_give(&REGRESS_SEM);
	k_mem_pool_free(&helper_block);
}

/**
 * @ingroup kernel_memory_pool_tests
 * @brief Test k_mem_pool_alloc(timeout)
 *
 * @see k_mem_pool_alloc()
 */
static void test_pool_block_get_timeout(void)
{
	struct k_mem_block block;
	int rv;         /* return value from k_mem_pool_alloc() */
	int j;          /* loop counter */

	for (j = 0; j < 8; j++) {
		pool_block_get_work("k_mem_pool_alloc", pool_block_get_wt_func,
				    getwt_set, ARRAY_SIZE(getwt_set));

		free_blocks(getwt_set, ARRAY_SIZE(getwt_set));
	}

	rv = k_mem_pool_alloc(&POOL_ID, &helper_block, 3148, 5);
	zassert_true(rv == 0,
		     "Failed to get size 3148 byte block from POOL_ID");

	rv = k_mem_pool_alloc(&POOL_ID, &block, 3148, K_NO_WAIT);
	zassert_true(rv == -ENOMEM, "Unexpectedly got size 3148 "
		     "byte block from POOL_ID");

	k_sem_give(&HELPER_SEM);    /* Activate helper_task */
	rv = k_mem_pool_alloc(&POOL_ID, &block, 3148, 20);
	zassert_true(rv == 0, "Failed to get size 3148 byte block from POOL_ID");

	rv = k_sem_take(&REGRESS_SEM, K_NO_WAIT);
	zassert_true(rv == 0, "Failed to get size 3148 "
		     "byte block within 20 ticks");

	k_mem_pool_free(&block);

}

/**
 * @ingroup kernel_memory_pool_tests
 * @see k_mem_pool_alloc(), k_mem_pool_free()
 */
static void test_pool_block_get_wait(void)
{
	int rv;

	rv = k_mem_pool_alloc(&POOL_ID, &block_list[0], 3000, K_FOREVER);
	zassert_equal(rv, 0, "k_mem_pool_alloc(3000) expected %d, got %d\n", 0, rv);

	k_sem_give(&ALTERNATE_SEM);    /* Wake alternate_task */
	evidence = 0;
	rv = k_mem_pool_alloc(&POOL_ID, &block_list[1], 128, K_FOREVER);
	zassert_true(rv == 0, "k_mem_pool_alloc(128) expected %d, got %d\n", 0, rv);

	switch (evidence) {
	case 0:
		zassert_true(evidence == 0, "k_mem_pool_alloc(128) did not block!");
	case 1:
		break;
	case 2:
	default:
		zassert_true(1, "Rescheduling did not occur "
			     "after k_mem_pool_free()");
	}

	k_mem_pool_free(&block_list[1]);

}

/**
 * @brief Alternate task in the test suite
 *
 * This routine runs at a lower priority than main thread.
 *
 * @return N/A
 */
void alternate_task(void)
{
	k_sem_take(&ALTERNATE_SEM, K_FOREVER);

	evidence = 1;

	k_mem_pool_free(&block_list[0]);

	evidence = 2;
}

/**
 * @ingroup kernel_memory_pool_tests
 * @brief Test the k_malloc() and k_free() APIs
 *
 * The heap memory pool is 256 bytes in size, and thus has only 4 blocks
 * of 64 bytes or a single block of 256 bytes. (Each block has a lesser
 * amount of usable space, due to the hidden block descriptor info the
 * kernel adds at the start of any block allocated from this memory pool.)
 *
 *
 * @see k_malloc(), k_free()
 */
static void test_pool_malloc(void)
{
	char *block[4];
	int j;     /* loop counter */

	/* allocate a large block (which consumes the entire pool buffer) */
	block[0] = k_malloc(150);
	zassert_not_null(block[0], "150 byte allocation failed");

	/* ensure a small block can no longer be allocated */
	block[1] = k_malloc(16);
	zassert_is_null(block[1], "16 byte allocation did not fail");

	/* return the large block */
	k_free(block[0]);

	/* allocate a small block (triggers block splitting)*/
	block[0] = k_malloc(16);
	zassert_not_null(block[0], "16 byte allocation 0 failed");

	/* ensure a large block can no longer be allocated */
	block[1] = k_malloc(80);
	zassert_is_null(block[1], "80 byte allocation did not fail");

	/* ensure all remaining small blocks can be allocated */
	for (j = 1; j < 4; j++) {
		block[j] = k_malloc(16);
		zassert_not_null(block[j], "16 byte allocation %d failed\n", j);
	}

	/* ensure a small block can no longer be allocated */
	zassert_is_null(k_malloc(8), "8 byte allocation did not fail");

	/* return the small blocks to pool in a "random" order */
	k_free(block[2]);
	k_free(block[0]);
	k_free(block[3]);
	k_free(block[1]);

	/* allocate large block (triggers autodefragmentation) */
	block[0] = k_malloc(100);
	zassert_not_null(block[0], "100 byte allocation failed");

	/* ensure a small block can no longer be allocated */
	zassert_is_null(k_malloc(32), "32 byte allocation did not fail");

	/* ensure overflow detection is working */
	zassert_is_null(k_malloc(0xffffffff), "overflow check failed");
	zassert_is_null(k_calloc(0xffffffff, 2), "overflow check failed");
}

K_THREAD_DEFINE(t_alternate, STACKSIZE, alternate_task, NULL, NULL, NULL,
		6, 0, K_NO_WAIT);

K_THREAD_DEFINE(t_helper, STACKSIZE, helper_task, NULL, NULL, NULL,
		7, 0, K_NO_WAIT);

void test_main(void)
{
	ztest_test_suite(mempool,
			 ztest_unit_test(test_pool_block_get),
			 ztest_unit_test(test_pool_block_get_timeout),
			 ztest_unit_test(test_pool_block_get_wait),
			 ztest_unit_test(test_pool_malloc)
			 );
	ztest_run_test_suite(mempool);
}
