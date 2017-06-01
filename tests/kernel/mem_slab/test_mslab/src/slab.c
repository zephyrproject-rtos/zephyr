/* map.c - test memory slab APIs */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test memory slab APIs
 *
 * This module tests the following map routines:
 *
 *     k_mem_slab_alloc
 *     task_mem_map_free
 *     task_mem_map_used_get
 *
 * @note
 * One should ensure that the block is released to the same map from which it
 * was allocated, and is only released once.  Using an invalid pointer will
 * have unpredictable side effects.
 */

#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>

/* size of stack area used by each thread */
#define STACKSIZE 1024 + CONFIG_TEST_EXTRA_STACKSIZE

/* Number of memory blocks. The minimum number of blocks needed to run the
 * test is 2
 */
#define NUMBLOCKS   2

static int tcRC = TC_PASS;     /* test case return code */

int testSlabGetAllBlocks(void **P);
int testSlabFreeAllBlocks(void **P);


K_SEM_DEFINE(SEM_HELPERDONE, 0, 1);
K_SEM_DEFINE(SEM_REGRESSDONE, 0, 1);

K_MEM_SLAB_DEFINE(MAP_LgBlks, 1024, NUMBLOCKS, 4);


/**
 *
 * @brief Verify return value
 *
 * This routine verifies current value against expected value
 * and returns true if they are the same.
 *
 * @param expectRetValue     expect value
 * @param currentRetValue    current value
 *
 * @return  true, false
 */

bool verifyRetValue(int expectRetValue, int currentRetValue)
{
	return (expectRetValue == currentRetValue);

} /* verifyRetValue */

/**
 *
 * @brief Helper task
 *
 * This routine gets all blocks from the memory slab.  It uses semaphores
 * SEM_REGRESDONE and SEM_HELPERDONE to synchronize between different parts
 * of the test.
 *
 * @return  N/A
 */

void HelperTask(void)
{
	void *ptr[NUMBLOCKS];           /* Pointer to memory block */

	memset(ptr, 0, sizeof(ptr));    /* keep static checkers happy */
	/* Wait for part 1 to complete */
	k_sem_take(&SEM_REGRESSDONE, K_FOREVER);

	/* Part 2 of test */

	TC_PRINT("Starts %s\n", __func__);
	PRINT_LINE;
	TC_PRINT("(2) - Allocate %d blocks in <%s>\n", NUMBLOCKS, __func__);
	PRINT_LINE;

	/* Test k_mem_slab_alloc */
	tcRC = testSlabGetAllBlocks(ptr);
	if (tcRC == TC_FAIL) {
		TC_ERROR("Failed testSlabGetAllBlocks function\n");
		goto exitTest1;          /* terminate test */
	}

	k_sem_give(&SEM_HELPERDONE);  /* Indicate part 2 is complete */
	/* Wait for part 3 to complete */
	k_sem_take(&SEM_REGRESSDONE, K_FOREVER);

	/*
	 * Part 4 of test.
	 * Free the first memory block.  RegressionTask is currently blocked
	 * waiting (with a timeout) for a memory block.  Freeing the memory
	 * block will unblock RegressionTask.
	 */
	PRINT_LINE;
	TC_PRINT("(4) - Free a block in <%s> to unblock the other task "
		 "from alloc timeout\n", __func__);
	PRINT_LINE;

	TC_PRINT("%s: About to free a memory block\n", __func__);
	k_mem_slab_free(&MAP_LgBlks, &ptr[0]);
	k_sem_give(&SEM_HELPERDONE);

	/* Part 5 of test */
	k_sem_take(&SEM_REGRESSDONE, K_FOREVER);
	PRINT_LINE;
	TC_PRINT("(5) <%s> freeing the next block\n", __func__);
	PRINT_LINE;
	TC_PRINT("%s: About to free another memory block\n", __func__);
	k_mem_slab_free(&MAP_LgBlks, &ptr[1]);

	/*
	 * Free all the other blocks.  The first 2 blocks are freed by this task
	 */
	for (int i = 2; i < NUMBLOCKS; i++) {
		k_mem_slab_free(&MAP_LgBlks, &ptr[i]);
	}
	TC_PRINT("%s: freed all blocks allocated by this task\n", __func__);

exitTest1:

	TC_END_RESULT(tcRC);
	k_sem_give(&SEM_HELPERDONE);
}  /* HelperTask */


/**
 *
 * @brief Get all blocks from the memory slab
 *
 * Get all blocks from the memory slab.  It also tries to get one more block
 * from the map after the map is empty to verify the error return code.
 *
 * This routine tests the following:
 *
 *   k_mem_slab_alloc(), k_mem_slab_num_used_get()
 *
 * @param p    pointer to pointer of allocated blocks
 *
 * @return  TC_PASS, TC_FAIL
 */

