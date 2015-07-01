/* event_b.c */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
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

#include "master.h"

#ifdef EVENT_BENCH

/* define a event */
#define TEST_EVENT 0

/* #define EVENT_CHECK */
#ifdef EVENT_CHECK
static char EventSignalErr[] = "------------ Error signalling event.\n";
static char EventTestErr[] = "------------ Error testing event.\n";
static char EventHandlerErr[] = "------------ Error in event handler.\n";
#endif

/* global Event value */
volatile int nEventValue;

/*
 * Function prototypes.
 */
int example_handler (int event);

/*
 * Function declarations.
 */

/**
 *
 * event_test - event signal speed test
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void event_test(void)
{
	int nReturn;
	int nCounter;
	uint32_t et; /* elapsed time */

	PRINT_STRING(dashline, output_file);
	et = BENCH_START();
	for (nCounter = 0; nCounter < NR_OF_EVENT_RUNS; nCounter++) {
		nReturn = task_event_send(TEST_EVENT);
#ifdef EVENT_CHECK
		if (nReturn != RC_OK) {
			PRINT_STRING(EventSignalErr, output_file);
			return; /* error */
		}
#endif /* EVENT_CHECK */
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "Signal enabled event",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_EVENT_RUNS));

	et = BENCH_START();
	for (nCounter = 0; nCounter < NR_OF_EVENT_RUNS; nCounter++) {
		nReturn = task_event_send(TEST_EVENT);
#ifdef EVENT_CHECK
		if (nReturn != RC_OK) {
			PRINT_STRING(EventSignalErr, output_file);
			task_sleep(SLEEP_TIME);
			return; /* error */
		}
#endif /* EVENT_CHECK */
		nReturn = task_event_recv(TEST_EVENT);
#ifdef EVENT_CHECK
		if (nReturn != RC_OK) {
			PRINT_STRING(EventTestErr, output_file);
			task_sleep(SLEEP_TIME);
			return; /* error */
		}
#endif /* EVENT_CHECK */
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "Signal event & Test event",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_EVENT_RUNS));

	et = BENCH_START();
	for (nCounter = 0; nCounter < NR_OF_EVENT_RUNS; nCounter++) {
		nReturn = task_event_send(TEST_EVENT);
#ifdef EVENT_CHECK
		if (nReturn != RC_OK) {
			PRINT_STRING(EventSignalErr, output_file);
			return; /* error */
		}
#endif /* EVENT_CHECK */
		nReturn = task_event_recv_wait(TEST_EVENT);
#ifdef EVENT_CHECK
		if (nReturn != RC_OK) {
			PRINT_STRING(EventTestErr, output_file);
			return; /* error */
		}
#endif /* EVENT_CHECK */
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "Signal event & TestW event",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_EVENT_RUNS));

	PRINT_STRING("| Signal event with installed handler"
				 "                                         |\n", output_file);
	nReturn = task_event_set_handler (TEST_EVENT, example_handler);
	if (nReturn != RC_OK) {
		PRINT_F(output_file, "-------- Error installing event handler.\n");
		task_sleep(SLEEP_TIME);
		return; /* error */
	}

	for (nCounter = 0; nCounter < NR_OF_EVENT_RUNS; nCounter++) {
		nReturn = task_event_send(TEST_EVENT);
#ifdef EVENT_CHECK
		if (nReturn != RC_OK) {
			PRINT_STRING(EventSignalErr, output_file);
			task_sleep(SLEEP_TIME);
			return; /* error */
		}
		if (nEventValue != TEST_EVENT + 1) {
			PRINT_STRING(EventHandlerErr, output_file);
			task_sleep(SLEEP_TIME);
			return; /* error */
		}
#endif /* EVENT_CHECK */
		nEventValue = 0;
	}

	nReturn = task_event_set_handler (TEST_EVENT, NULL);
	if (nReturn != RC_OK) {
		PRINT_F(output_file, "Error removing event handler.\n");
		task_sleep(SLEEP_TIME);
		return; /* error */
	}

	PRINT_STRING("|    Handler responds OK"
				 "                                                      |\n",
				 output_file);

	return; /* success */
}


/**
 *
 * example_handler - event handler for the tests
 *
 * The event handler for the test. Sets up nEventValue global variable.
 * This variable is used in the main test.
 *
 * @return 1
 *
 * \NOMANUAL
 */

int example_handler (int event)
{
	nEventValue = event + 1;

	return 1;
}

#endif /* EVENT_BENCH */
