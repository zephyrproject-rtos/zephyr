/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Test memory slab APIs
 *
 * @defgroup kernel_memory_slab_tests Memory Slab Tests
 *
 * @ingroup all_tests
 *
 * This module tests the following memory slab routines:
 *
 *     k_mem_slab_alloc
 *     k_mem_slab_free
 *     k_mem_slab_num_used_get
 *
 * @note
 * One should ensure that the block is released to the same memory slab from
 * which it was allocated, and is only released once.  Using an invalid pointer
 * will have unpredictable side effects.
 */

#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/* size of stack area used by each thread */
#define STACKSIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

/* Number of memory blocks. The minimum number of blocks needed to run the
 * test is 2
 */
#define NUMBLOCKS   4

void test_slab_get_all_blocks(void **p);
void test_slab_free_all_blocks(void **p);


K_SEM_DEFINE(SEM_HELPERDONE, 0, 1);
K_SEM_DEFINE(SEM_REGRESSDONE, 0, 1);

K_MEM_SLAB_DEFINE(map_lgblks, 1024, NUMBLOCKS, 4);


/**
 *
 * @brief Helper task
 *
 * This routine gets all blocks from the memory slab.  It uses semaphores
 * SEM_REGRESDONE and SEM_HELPERDONE to synchronize between different parts
 * of the test.
 *
 */

void helper_thread(void)
{
	void *ptr[NUMBLOCKS];           /* Pointer to memory block */

	(void)memset(ptr, 0, sizeof(ptr));    /* keep static checkers happy */
	/* Wait for part 1 to complete */
	k_sem_take(&SEM_REGRESSDONE, K_FOREVER);

	/* Part 2 of test */

	TC_PRINT("(2) - Allocate %d blocks in <%s>\n", NUMBLOCKS, __func__);

	/* Test k_mem_slab_alloc */
	test_slab_get_all_blocks(ptr);

	k_sem_give(&SEM_HELPERDONE);  /* Indicate part 2 is complete */
	/* Wait for part 3 to complete */
	k_sem_take(&SEM_REGRESSDONE, K_FOREVER);

	/*
	 * Part 4 of test.
	 * Free the first memory block.  RegressionTask is currently blocked
	 * waiting (with a timeout) for a memory block.  Freeing the memory
	 * block will unblock RegressionTask.
	 */
	TC_PRINT("(4) - Free a block in <%s> to unblock the other task "
		 "from alloc timeout\n", __func__);

	TC_PRINT("%s: About to free a memory block\n", __func__);
	k_mem_slab_free(&map_lgblks, ptr[0]);
	k_sem_give(&SEM_HELPERDONE);

	/* Part 5 of test */
	k_sem_take(&SEM_REGRESSDONE, K_FOREVER);
	TC_PRINT("(5) <%s> freeing the next block\n", __func__);
	TC_PRINT("%s: About to free another memory block\n", __func__);
	k_mem_slab_free(&map_lgblks, ptr[1]);

	/*
	 * Free all the other blocks.  The first 2 blocks are freed by this task
	 */
	for (int i = 2; i < NUMBLOCKS; i++) {
		k_mem_slab_free(&map_lgblks, ptr[i]);
	}
	TC_PRINT("%s: freed all blocks allocated by this task\n", __func__);


	k_sem_give(&SEM_HELPERDONE);

}  /* helper thread */


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
 */

void test_slab_get_all_blocks(void **p)
{
	void *errptr;   /* Pointer to block */

	for (int i = 0; i < NUMBLOCKS; i++) {
		/* Verify number of used blocks in the map */
		zassert_equal(k_mem_slab_num_used_get(&map_lgblks), i,
			      "Failed k_mem_slab_num_used_get");

		/* Get memory block */
		zassert_equal(k_mem_slab_alloc(&map_lgblks, &p[i], K_NO_WAIT), 0,
			      "Failed k_mem_slab_alloc");
	} /* for */

	/*
	 * Verify number of used blocks in the map - expect all blocks are
	 * used
	 */
	zassert_equal(k_mem_slab_num_used_get(&map_lgblks), NUMBLOCKS,
		      "Failed k_mem_slab_num_used_get");

	/* Try to get one more block and it should fail */
	zassert_equal(k_mem_slab_alloc(&map_lgblks, &errptr, K_NO_WAIT), -ENOMEM,
		      "Failed k_mem_slab_alloc");

}  /* test_slab_get_all_blocks */

/**
 *
 * @brief Free all memory blocks
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
 */