int testSlabGetAllBlocks(void **p)
{
	int retValue;   /* task_mem_map_xxx interface return value */
	void *errPtr;   /* Pointer to block */

	TC_PRINT("Function %s\n", __func__);

	for (int i = 0; i < NUMBLOCKS; i++) {
		/* Verify number of used blocks in the map */
		retValue = k_mem_slab_num_used_get(&MAP_LgBlks);
		if (verifyRetValue(i, retValue)) {
			TC_PRINT("MAP_LgBlks used %d blocks\n", retValue);
		} else {
			TC_ERROR("Failed task_mem_map_used_get for "
				 "MAP_LgBlks, i=%d, retValue=%d\n",
				 i, retValue);
			return TC_FAIL;
		}

		/* Get memory block */
		retValue = k_mem_slab_alloc(&MAP_LgBlks, &p[i], K_NO_WAIT);
		if (verifyRetValue(0, retValue)) {
			TC_PRINT("  k_mem_slab_alloc OK, p[%d] = %p\n",
				 i, p[i]);
		} else {
			TC_ERROR("Failed k_mem_slab_alloc, i=%d, "
				 "retValue %d\n", i, retValue);
			return TC_FAIL;
		}

	} /* for */

	/* Verify number of used blocks in the map - expect all blocks are
	 * used
	 */
	retValue = k_mem_slab_num_used_get(&MAP_LgBlks);
	if (verifyRetValue(NUMBLOCKS, retValue)) {
		TC_PRINT("MAP_LgBlks used %d blocks\n", retValue);
	} else {
		TC_ERROR("Failed task_mem_map_used_get for MAP_LgBlks, "
			 "retValue %d\n", retValue);
		return TC_FAIL;
	}

	/* Try to get one more block and it should fail */
	retValue = k_mem_slab_alloc(&MAP_LgBlks, &errPtr, K_NO_WAIT);
	if (verifyRetValue(-ENOMEM, retValue)) {
		TC_PRINT("  k_mem_slab_alloc RC_FAIL expected as all (%d) "
			 "blocks are used.\n", NUMBLOCKS);
	} else {
		TC_ERROR("Failed k_mem_slab_alloc, expect RC_FAIL, got %d\n",
			 retValue);
		return TC_FAIL;
	}

	PRINT_LINE;

	return TC_PASS;
}  /* testSlabGetAllBlocks */

/**
 *
 * @brief Free all memeory blocks
 *
 * This routine frees all memory blocks and also verifies that the number of
 * blocks used are correct.
 *
 * This routine tests the following:
 *
 *   k_mem_slab_free(&), k_mem_slab_num_used_get(&)
 *
 * @param p    pointer to pointer of allocated blocks
 *
 * @return  TC_PASS, TC_FAIL
 */

int testSlabFreeAllBlocks(void **p)
{
	int retValue;     /* task_mem_map_xxx interface return value */

	TC_PRINT("Function %s\n", __func__);

	for (int i = 0; i < NUMBLOCKS; i++) {
		/* Verify number of used blocks in the map */
		retValue = k_mem_slab_num_used_get(&MAP_LgBlks);
		if (verifyRetValue(NUMBLOCKS - i, retValue)) {
			TC_PRINT("MAP_LgBlks used %d blocks\n", retValue);
		} else {
			TC_ERROR("Failed task_mem_map_used_get for "
				 "MAP_LgBlks, expect %d, got %d\n",
				 NUMBLOCKS - i, retValue);
			return TC_FAIL;
		}

		TC_PRINT("  block ptr to free p[%d] = %p\n", i, p[i]);
		/* Free memory block */
		k_mem_slab_free(&MAP_LgBlks, &p[i]);

		TC_PRINT("MAP_LgBlks freed %d block\n", i + 1);

	} /* for */

	/*
	 * Verify number of used blocks in the map
	 *  - should be 0 as no blocks are used
	 */

	retValue = k_mem_slab_num_used_get(&MAP_LgBlks);
	if (verifyRetValue(0, retValue)) {
		TC_PRINT("MAP_LgBlks used %d blocks\n", retValue);
	} else {
		TC_ERROR("Failed task_mem_map_used_get for MAP_LgBlks, "
			 "retValue %d\n", retValue);
		return TC_FAIL;
	}

	PRINT_LINE;
	return TC_PASS;
}   /* testSlabFreeAllBlocks */

/**
 *
 * @brief Print the pointers
 *
 * This routine prints out the pointers.
 *
 * @param pointer    pointer to pointer of allocated blocks
 *
 * @return  N/A
 */
void printPointers(void **pointer)
{
	TC_PRINT("%s: ", __func__);
	for (int i = 0; i < NUMBLOCKS; i++) {
		TC_PRINT("p[%d] = %p, ", i, pointer[i]);
	}

	TC_PRINT("\n");
	PRINT_LINE;

}  /* printPointers */

