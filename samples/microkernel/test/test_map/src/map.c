/* map.c - test microkernel memory map APIs */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module tests the following map routines:

   task_mem_map_alloc, task_mem_map_alloc_wait, task_mem_map_alloc_wait_timeout
   task_mem_map_free
   task_mem_map_used_get

NOTE
One should ensure that the block is released to the same map from which it was
allocated, and is only released once.  Using an invalid pointer will have
unpredictable side effects.
 */

#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>

#define NUMBLOCKS   2       /*
                             * Number of memory blocks.  This number
                             * has to be aligned with the number in MDEF file
                             * The minimum number of blocks needed to run the
                             * test is 2
                             */

static int tcRC = TC_PASS;     /* test case return code */

int testMapGetAllBlocks(void **P);
int testMapFreeAllBlocks(void **P);

/**
 *
 * verifyRetValue - verify return value
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
 * HelperTask - helper task
 *
 * This routine gets all blocks from the memory map.  It uses semaphores
 * SEM_REGRESDONE and SEM_HELPERDONE to synchronize between different parts
 * of the test.
 *
 * @return  N/A
 */

void HelperTask(void)
{
	void *ptr[NUMBLOCKS];      /* Pointer to memory block */

	task_sem_take_wait(SEM_REGRESSDONE);   /* Wait for part 1 to complete */

	/* Part 2 of test */

	TC_PRINT("Starts %s\n", __func__);

	/* Test task_mem_map_alloc */
	tcRC = testMapGetAllBlocks(ptr);
	if (tcRC == TC_FAIL) {
		TC_ERROR("Failed testMapGetAllBlocks function\n");
		goto exitTest1;          /* terminate test */
	}

	task_sem_give(SEM_HELPERDONE);  /* Indicate part 2 is complete */
	task_sem_take_wait(SEM_REGRESSDONE);  /* Wait for part 3 to complete */

	/*
	 * Part 4 of test.
	 * Free the first memory block.  RegressionTask is currently blocked
	 * waiting (with a timeout) for a memory block.  Freeing the memory
	 * block will unblock RegressionTask.
	 */

	TC_PRINT("%s: About to free a memory block\n", __func__);
	task_mem_map_free(MAP_LgBlks, &ptr[0]);
	task_sem_give(SEM_HELPERDONE);

	/* Part 5 of test */
	task_sem_take_wait(SEM_REGRESSDONE);
	TC_PRINT("%s: About to free another memory block\n", __func__);
	task_mem_map_free(MAP_LgBlks, &ptr[1]);

	/*
	 * Free all the other blocks.  The first 2 blocks are freed by this task
	 */
	for (int i = 2; i < NUMBLOCKS; i++) {
		task_mem_map_free(MAP_LgBlks, &ptr[i]);
	}
	TC_PRINT("%s: freed all blocks allocated by this task\n", __func__);

exitTest1:

	TC_END_RESULT(tcRC);
	task_sem_give(SEM_HELPERDONE);
}  /* HelperTask */


/**
 *
 * testMapGetAllBlocks - get all blocks from the memory map
 *
 * Get all blocks from the memory map.  It also tries to get one more block
 * from the map after the map is empty to verify the error return code.
 *
 * This routine tests the following:
 *
 *   task_mem_map_alloc(), task_mem_map_used_get()
 *
 * @param p    pointer to pointer of allocated blocks
 *
 * @return  TC_PASS, TC_FAIL
 */