void test_slab_free_all_blocks(void **p)
{
	for (int i = 0; i < NUMBLOCKS; i++) {
		/* Verify number of used blocks in the map */
		zassert_equal(k_mem_slab_num_used_get(&map_lgblks), NUMBLOCKS - i,
			      "Failed k_mem_slab_num_used_get");

		TC_PRINT("  block ptr to free p[%d] = %p\n", i, p[i]);
		/* Free memory block */
		k_mem_slab_free(&map_lgblks, p[i]);

		TC_PRINT("map_lgblks freed %d block\n", i + 1);

	} /* for */

	/*
	 * Verify number of used blocks in the map
	 *  - should be 0 as no blocks are used
	 */

	zassert_equal(k_mem_slab_num_used_get(&map_lgblks), 0,
		      "Failed k_mem_slab_num_used_get");

}   /* test_slab_free_all_blocks */

/**
 *
 * @brief Main task to test memory slab interfaces
 *
 * @ingroup kernel_memory_slab_tests
 *
 * @details Verify that system allows for the definitions of boot-time
 * memory regions.
 * This routine calls test_slab_get_all_blocks() to get all
 * memory blocks from the map and calls test_slab_free_all_blocks()
 * to free all memory blocks. It also tries to wait (with and without
 * timeout) for a memory block.
 *
 * @see k_mem_slab_alloc(), k_mem_slab_num_used_get(),
 * memset(), k_mem_slab_free()
 */
ZTEST(memory_slab_1cpu, test_mslab)
{
	int ret_value;                  /* task_mem_map_xxx interface return value */
	void *b;                        /* Pointer to memory block */
	void *ptr[NUMBLOCKS];           /* Pointer to memory block */

	/* not strictly necessary, but keeps coverity checks happy */
	(void)memset(ptr, 0, sizeof(ptr));

	/* Part 1 of test */

	TC_PRINT("(1) - Allocate and free %d blocks "
		 "in <%s>\n", NUMBLOCKS, __func__);

	/* Test k_mem_slab_alloc */
	test_slab_get_all_blocks(ptr);

	/* Test task_mem_map_free */
	test_slab_free_all_blocks(ptr);

	k_sem_give(&SEM_REGRESSDONE);   /* Allow helper thread to run */
	/* Wait for helper thread to finish */
	k_sem_take(&SEM_HELPERDONE, K_FOREVER);

	/*
	 * Part 3 of test.
	 *
	 * helper thread got all memory blocks.  There is no free block left.
	 * The call will timeout.  Note that control does not switch back to
	 * helper thread as it is waiting for SEM_REGRESSDONE.
	 */

	TC_PRINT("(3) - Further allocation results in  timeout "
		 "in <%s>\n", __func__);

	ret_value = k_mem_slab_alloc(&map_lgblks, &b, K_MSEC(20));
	zassert_equal(-EAGAIN, ret_value,
		      "Failed k_mem_slab_alloc, retValue %d\n", ret_value);

	TC_PRINT("%s: start to wait for block\n", __func__);
	k_sem_give(&SEM_REGRESSDONE);    /* Allow helper thread to run part 4 */
	ret_value = k_mem_slab_alloc(&map_lgblks, &b, K_MSEC(50));
	zassert_equal(0, ret_value,
		      "Failed k_mem_slab_alloc, ret_value %d\n", ret_value);

	/* Wait for helper thread to complete */
	k_sem_take(&SEM_HELPERDONE, K_FOREVER);

	TC_PRINT("%s: start to wait for block\n", __func__);
	k_sem_give(&SEM_REGRESSDONE);    /* Allow helper thread to run part 5 */
	ret_value = k_mem_slab_alloc(&map_lgblks, &b, K_FOREVER);
	zassert_equal(0, ret_value,
		      "Failed k_mem_slab_alloc, ret_value %d\n", ret_value);

	/* Wait for helper thread to complete */
	k_sem_take(&SEM_HELPERDONE, K_FOREVER);


	/* Free memory block */
	TC_PRINT("%s: Used %d block\n", __func__,
		 k_mem_slab_num_used_get(&map_lgblks));
	k_mem_slab_free(&map_lgblks, b);
	TC_PRINT("%s: 1 block freed, used %d block\n",
		 __func__,  k_mem_slab_num_used_get(&map_lgblks));
}

K_THREAD_DEFINE(HELPER, STACKSIZE, helper_thread, NULL, NULL, NULL,
		7, 0, 0);

/*test case main entry*/
ZTEST_SUITE(memory_slab_1cpu, NULL, NULL, ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
