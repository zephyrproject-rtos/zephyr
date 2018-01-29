/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file test memory pool and heap APIs
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
 *
 * @brief Compare the two blocks
 *
 * @return 0 if the same, non-zero if not the same
 */

int block_compare(struct k_mem_block *b1, struct k_mem_block *b2)
{
	char *p1 = (char *) b1;
	char *p2 = (char *) b2;
	int i;
	int diff = 0;

	for (i = 0; i < sizeof(struct k_mem_block); i++) {
		diff = p2[i] - p1[i];
		if (diff != 0) {
			break;
		}
	}

	return diff;
}

/**
 *
 * @brief Wrapper for k_mem_pool_alloc()
 *
 * @return k_mem_pool_alloc() return value
 */

int pool_block_get_func(struct k_mem_block *block, struct k_mem_pool *pool,
			int size, s32_t unused)
{
	ARG_UNUSED(unused);

	return k_mem_pool_alloc(pool, block, size, K_NO_WAIT);
}

/**
 *
 * @brief Wrapper for k_mem_pool_alloc(K_FOREVER)
 *
 * @return k_mem_pool_alloc() return value
 */

int pool_block_get_w_func(struct k_mem_block *block, struct k_mem_pool *pool,
			  int size, s32_t unused)
{
	ARG_UNUSED(unused);

	return k_mem_pool_alloc(pool, block, size, K_FOREVER);
}

/**
 *
 * @brief Wrapper for k_mem_pool_alloc(timeout)
 *
 * @return k_mem_pool_alloc(timeout) return value
 */

int pool_block_get_wt_func(struct k_mem_block *block, struct k_mem_pool *pool,
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

void free_blocks(struct TEST_CASE *tests, int n_tests)
{
	int i;

	for (i = 0; i < n_tests; i++) {
		if (tests[i].rcode == 0) {
			k_mem_pool_free(tests[i].block);
		}
	}
}

/**
 *
 * @brief Perform the work of getting blocks
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pool_block_get_work(char *string, pool_block_get_func_t func,
			struct TEST_CASE *tests, int n_tests)
{
	int i;
	int rv;

	for (i = 0; i < n_tests; i++) {
		rv = func(tests[i].block, tests[i].pool_id, tests[i].size,
			  tests[i].timeout);
		if (rv != tests[i].rcode) {
			TC_ERROR("%s() expected %d, got %d\n"
				 "size: %d, timeout: %d\n", string, tests[i].rcode, rv,
				 tests[i].size, tests[i].timeout);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 *
 * @brief Test the k_mem_pool_alloc(K_NO_WAIT) API
 *
 * The pool is 4 k_b in size.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pool_block_get_test(void)
{
	int rv;         /* return value from k_mem_pool_alloc() */
	int j;          /* loop counter */

	for (j = 0; j < 8; j++) {
		rv = pool_block_get_work("k_mem_pool_alloc", pool_block_get_func,
					 get_set, ARRAY_SIZE(get_set));
		if (rv != TC_PASS) {
			return TC_FAIL;
		}

		free_blocks(get_set, ARRAY_SIZE(get_set));

		rv = pool_block_get_work("k_mem_pool_alloc", pool_block_get_func,
					 get_set2, ARRAY_SIZE(get_set2));
		if (rv != TC_PASS) {
			return TC_FAIL;
		}

		free_blocks(get_set2, ARRAY_SIZE(get_set2));
	}

	return TC_PASS;
}

/**
 *
 * @brief Helper task to pool_block_get_timeout_test()
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
 *
 * @brief Test k_mem_pool_alloc(timeout)
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pool_block_get_timeout_test(void)
{
	struct k_mem_block block;
	int rv;         /* return value from k_mem_pool_alloc() */
	int j;          /* loop counter */

	for (j = 0; j < 8; j++) {
		rv = pool_block_get_work("k_mem_pool_alloc", pool_block_get_wt_func,
					 getwt_set, ARRAY_SIZE(getwt_set));
		if (rv != TC_PASS) {
			return TC_FAIL;
		}

		free_blocks(getwt_set, ARRAY_SIZE(getwt_set));
	}

	rv = k_mem_pool_alloc(&POOL_ID, &helper_block, 3148, 5);
	if (rv != 0) {
		TC_ERROR("Failed to get size 3148 byte block from POOL_ID\n");
		return TC_FAIL;
	}

	rv = k_mem_pool_alloc(&POOL_ID, &block, 3148, K_NO_WAIT);
	if (rv != -ENOMEM) {
		TC_ERROR("Unexpectedly got size 3148 "
			 "byte block from POOL_ID\n");
		return TC_FAIL;
	}

	k_sem_give(&HELPER_SEM);    /* Activate helper_task */
	rv = k_mem_pool_alloc(&POOL_ID, &block, 3148, 20);
	if (rv != 0) {
		TC_ERROR("Failed to get size 3148 byte block from POOL_ID\n");
		return TC_FAIL;
	}

	rv = k_sem_take(&REGRESS_SEM, K_NO_WAIT);
	if (rv != 0) {
		TC_ERROR("Failed to get size 3148 "
			 "byte block within 20 ticks\n");
		return TC_FAIL;
	}

	k_mem_pool_free(&block);

	return TC_PASS;
}

