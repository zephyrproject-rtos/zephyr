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
/**   Interrupt Processing Test                                           */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    tm_interrupt_processing_test                        PORTABLE C      */
/*                                                           6.1.7        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the No-preemption interrupt processing test.      */
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

unsigned long tm_interrupt_thread_0_counter;
unsigned long tm_interrupt_handler_counter;

/* Define the test thread prototypes.  */

void tm_interrupt_thread_0_entry(void *p1, void *p2, void *p3);

/* Define the reporting function prototype.  */

void tm_interrupt_thread_report(void);

/* Define the interrupt handler.  This must be called from the RTOS.  */

void tm_interrupt_handler(void);

/* Define the initialization prototype.  */

void tm_interrupt_processing_initialize(void);

/* Define main entry point.  */

int main(void)
{

	/* Initialize the test.  */
	tm_initialize(tm_interrupt_processing_initialize);

	return 0;
}

/* Define the interrupt processing test initialization.  */

void tm_interrupt_processing_initialize(void)
{

	/* Create thread that generates the interrupt at priority 10.  */
	tm_thread_create(0, 10, tm_interrupt_thread_0_entry);

	/*
	 * Create a semaphore that will be posted from the interrupt
	 * handler.
	 */
	tm_semaphore_create(0);

	/* Resume just thread 0.  */
	tm_thread_resume(0);

	tm_interrupt_thread_report();
}

/* Define the thread that generates the interrupt.  */
void tm_interrupt_thread_0_entry(void *p1, void *p2, void *p3)
{

	int status;

	/* Pickup the semaphore since it is initialized to 1 by default. */
	status = tm_semaphore_get(0);

	/* Check for good status.  */
	if (status != TM_SUCCESS) {
		return;
	}

	while (1) {

		/*
		 * Force an interrupt. The underlying RTOS must see that the
		 * the interrupt handler is called from the appropriate software
		 * interrupt or trap.
		 */

		TM_CAUSE_INTERRUPT;

		/*
		 * We won't get back here until the interrupt processing is
		 * complete, including the setting of the semaphore from the
		 * interrupt handler.
		 */

		/* Pickup the semaphore set by the interrupt handler. */
		status = tm_semaphore_get(0);

		/* Check for good status.  */
		if (status != TM_SUCCESS) {
			return;
		}

		/* Increment this thread's counter.  */
		tm_interrupt_thread_0_counter++;
	}
}

/*
 * Define the interrupt handler.  This must be called from the RTOS trap handler.
 * To be fair, it must behave just like a processor interrupt, i.e. it must save
 * the full context of the interrupted thread during the preemption processing.
 */
void tm_interrupt_handler(void)
{
	/* Increment the interrupt count.  */
	tm_interrupt_handler_counter++;

	/* Put the semaphore from the interrupt handler.  */
	tm_semaphore_put(0);
}

/* Define the interrupt test reporting function.  */
void tm_interrupt_thread_report(void)
{

	unsigned long total;
	unsigned long last_total;
	unsigned long relative_time;
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
		printf("**** Thread-Metric Interrupt Processing Test **** Relative Time: %lu\n",
		       relative_time);

		/* Calculate the total of all the counters.  */
		total = tm_interrupt_thread_0_counter + tm_interrupt_handler_counter;

		/* Calculate the average of all the counters.  */
		average = total / 2;

		/* See if there are any errors.  */
		if ((tm_interrupt_thread_0_counter < (average - 1)) ||
		    (tm_interrupt_thread_0_counter > (average + 1)) ||
		    (tm_interrupt_handler_counter < (average - 1)) ||
		    (tm_interrupt_handler_counter > (average + 1))) {

			printf("ERROR: Invalid counter value(s). Interrupt processing test has "
			       "failed!\n");
		}

		/* Show the total interrupts for the time period.  */
		printf("Time Period Total:  %lu\n\n", tm_interrupt_handler_counter - last_total);

		/* Save the last total number of interrupts.  */
		last_total = tm_interrupt_handler_counter;
	}
}