int testMapGetAllBlocks(void **p)
{
	int retValue;  /* task_mem_map_xxx interface return value */
	void *errPtr;  /* Pointer to block */

	TC_PRINT("Function %s\n", __func__);

	/* Number of blocks in the map is defined in MDEF file */
	for (int i = 0; i < NUMBLOCKS; i++) {
		/* Verify number of used blocks in the map */
		retValue = task_mem_map_used_get(MAP_LgBlks);
		if (verifyRetValue(i, retValue)) {
			TC_PRINT("MAP_LgBlks used %d blocks\n", retValue);
		} else {
			TC_ERROR("Failed task_mem_map_used_get for MAP_LgBlks, i=%d, retValue=%d\n",
				i, retValue);
			return TC_FAIL;
		}

		/* Get memory block */
		retValue = task_mem_map_alloc(MAP_LgBlks, &p[i]);
		if (verifyRetValue(RC_OK, retValue)) {
			TC_PRINT("  task_mem_map_alloc OK, p[%d] = %p\n", i, p[i]);
		} else {
			TC_ERROR("Failed task_mem_map_alloc, i=%d, retValue %d\n",
				i, retValue);
			return TC_FAIL;
		}

	} /* for */

	/* Verify number of used blocks in the map - expect all blocks are used */
	retValue = task_mem_map_used_get(MAP_LgBlks);
	if (verifyRetValue(NUMBLOCKS, retValue)) {
		TC_PRINT("MAP_LgBlks used %d blocks\n", retValue);
	} else {
		TC_ERROR("Failed task_mem_map_used_get for MAP_LgBlks, retValue %d\n",
			retValue);
		return TC_FAIL;
	}

	/* Try to get one more block and it should fail */
	retValue = task_mem_map_alloc(MAP_LgBlks, &errPtr);
	if (verifyRetValue(RC_FAIL, retValue)) {
		TC_PRINT("  task_mem_map_alloc RC_FAIL expected as all (%d) blocks are used.\n",
			NUMBLOCKS);
	} else {
		TC_ERROR("Failed task_mem_map_alloc, expect RC_FAIL, got %d\n", retValue);
		return TC_FAIL;
	}

	PRINT_LINE;

	return TC_PASS;
}  /* testMapGetAllBlocks */

/**
 *
 * testMapFreeAllBlocks - free all memeory blocks
 *
 * This routine frees all memory blocks and also verifies that the number of
 * blocks used are correct.
 *
 * This routine tests the following:
 *
 *   task_mem_map_free(), task_mem_map_used_get()
 *
 * @param p    pointer to pointer of allocated blocks
 *
 * @return  TC_PASS, TC_FAIL
 */

int testMapFreeAllBlocks(void **p)
{
	int retValue;     /* task_mem_map_xxx interface return value */

	TC_PRINT("Function %s\n", __func__);

	/* Number of blocks in the map is defined in MDEF file */
	for (int i = 0; i < NUMBLOCKS; i++) {
		/* Verify number of used blocks in the map */
		retValue = task_mem_map_used_get(MAP_LgBlks);
		if (verifyRetValue(NUMBLOCKS - i, retValue)) {
			TC_PRINT("MAP_LgBlks used %d blocks\n", retValue);
		} else {
			TC_ERROR("Failed task_mem_map_used_get for MAP_LgBlks, expect %d, got %d\n",
				NUMBLOCKS - i, retValue);
			return TC_FAIL;
		}

		TC_PRINT("  block ptr to free p[%d] = %p\n", i, p[i]);
		/* Free memory block */
		task_mem_map_free(MAP_LgBlks, &p[i]);

		TC_PRINT("MAP_LgBlks freed %d block\n", i + 1);

	} /* for */

	/*
	 * Verify number of used blocks in the map
	 *  - should be 0 as no blocks are used
	 */

	retValue = task_mem_map_used_get(MAP_LgBlks);
	if (verifyRetValue(0, retValue)) {
		TC_PRINT("MAP_LgBlks used %d blocks\n", retValue);
	} else {
		TC_ERROR("Failed task_mem_map_used_get for MAP_LgBlks, retValue %d\n",
			retValue);
		return TC_FAIL;
	}

	PRINT_LINE;
	return TC_PASS;
}   /* testMapFreeAllBlocks */

/**
 *
 * printPointers - print the pointers
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
 * RegressionTask - main task to test task_mem_map_xxx interfaces
 *
 * This routine calls testMapGetAllBlocks() to get all memory blocks from the
 * map and calls testMapFreeAllBlocks() to free all memory blocks.  It also
 * tries to wait (with and without timeout) for a memory block.
 *
 * This routine tests the following:
 *
 *   task_mem_map_alloc_wait, task_mem_map_alloc_wait_timeout
 *
 * @return  N/A
 */