/**
 *
 * pool_block_get_wait_test -
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pool_block_get_wait_test(void)
{
	int rv;

	rv = k_mem_pool_alloc(&POOL_ID, &block_list[0], 3000, K_FOREVER);
	if (rv != 0) {
		TC_ERROR("k_mem_pool_alloc(3000) expected %d, got %d\n", 0, rv);
		return TC_FAIL;
	}

	k_sem_give(&ALTERNATE_SEM);    /* Wake alternate_task */
	evidence = 0;
	rv = k_mem_pool_alloc(&POOL_ID, &block_list[1], 128, K_FOREVER);
	if (rv != 0) {
		TC_ERROR("k_mem_pool_alloc(128) expected %d, got %d\n", 0, rv);
		return TC_FAIL;
	}

	switch (evidence) {
	case 0:
		TC_ERROR("k_mem_pool_alloc(128) did not block!\n");
		return TC_FAIL;
	case 1:
		break;
	case 2:
	default:
		TC_ERROR("Rescheduling did not occur "
			 "after k_mem_pool_free()\n");
		return TC_FAIL;
	}

	k_mem_pool_free(&block_list[1]);

	return TC_PASS;
}

/**
 *
 * @brief Alternate task in the test suite
 *
 * This routine runs at a lower priority than Regression_task().
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
 *
 * @brief Test the k_malloc() and k_free() APIs
 *
 * The heap memory pool is 256 bytes in size, and thus has only 4 blocks
 * of 64 bytes or a single block of 256 bytes. (Each block has a lesser
 * amount of usable space, due to the hidden block descriptor info the
 * kernel adds at the start of any block allocated from this memory pool.)
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pool_malloc_test(void)
{
	char *block[4];
	int j;     /* loop counter */

	TC_PRINT("Testing k_malloc() and k_free() ...\n");

	/* allocate a large block (which consumes the entire pool buffer) */
	block[0] = k_malloc(150);
	if (block[0] == NULL) {
		TC_ERROR("150 byte allocation failed\n");
		return TC_FAIL;
	}

	/* ensure a small block can no longer be allocated */
	block[1] = k_malloc(16);
	if (block[1] != NULL) {
		TC_ERROR("16 byte allocation did not fail\n");
		return TC_FAIL;
	}

	/* return the large block */
	k_free(block[0]);

	/* allocate a small block (triggers block splitting)*/
	block[0] = k_malloc(16);
	if (block[0] == NULL) {
		TC_ERROR("16 byte allocation 0 failed\n");
		return TC_FAIL;
	}

	/* ensure a large block can no longer be allocated */
	block[1] = k_malloc(80);
	if (block[1] != NULL) {
		TC_ERROR("80 byte allocation did not fail\n");
		return TC_FAIL;
	}

	/* ensure all remaining small blocks can be allocated */
	for (j = 1; j < 4; j++) {
		block[j] = k_malloc(16);
		if (block[j] == NULL) {
			TC_ERROR("16 byte allocation %d failed\n", j);
			return TC_FAIL;
		}
	}

	/* ensure a small block can no longer be allocated */
	if (k_malloc(8) != NULL) {
		TC_ERROR("8 byte allocation did not fail\n");
		return TC_FAIL;
	}

	/* return the small blocks to pool in a "random" order */
	k_free(block[2]);
	k_free(block[0]);
	k_free(block[3]);
	k_free(block[1]);

	/* allocate large block (triggers autodefragmentation) */
	block[0] = k_malloc(100);
	if (block[0] == NULL) {
		TC_ERROR("100 byte allocation failed\n");
		return TC_FAIL;
	}

	/* ensure a small block can no longer be allocated */
	if (k_malloc(32) != NULL) {
		TC_ERROR("32 byte allocation did not fail\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Main task in the test suite
 *
 * This is the entry point to the memory pool test suite.
 *
 * @return N/A
 */

void test_mem_pool(void)
{
	int tc_rc;     /* test case return code */

	TC_START("Test Memory Pool and Heap APIs");

	TC_PRINT("Testing k_mem_pool_alloc(K_NO_WAIT) ...\n");
	tc_rc = pool_block_get_test();
	zassert_equal(tc_rc, TC_PASS, "pool block failure");

	TC_PRINT("Testing k_mem_pool_alloc(timeout) ...\n");
	tc_rc = pool_block_get_timeout_test();
	zassert_equal(tc_rc, TC_PASS, "pool block timeout failure");

	TC_PRINT("Testing k_mem_pool_alloc(K_FOREVER) ...\n");
	tc_rc = pool_block_get_wait_test();
	zassert_equal(tc_rc, TC_PASS, "pool block wait failure");

	tc_rc = pool_malloc_test();
	zassert_equal(tc_rc, TC_PASS, "pool malloc failure");
}


K_THREAD_DEFINE(t_alternate, STACKSIZE, alternate_task, NULL, NULL, NULL,
		6, 0, K_NO_WAIT);

K_THREAD_DEFINE(t_helper, STACKSIZE, helper_task, NULL, NULL, NULL,
		7, 0, K_NO_WAIT);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_mempool, ztest_unit_test(test_mem_pool));
	ztest_run_test_suite(test_mempool);
}
