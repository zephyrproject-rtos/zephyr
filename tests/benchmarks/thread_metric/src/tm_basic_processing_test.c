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
/**   Basic Processing Test                                               */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    tm_basic_processing_test                            PORTABLE C      */
/*                                                           6.1.7        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the basic test for determining board processing   */
/*    capabilities                                                        */
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

volatile unsigned long tm_basic_processing_counter;

/*
 * Test array.  We will just do a series of calculations on the
 * test array to eat up processing bandwidth. The idea is that
 * all RTOSes should produce the same metric here if everything
 * else is equal, e.g. processor speed, memory speed, etc.
 */

volatile unsigned long tm_basic_processing_array[1024];

/* Define the test thread prototypes.  */

void tm_basic_processing_thread_0_entry(void *p1, void *p2, void *p3);

/* Define the reporting function prototype.  */

void tm_basic_processing_thread_report(void);

/* Define the initialization prototype.  */

void tm_basic_processing_initialize(void);

/* Define main entry point.  */

int main(void)
{
	/* Initialize the test.  */
	tm_initialize(tm_basic_processing_initialize);

	return 0;
}

/* Define the basic processing test initialization.  */

void tm_basic_processing_initialize(void)
{
	/* Create thread 0 at priority 10.  */
	tm_thread_create(0, 10, tm_basic_processing_thread_0_entry);

	/* Resume thread 0.  */
	tm_thread_resume(0);

	tm_basic_processing_thread_report();
}

/* Define the basic processing thread.  */
void tm_basic_processing_thread_0_entry(void *p1, void *p2, void *p3)
{
	int i;

	(void)p1;
	(void)p2;
	(void)p3;

	/* Initialize the test array.   */
	for (i = 0; i < 1024; i++) {

		/* Clear the basic processing array.  */
		tm_basic_processing_array[i] = 0;
	}

	while (1) {

		/*
		 * Loop through the basic processing array, add the previous
		 * contents with the contents of the tm_basic_processing_counter
		 * and xor the result with the previous value...   just to eat
		 * up some time.
		 */

		for (i = 0; i < 1024; i++) {

			/* Update each array entry.  */
			tm_basic_processing_array[i] =
				(tm_basic_processing_array[i] + tm_basic_processing_counter) ^
				tm_basic_processing_array[i];
		}

		/* Increment the basic processing counter.  */
		tm_basic_processing_counter++;
	}
}

/* Define the basic processing reporting function.  */
void tm_basic_processing_thread_report(void)
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
		printf("**** Thread-Metric Basic Single Thread Processing Test **** Relative Time: "
		       "%lu\n",
		       relative_time);

		/* See if there are any errors.  */
		if (tm_basic_processing_counter == last_counter) {

			printf("ERROR: Invalid counter value(s). Basic processing thread died!\n");
		}

		/* Show the time period total.  */
		printf("Time Period Total:  %lu\n\n", tm_basic_processing_counter - last_counter);

		/* Save the last counter.  */
		last_counter = tm_basic_processing_counter;
	}
}
