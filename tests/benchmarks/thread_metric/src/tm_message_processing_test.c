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
/**   Message Processing Test                                             */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    tm_message_processing_test                          PORTABLE C      */
/*                                                           6.1.7        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    Basic test for message exchange processing.                         */
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

unsigned long tm_message_processing_counter;
unsigned long tm_message_sent[4];
unsigned long tm_message_received[4];

/* Define the test thread prototypes.  */

void tm_message_processing_thread_0_entry(void *p1, void *p2, void *p3);

/* Define the reporting function prototype.  */

void tm_message_processing_thread_report(void);

/* Define the initialization prototype.  */

void tm_message_processing_initialize(void);

/* Define main entry point.  */

int main(void)
{

	/* Initialize the test.  */
	tm_initialize(tm_message_processing_initialize);

	return 0;
}

/* Define the message processing test initialization.  */

void tm_message_processing_initialize(void)
{

	/* Create thread 0 at priority 10.  */
	tm_thread_create(0, 10, tm_message_processing_thread_0_entry);

	/* Resume thread 0.  */
	tm_thread_resume(0);

	/* Create a queue for the message passing.  */
	tm_queue_create(0);

	tm_message_processing_thread_report();
}

/* Define the message processing thread.  */
void tm_message_processing_thread_0_entry(void *p1, void *p2, void *p3)
{
	(void)p1;
	(void)p2;
	(void)p3;

	/* Initialize the source message.   */
	tm_message_sent[0] = 0x11112222;
	tm_message_sent[1] = 0x33334444;
	tm_message_sent[2] = 0x55556666;
	tm_message_sent[3] = 0x77778888;

	while (1) {
		/* Send a message to the queue.  */
		tm_queue_send(0, tm_message_sent);

		/* Receive a message from the queue.  */
		tm_queue_receive(0, tm_message_received);

		/* Check for invalid message.  */
		if (tm_message_received[3] != tm_message_sent[3]) {
			break;
		}

		/* Increment the last word of the 16-byte message.  */
		tm_message_sent[3]++;

		/* Increment the number of messages sent and received.  */
		tm_message_processing_counter++;
	}
}

/* Define the message test reporting function.  */
void tm_message_processing_thread_report(void)
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
		printf("**** Thread-Metric Message Processing Test **** Relative Time: %lu\n",
		       relative_time);

		/* See if there are any errors.  */
		if (tm_message_processing_counter == last_counter) {

			printf("ERROR: Invalid counter value(s). Error sending/receiving "
			       "messages!\n");
		}

		/* Show the time period total.  */
		printf("Time Period Total:  %lu\n\n", tm_message_processing_counter - last_counter);

		/* Save the last counter.  */
		last_counter = tm_message_processing_counter;
	}
}
