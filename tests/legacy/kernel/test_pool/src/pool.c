/* pool.c - test microkernel memory pool APIs */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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

/*
DESCRIPTION
This modules tests the following memory pool routines:

  task_mem_pool_alloc(),
  task_mem_pool_free()
 */

#include <zephyr.h>
#include <tc_util.h>
#include <misc/util.h>

#define  ONE_SECOND     (sys_clock_ticks_per_sec)
#define  TENTH_SECOND   (sys_clock_ticks_per_sec / 10)

#define  NUM_BLOCKS     64

#define  DEFRAG_BLK_TEST 2222

typedef struct {
	struct k_block *block;    /* pointer to block data */
	kmemory_pool_t  poolId;   /* pool ID */
	int       size;     /* request size in bytes */
	int32_t   timeout;  /* # of ticks to wait */
	int       rcode;    /* expected return code */
} TEST_CASE;

typedef int (*poolBlockGetFunc_t)(struct k_block *, kmemory_pool_t, int, int32_t);
typedef int (*poolMoveBlockFunc_t)(struct k_block *, kmemory_pool_t);

static volatile int evidence = 0;

static struct k_block  blockList[NUM_BLOCKS];
static struct k_block  helperBlock;

static TEST_CASE getSet[] = {
	{&blockList[0], POOL_ID, 0, 0, RC_OK},
	{&blockList[1], POOL_ID, 1, 0, RC_OK},
	{&blockList[2], POOL_ID, 32, 0, RC_OK},
	{&blockList[3], POOL_ID, 64, 0, RC_OK},
	{&blockList[4], POOL_ID, 128, 0, RC_OK},
	{&blockList[5], POOL_ID, 256, 0, RC_OK},
	{&blockList[6], POOL_ID, 512, 0, RC_OK},
	{&blockList[7], POOL_ID, 1024, 0, RC_OK},
	{&blockList[8], POOL_ID, 2048, 0, RC_FAIL},
	{&blockList[9], POOL_ID, 4096, 0, RC_FAIL}
};

static TEST_CASE getSet2[] = {
	{&blockList[0], POOL_ID, 4096, 0, RC_OK},
	{&blockList[1], POOL_ID, 2048, 0, RC_FAIL},
	{&blockList[2], POOL_ID, 1024, 0, RC_FAIL},
	{&blockList[3], POOL_ID, 512, 0, RC_FAIL},
	{&blockList[4], POOL_ID, 256, 0, RC_FAIL}
};

static TEST_CASE getwtSet[] = {
	{&blockList[0], POOL_ID, 4096, TENTH_SECOND, RC_OK},
	{&blockList[1], POOL_ID, 2048, TENTH_SECOND, RC_TIME},
	{&blockList[2], POOL_ID, 1024, TENTH_SECOND, RC_TIME},
	{&blockList[3], POOL_ID, 512, TENTH_SECOND, RC_TIME},
	{&blockList[4], POOL_ID, 256, TENTH_SECOND, RC_TIME}
};

static TEST_CASE defrag[] = {
	{&blockList[0], POOL_ID, 64, 0, RC_OK},
	{&blockList[1], POOL_ID, 64, 0, RC_OK},
	{&blockList[2], POOL_ID, 64, 0, RC_OK},
	{&blockList[3], POOL_ID, 64, 0, RC_OK},
	{&blockList[4], POOL_ID, 256, 0, RC_OK},
	{&blockList[5], POOL_ID, 256, 0, RC_OK},
	{&blockList[6], POOL_ID, 256, 0, RC_OK},
	{&blockList[7], POOL_ID, 1024, 0, RC_OK},
	{&blockList[8], POOL_ID, 1024, 0, RC_OK},
	{&blockList[9], POOL_ID, 1024, 0, RC_OK}
};

/**
 *
 * @brief Compare the two blocks
 *
 * @return 0 if the same, non-zero if not the same
 */