void RegressionTask(void)
{
	int   retValue;            /* task_mem_map_xxx interface return value */
	void *b;                   /* Pointer to memory block */
	void *ptr[NUMBLOCKS];      /* Pointer to memory block */

	/* Part 1 of test */

	TC_START("Test Microkernel Memory Maps");
	TC_PRINT("Starts %s\n", __func__);

	/* Test task_mem_map_alloc */
	tcRC = testMapGetAllBlocks(ptr);
	if (tcRC == TC_FAIL) {
		TC_ERROR("Failed testMapGetAllBlocks function\n");
		goto exitTest;           /* terminate test */
	}

	printPointers(ptr);
	/* Test task_mem_map_free */
	tcRC = testMapFreeAllBlocks(ptr);
	if (tcRC == TC_FAIL) {
		TC_ERROR("Failed testMapFreeAllBlocks function\n");
		goto exitTest;           /* terminate test */
	}

	printPointers(ptr);

	task_sem_give(SEM_REGRESSDONE);   /* Allow HelperTask to run */
	task_sem_take_wait(SEM_HELPERDONE);     /* Wait for HelperTask to finish */

	/*
	 * Part 3 of test.
	 *
	 * HelperTask got all memory blocks.  There is no free block left.
	 * The call will timeout.  Note that control does not switch back to
	 * HelperTask as it is waiting for SEM_REGRESSDONE.
	 */

	retValue = task_mem_map_alloc_wait_timeout(MAP_LgBlks, &b, 2);
	if (verifyRetValue(RC_TIME, retValue)) {
		TC_PRINT("%s: task_mem_map_alloc_wait_timeout timeout expected\n", __func__);
	} else {
		TC_ERROR("Failed task_mem_map_alloc_wait_timeout, retValue %d\n", retValue);
		tcRC = TC_FAIL;
		goto exitTest;           /* terminate test */
	}

	TC_PRINT("%s: start to wait for block\n", __func__);
	task_sem_give(SEM_REGRESSDONE);    /* Allow HelperTask to run part 4 */
	retValue = task_mem_map_alloc_wait_timeout(MAP_LgBlks, &b, 5);
	if (verifyRetValue(RC_OK, retValue)) {
		TC_PRINT("%s: task_mem_map_alloc_wait_timeout OK, block allocated at %p\n",
			__func__, b);
	} else {
		TC_ERROR("Failed task_mem_map_alloc_wait_timeout, retValue %d\n", retValue);
		tcRC = TC_FAIL;
		goto exitTest;           /* terminate test */
	}

	task_sem_take_wait(SEM_HELPERDONE);   /* Wait for HelperTask to complete */

	TC_PRINT("%s: start to wait for block\n", __func__);
	task_sem_give(SEM_REGRESSDONE);    /* Allow HelperTask to run part 5 */
	retValue = task_mem_map_alloc_wait(MAP_LgBlks, &b);
	if (verifyRetValue(RC_OK, retValue)) {
		TC_PRINT("%s: task_mem_map_alloc_wait OK, block allocated at %p\n",
			__func__, b);
	} else {
		TC_ERROR("Failed task_mem_map_alloc_wait, retValue %d\n", retValue);
		tcRC = TC_FAIL;
		goto exitTest;           /* terminate test */
	}

	task_sem_take_wait(SEM_HELPERDONE);   /* Wait for HelperTask to complete */


	/* Free memory block */
	TC_PRINT("%s: Used %d block\n", __func__,  task_mem_map_used_get(MAP_LgBlks));
	task_mem_map_free(MAP_LgBlks, &b);
	TC_PRINT("%s: 1 block freed, used %d block\n",
		__func__,  task_mem_map_used_get(MAP_LgBlks));

exitTest:

	TC_END_RESULT(tcRC);
	TC_END_REPORT(tcRC);
}  /* RegressionTask */
