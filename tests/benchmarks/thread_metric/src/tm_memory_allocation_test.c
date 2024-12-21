/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** Thread-Metric Component                                               */
/**                                                                       */
/**   Memory Allocation Test                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    tm_memory_allocation_test                           PORTABLE C      */
/*                                                           6.1.7        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the Message exchange processing test.             */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  10-15-2021     William E. Lamie         Initial Version 6.1.7         */
/*                                                                        */
/**************************************************************************/
#include "tm_api.h"

/* Define the counters used in the demo application...  */

unsigned long tm_memory_allocation_counter;

/* Define the test thread prototypes.  */

void tm_memory_allocation_thread_0_entry(void *p1, void *p2, void *p3);

/* Define the reporting function prototype.  */

void tm_memory_allocation_thread_report(void);

/* Define the initialization prototype.  */

void tm_memory_allocation_initialize(void);

/* Define main entry point.  */

int main(void)
{

	/* Initialize the test.  */
	tm_initialize(tm_memory_allocation_initialize);

	return 0;
}

/* Define the memory allocation processing test initialization.  */

void tm_memory_allocation_initialize(void)
{
	/* Create a memory pool.  */
	tm_memory_pool_create(0);

	/* Create thread 0 at priority 10.  */
	tm_thread_create(0, 10, tm_memory_allocation_thread_0_entry);

	/* Resume thread 0.  */
	tm_thread_resume(0);

	tm_memory_allocation_thread_report();
}

/* Define the memory allocation processing thread.  */
void tm_memory_allocation_thread_0_entry(void *p1, void *p2, void *p3)
{

	int status;
	unsigned char *memory_ptr;

	while (1) {

		/* Allocate memory from pool.  */
		tm_memory_pool_allocate(0, &memory_ptr);

		/* Release the memory back to the pool.  */
		status = tm_memory_pool_deallocate(0, memory_ptr);

		/* Check for invalid memory allocation/deallocation.  */
		if (status != TM_SUCCESS) {
			break;
		}

		/* Increment the number of memory allocations sent and received.  */
		tm_memory_allocation_counter++;
	}
}

/* Define the memory allocation test reporting function.  */
void tm_memory_allocation_thread_report(void)
{

	unsigned long last_counter;
	unsigned long relative_time;

	/* Initialize the last counter.  */
	last_counter = 0;

	/* Initialize the relative time.  */
	relative_time = 0;

	while (1) {

		/* Sleep to allow the test to run.  */
		tm_thread_sleep(TM_TEST_DURATION);

		/* Increment the relative time.  */
		relative_time = relative_time + TM_TEST_DURATION;

		/* Print results to the stdio window.  */
		printf("**** Thread-Metric Memory Allocation Test **** Relative Time: %lu\n",
		       relative_time);

		/* See if there are any errors.  */
		if (tm_memory_allocation_counter == last_counter) {

			printf("ERROR: Invalid counter value(s). Error allocating/deallocating "
			       "memory!\n");
		}

		/* Show the time period total.  */
		printf("Time Period Total:  %lu\n\n", tm_memory_allocation_counter - last_counter);

		/* Save the last counter.  */
		last_counter = tm_memory_allocation_counter;
	}
}