int blockCompare(struct k_block *b1, struct k_block *b2)
{
	char *p1 = (char *) b1;
	char *p2 = (char *) b2;
	int i;
	int diff = 0;

	for (i = 0; i < sizeof(struct k_block); i++) {
		diff = p2[i] - p1[i];
		if (diff != 0) {
			break;
		}
	}

	return diff;
}

/**
 *
 * @brief Wrapper for task_mem_pool_alloc()
 *
 * @return task_mem_pool_alloc() return value
 */

int poolBlockGetFunc(struct k_block *block, kmemory_pool_t pool, int size,
					 int32_t unused)
{
	ARG_UNUSED(unused);

	return task_mem_pool_alloc(block, pool, size, TICKS_NONE);
}

/**
 *
 * @brief Wrapper for task_mem_pool_alloc(TICKS_UNLIMITED)
 *
 * @return task_mem_pool_alloc() return value
 */

int poolBlockGetWFunc(struct k_block *block, kmemory_pool_t pool, int size,
					  int32_t unused)
{
	ARG_UNUSED(unused);

	return task_mem_pool_alloc(block, pool, size, TICKS_UNLIMITED);
}

/**
 *
 * @brief Wrapper for task_mem_pool_alloc(timeout)
 *
 * @return task_mem_pool_alloc(timeout) return value
 */

int poolBlockGetWTFunc(struct k_block *block, kmemory_pool_t pool,
					   int size, int32_t timeout)
{
	return task_mem_pool_alloc(block, pool, size, timeout);
}

/**
 *
 * @brief Free any blocks allocated in the test set
 *
 * @return N/A
 */

void freeBlocks(TEST_CASE *tests, int nTests)
{
	int  i;

	for (i = 0; i < nTests; i++) {
		if (tests[i].rcode == RC_OK) {
			task_mem_pool_free(tests[i].block);
		}
	}
}

/**
 *
 * @brief Perform the work of getting blocks
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int poolBlockGetWork(char *string, poolBlockGetFunc_t func,
					 TEST_CASE *tests, int nTests)
{
	int  i;
	int  rv;

	for (i = 0; i < nTests; i++) {
		rv = func(tests[i].block, tests[i].poolId, tests[i].size,
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
 * @brief Test the task_mem_pool_alloc(TICKS_NONE) API
 *
 * The pool is 4 kB in size.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int poolBlockGetTest(void)
{
	int  rv;   /* return value from task_mem_pool_alloc() */
	int  j;    /* loop counter */

	for (j = 0; j < 8; j++) {
		rv = poolBlockGetWork("task_mem_pool_alloc", poolBlockGetFunc,
							  getSet, ARRAY_SIZE(getSet));
		if (rv != TC_PASS) {
			return TC_FAIL;
		}

		freeBlocks(getSet, ARRAY_SIZE(getSet));

		rv = poolBlockGetWork("task_mem_pool_alloc", poolBlockGetFunc,
							  getSet2, ARRAY_SIZE(getSet2));
		if (rv != TC_PASS) {
			return TC_FAIL;
		}

		freeBlocks(getSet2, ARRAY_SIZE(getSet2));
	}

	return TC_PASS;
}

/**
 *
 * @brief Helper task to poolBlockGetTimeoutTest()
 *
 * @return N/A
 */

void HelperTask(void)
{
	task_sem_take(HELPER_SEM, TICKS_UNLIMITED);

	task_sem_give(REGRESS_SEM);
	task_mem_pool_free(&helperBlock);
}

