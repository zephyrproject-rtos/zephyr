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
/**   Preemptive Scheduling Test                                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    tm_preemptive_scheduling_test                       PORTABLE C      */
/*                                                           6.1.7        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the preemptive scheduling test.                   */
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

unsigned long tm_preemptive_thread_0_counter;
unsigned long tm_preemptive_thread_1_counter;
unsigned long tm_preemptive_thread_2_counter;
unsigned long tm_preemptive_thread_3_counter;
unsigned long tm_preemptive_thread_4_counter;

/* Define the test thread prototypes.  */

void tm_preemptive_thread_0_entry(void *p1, void *p2, void *p3);
void tm_preemptive_thread_1_entry(void *p1, void *p2, void *p3);
void tm_preemptive_thread_2_entry(void *p1, void *p2, void *p3);
void tm_preemptive_thread_3_entry(void *p1, void *p2, void *p3);
void tm_preemptive_thread_4_entry(void *p1, void *p2, void *p3);

/* Define the reporting function prototype.  */

void tm_preemptive_thread_report(void);

/* Define the initialization prototype.  */

void tm_preemptive_scheduling_initialize(void);

/* Define main entry point.  */

int main(void)
{

	/* Initialize the test.  */
	tm_initialize(tm_preemptive_scheduling_initialize);

	return 0;
}

/* Define the preemptive scheduling test initialization.  */

void tm_preemptive_scheduling_initialize(void)
{

	/* Create thread 0 at priority 10.  */
	tm_thread_create(0, 10, tm_preemptive_thread_0_entry);

	/* Create thread 1 at priority 9.  */
	tm_thread_create(1, 9, tm_preemptive_thread_1_entry);

	/* Create thread 2 at priority 8.  */
	tm_thread_create(2, 8, tm_preemptive_thread_2_entry);

	/* Create thread 3 at priority 7.  */
	tm_thread_create(3, 7, tm_preemptive_thread_3_entry);

	/* Create thread 4 at priority 6.  */
	tm_thread_create(4, 6, tm_preemptive_thread_4_entry);

	/* Resume just thread 0.  */
	tm_thread_resume(0);

	tm_preemptive_thread_report();
}

/* Define the first preemptive thread.  */
void tm_preemptive_thread_0_entry(void *p1, void *p2, void *p3)
{
	(void)p1;
	(void)p2;
	(void)p3;

	while (1) {
		/* Resume thread 1.  */
		tm_thread_resume(1);

		/*
		 * We won't get back here until threads 1, 2, 3, and 4 all execute and
		 * self-suspend.
		 */

		/* Increment this thread's counter.  */
		tm_preemptive_thread_0_counter++;
	}
}

/* Define the second preemptive thread.  */
void tm_preemptive_thread_1_entry(void *p1, void *p2, void *p3)
{
	(void)p1;
	(void)p2;
	(void)p3;

	while (1) {

		/* Resume thread 2.  */
		tm_thread_resume(2);

		/*
		 * We won't get back here until threads 2, 3, and 4 all execute and
		 * self-suspend.
		 */

		/* Increment this thread's counter.  */
		tm_preemptive_thread_1_counter++;

		/* Suspend self!  */
		tm_thread_suspend(1);
	}
}

/* Define the third preemptive thread.  */
void tm_preemptive_thread_2_entry(void *p1, void *p2, void *p3)
{
	(void)p1;
	(void)p2;
	(void)p3;

	while (1) {

		/* Resume thread 3.  */
		tm_thread_resume(3);

		/*
		 * We won't get back here until threads 3 and 4 execute and
		 * self-suspend.
		 */

		/* Increment this thread's counter.  */
		tm_preemptive_thread_2_counter++;

		/* Suspend self!  */
		tm_thread_suspend(2);
	}
}

/* Define the fourth preemptive thread.  */
void tm_preemptive_thread_3_entry(void *p1, void *p2, void *p3)
{
	(void)p1;
	(void)p2;
	(void)p3;

	while (1) {

		/* Resume thread 4.  */
		tm_thread_resume(4);

		/*
		 * We won't get back here until thread 4 executes and
		 * self-suspends.
		 */

		/* Increment this thread's counter.  */
		tm_preemptive_thread_3_counter++;

		/* Suspend self!  */
		tm_thread_suspend(3);
	}
}

/* Define the fifth preemptive thread.  */
void tm_preemptive_thread_4_entry(void *p1, void *p2, void *p3)
{
	(void)p1;
	(void)p2;
	(void)p3;

	while (1) {

		/* Increment this thread's counter.  */
		tm_preemptive_thread_4_counter++;

		/* Self suspend thread 4.  */
		tm_thread_suspend(4);
	}
}

/* Define the preemptive test reporting function.  */
void tm_preemptive_thread_report(void)
{

	unsigned long total;
	unsigned long relative_time;
	unsigned long last_total;
	unsigned long average;

	/* Initialize the last total.  */
	last_total = 0;

	/* Initialize the relative time.  */
	relative_time = 0;

	while (1) {

		/* Sleep to allow the test to run.  */
		tm_thread_sleep(TM_TEST_DURATION);

		/* Increment the relative time.  */
		relative_time = relative_time + TM_TEST_DURATION;

		/* Print results to the stdio window.  */
		printf("**** Thread-Metric Preemptive Scheduling Test **** Relative Time: %lu\n",
		       relative_time);

		/* Calculate the total of all the counters.  */
		total = tm_preemptive_thread_0_counter + tm_preemptive_thread_1_counter +
			tm_preemptive_thread_2_counter + tm_preemptive_thread_3_counter +
			tm_preemptive_thread_4_counter;

		/* Calculate the average of all the counters.  */
		average = total / 5;

		/* See if there are any errors.  */
		if ((tm_preemptive_thread_0_counter < (average - 1)) ||
		    (tm_preemptive_thread_0_counter > (average + 1)) ||
		    (tm_preemptive_thread_1_counter < (average - 1)) ||
		    (tm_preemptive_thread_1_counter > (average + 1)) ||
		    (tm_preemptive_thread_2_counter < (average - 1)) ||
		    (tm_preemptive_thread_2_counter > (average + 1)) ||
		    (tm_preemptive_thread_3_counter < (average - 1)) ||
		    (tm_preemptive_thread_3_counter > (average + 1)) ||
		    (tm_preemptive_thread_4_counter < (average - 1)) ||
		    (tm_preemptive_thread_4_counter > (average + 1))) {

			printf("ERROR: Invalid counter value(s). Preemptive counters should not be "
			       "more that 1 different than the average!\n");
			printf("   Average: %lu, 0: %lu, 1: %lu, 2: %lu, 3: %lu, 4: %lu\n",
				average, tm_preemptive_thread_0_counter,
				tm_preemptive_thread_1_counter,
				tm_preemptive_thread_2_counter,
				tm_preemptive_thread_3_counter,
				tm_preemptive_thread_4_counter);
		}

		/* Show the time period total.  */
		printf("Time Period Total:  %lu\n\n", total - last_total);

		/* Save the last total.  */
		last_total = total;
	}
}