/**
 *
 * @brief Main task to test task_mem_map_xxx interfaces
 *
 * This routine calls testSlabGetAllBlocks() to get all memory blocks from the
 * map and calls testSlabFreeAllBlocks() to free all memory blocks.  It also
 * tries to wait (with and without timeout) for a memory block.
 *
 * This routine tests the following:
 *
 *   k_mem_slab_alloc
 *
 * @return  N/A
 */

void RegressionTask(void)
{
	int retValue;                   /* task_mem_map_xxx interface return value */
	void *b;                        /* Pointer to memory block */
	void *ptr[NUMBLOCKS];           /* Pointer to memory block */

	/* not strictly necessary, but keeps coverity checks happy */
	memset(ptr, 0, sizeof(ptr));

	/* Part 1 of test */

	TC_START("Test Kernel memory slabs");
	TC_PRINT("Starts %s\n", __func__);
	PRINT_LINE;
	TC_PRINT("(1) - Allocate and free %d blocks "
		 "in <%s>\n", NUMBLOCKS, __func__);
	PRINT_LINE;

	/* Test k_mem_slab_alloc */
	tcRC = testSlabGetAllBlocks(ptr);
	if (tcRC == TC_FAIL) {
		TC_ERROR("Failed testSlabGetAllBlocks function\n");
		goto exitTest;           /* terminate test */
	}

	printPointers(ptr);
	/* Test task_mem_map_free */
	tcRC = testSlabFreeAllBlocks(ptr);
	if (tcRC == TC_FAIL) {
		TC_ERROR("Failed testSlabFreeAllBlocks function\n");
		goto exitTest;           /* terminate test */
	}

	k_sem_give(&SEM_REGRESSDONE);   /* Allow HelperTask to run */
	/* Wait for HelperTask to finish */
	k_sem_take(&SEM_HELPERDONE, K_FOREVER);

	/*
	 * Part 3 of test.
	 *
	 * HelperTask got all memory blocks.  There is no free block left.
	 * The call will timeout.  Note that control does not switch back to
	 * HelperTask as it is waiting for SEM_REGRESSDONE.
	 */

	PRINT_LINE;
	TC_PRINT("(3) - Further allocation results in  timeout "
		 "in <%s>\n", __func__);
	PRINT_LINE;

	retValue = k_mem_slab_alloc(&MAP_LgBlks, &b, 20);
	if (verifyRetValue(-EAGAIN, retValue)) {
		TC_PRINT("%s: k_mem_slab_alloc times out which is "
			 "expected\n", __func__);
	} else {
		TC_ERROR("Failed k_mem_slab_alloc, retValue %d\n", retValue);
		tcRC = TC_FAIL;
		goto exitTest;           /* terminate test */
	}

	TC_PRINT("%s: start to wait for block\n", __func__);
	k_sem_give(&SEM_REGRESSDONE);    /* Allow HelperTask to run part 4 */
	retValue = k_mem_slab_alloc(&MAP_LgBlks, &b, 50);
	if (verifyRetValue(0, retValue)) {
		TC_PRINT("%s: k_mem_slab_alloc OK, block allocated at %p\n",
			 __func__, b);
	} else {
		TC_ERROR("Failed k_mem_slab_alloc, retValue %d\n", retValue);
		tcRC = TC_FAIL;
		goto exitTest;           /* terminate test */
	}

	/* Wait for HelperTask to complete */
	k_sem_take(&SEM_HELPERDONE, K_FOREVER);

	TC_PRINT("%s: start to wait for block\n", __func__);
	k_sem_give(&SEM_REGRESSDONE);    /* Allow HelperTask to run part 5 */
	retValue = k_mem_slab_alloc(&MAP_LgBlks, &b, K_FOREVER);
	if (verifyRetValue(0, retValue)) {
		TC_PRINT("%s: k_mem_slab_alloc OK, block allocated at %p\n",
			 __func__, b);
	} else {
		TC_ERROR("Failed k_mem_slab_alloc, retValue %d\n", retValue);
		tcRC = TC_FAIL;
		goto exitTest;           /* terminate test */
	}

	/* Wait for HelperTask to complete */
	k_sem_take(&SEM_HELPERDONE, K_FOREVER);


	/* Free memory block */
	TC_PRINT("%s: Used %d block\n", __func__,
		 k_mem_slab_num_used_get(&MAP_LgBlks));
	k_mem_slab_free(&MAP_LgBlks, &b);
	TC_PRINT("%s: 1 block freed, used %d block\n",
		 __func__,  k_mem_slab_num_used_get(&MAP_LgBlks));

exitTest:

	TC_END_RESULT(tcRC);
	TC_END_REPORT(tcRC);
}  /* RegressionTask */

K_THREAD_DEFINE(HELPERTASK, STACKSIZE, HelperTask, NULL, NULL, NULL,
		7, 0, K_NO_WAIT);

K_THREAD_DEFINE(REGRESSTASK, STACKSIZE, RegressionTask, NULL, NULL, NULL,
		5, 0, K_NO_WAIT);