/**
 *
 * @brief Test task_mem_pool_alloc(timeout)
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int poolBlockGetTimeoutTest(void)
{
	struct k_block  block;
	int  rv;   /* return value from task_mem_pool_alloc() */
	int  j;    /* loop counter */

	for (j = 0; j < 8; j++) {
		rv = poolBlockGetWork("task_mem_pool_alloc", poolBlockGetWTFunc,
							  getwtSet, ARRAY_SIZE(getwtSet));
		if (rv != TC_PASS) {
			return TC_FAIL;
		}

		freeBlocks(getwtSet, ARRAY_SIZE(getwtSet));
	}

	rv = task_mem_pool_alloc(&helperBlock, POOL_ID, 3148, 5);
	if (rv != RC_OK) {
		TC_ERROR("Failed to get size 3148 byte block from POOL_ID\n");
		return TC_FAIL;
	}

	rv = task_mem_pool_alloc(&block, POOL_ID, 3148, TICKS_NONE);
	if (rv != RC_FAIL) {
		TC_ERROR("Unexpectedly got size 3148 byte block from POOL_ID\n");
		return TC_FAIL;
	}

	task_sem_give(HELPER_SEM);    /* Activate HelperTask */
	rv = task_mem_pool_alloc(&block, POOL_ID, 3148, 20);
	if (rv != RC_OK) {
		TC_ERROR("Failed to get size 3148 byte block from POOL_ID\n");
		return TC_FAIL;
	}

	rv = task_sem_take(REGRESS_SEM, TICKS_NONE);
	if (rv != RC_OK) {
		TC_ERROR("Failed to get size 3148 byte block within 20 ticks\n");
		return TC_FAIL;
	}

	task_mem_pool_free(&block);

	return TC_PASS;
}

/**
 *
 * poolBlockGetWaitTest -
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int poolBlockGetWaitTest(void)
{
	int  rv;

	rv = task_mem_pool_alloc(&blockList[0], POOL_ID, 3000, TICKS_UNLIMITED);
	if (rv != RC_OK) {
		TC_ERROR("task_mem_pool_alloc(3000) expected %d, got %d\n", RC_OK, rv);
		return TC_FAIL;
	}

	task_sem_give(ALTERNATE_SEM);    /* Wake AlternateTask */
	evidence = 0;
	rv = task_mem_pool_alloc(&blockList[1], POOL_ID, 128, TICKS_UNLIMITED);
	if (rv != RC_OK) {
		TC_ERROR("task_mem_pool_alloc(128) expected %d, got %d\n", RC_OK, rv);
		return TC_FAIL;
	}

	switch (evidence) {
	case 0:
		TC_ERROR("task_mem_pool_alloc(128) did not block!\n");
		return TC_FAIL;
	case 1:
		break;
	case 2:
	default:
		TC_ERROR("Rescheduling did not occur after task_mem_pool_free()\n");
		return TC_FAIL;
	}

	task_mem_pool_free(&blockList[1]);

	return TC_PASS;
}

/**
 *
 * @brief Task responsible for defragmenting the pool POOL_ID
 *
 * @return N/A
 */

void DefragTask(void)
{
	task_sem_take(DEFRAG_SEM, TICKS_UNLIMITED);     /* Wait to be activated */

	task_mem_pool_defragment(POOL_ID);

	task_sem_give(REGRESS_SEM);   /* DefragTask is finished */
}

/**
 *
 * poolDefragTest -
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int poolDefragTest(void)
{
	int      rv;
	struct k_block  newBlock;

	/* Get a bunch of blocks */

	rv = poolBlockGetWork("task_mem_pool_alloc", poolBlockGetFunc,
						  defrag, ARRAY_SIZE(defrag));
	if (rv != TC_PASS) {
		return TC_FAIL;
	}


	task_sem_give(DEFRAG_SEM);    /* Activate DefragTask */

	/*
	 * Block on getting another block from the pool.
	 * This will allow DefragTask to execute so that we can get some
	 * better code coverage.  50 ticks is expected to more than sufficient
	 * time for DefragTask to finish.
	 */

	rv = task_mem_pool_alloc(&newBlock, POOL_ID, DEFRAG_BLK_TEST, 50);
	if (rv != RC_TIME) {
		TC_ERROR("task_mem_pool_alloc() returned %d, not %d\n", rv, RC_TIME);
		return TC_FAIL;
	}

	rv = task_sem_take(REGRESS_SEM, TICKS_NONE);
	if (rv != RC_OK) {
		TC_ERROR("DefragTask did not finish in allotted time!\n");
		return TC_FAIL;
	}

	/* Free the allocated blocks */

	freeBlocks(defrag, ARRAY_SIZE(defrag));

	return TC_PASS;
}

/**
 *
 * @brief Alternate task in the test suite
 *
 * This routine runs at a lower priority than RegressionTask().
 *
 * @return N/A
 */

void AlternateTask(void)
{
	task_sem_take(ALTERNATE_SEM, TICKS_UNLIMITED);

	evidence = 1;

	task_mem_pool_free(&blockList[0]);

	evidence = 2;
}

/**
 *
 * @brief Test the task_malloc() and task_free() APIs
 *
 * The heap memory pool is 256 bytes in size, and thus has only 4 blocks
 * of 64 bytes or a single block of 256 bytes. (Each block has a lesser
 * amount of usable space, due to the hidden block descriptor info the
 * kernel adds at the start of any block allocated from this memory pool.)
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int poolMallocTest(void)
{
	char *block[4];
	int  j;    /* loop counter */

	TC_PRINT("Testing task_malloc() and task_free() ...\n");

	/* allocate a large block (which consumes the entire pool buffer) */
	block[0] = task_malloc(150);
	if (block[0] == NULL) {
		TC_ERROR("150 byte allocation failed\n");
		return TC_FAIL;
	}

	/* ensure a small block can no longer be allocated */
	block[1] = task_malloc(16);
	if (block[1] != NULL) {
		TC_ERROR("16 byte allocation did not fail\n");
		return TC_FAIL;
	}

	/* return the large block */
	task_free(block[0]);

	/* allocate a small block (triggers block splitting)*/
	block[0] = task_malloc(16);
	if (block[0] == NULL) {
		TC_ERROR("16 byte allocation 0 failed\n");
		return TC_FAIL;
	}

	/* ensure a large block can no longer be allocated */
	block[1] = task_malloc(80);
	if (block[1] != NULL) {
		TC_ERROR("80 byte allocation did not fail\n");
		return TC_FAIL;
	}

	/* ensure all remaining small blocks can be allocated */
	for (j = 1; j < 4; j++) {
		block[j] = task_malloc(16);
		if (block[j] == NULL) {
			TC_ERROR("16 byte allocation %d failed\n", j);
			return TC_FAIL;
		}
	}

	/* ensure a small block can no longer be allocated */
	if (task_malloc(8) != NULL) {
		TC_ERROR("8 byte allocation did not fail\n");
		return TC_FAIL;
	}

	/* return the small blocks to pool in a "random" order */
	task_free(block[2]);
	task_free(block[0]);
	task_free(block[3]);
	task_free(block[1]);

	/* allocate large block (triggers autodefragmentation) */
	block[0] = task_malloc(100);
	if (block[0] == NULL) {
		TC_ERROR("100 byte allocation failed\n");
		return TC_FAIL;
	}

	/* ensure a small block can no longer be allocated */
	if (task_malloc(32) != NULL) {
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

void RegressionTask(void)
{
	int  tcRC;    /* test case return code */

	TC_START("Test Microkernel Memory Pools");

	TC_PRINT("Testing task_mem_pool_alloc(TICKS_NONE) ...\n");
	tcRC = poolBlockGetTest();
	if (tcRC != TC_PASS) {
		goto doneTests;
	}

	TC_PRINT("Testing task_mem_pool_alloc(timeout) ...\n");
	tcRC = poolBlockGetTimeoutTest();
	if (tcRC != TC_PASS) {
		goto doneTests;
	}

	TC_PRINT("Testing task_mem_pool_alloc(TICKS_UNLIMITED) ...\n");
	tcRC = poolBlockGetWaitTest();
	if (tcRC != TC_PASS) {
		goto doneTests;
	}

	TC_PRINT("Testing task_mem_pool_defragment() ...\n");
	tcRC = poolDefragTest();
	if (tcRC != TC_PASS) {
		goto doneTests;
	}

	tcRC = poolMallocTest();
	if (tcRC != TC_PASS) {
		goto doneTests;
	}

doneTests:
	TC_END_RESULT(tcRC);
	TC_END_REPORT(tcRC);
}
