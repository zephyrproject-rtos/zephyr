/* event_b.c */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "master.h"

#ifdef EVENT_BENCH

/* #define EVENT_CHECK */
#ifdef EVENT_CHECK
static const char event_signal_err[] = "----------- Error signalling event.\n";
static const char event_test_err[] = "----------- Error testing event.\n";
static const char event_handler_err[] = "----------- Error in event handler\n";
#endif

/* global Event value */
volatile int nevent_value;

/*
 * Function prototypes.
 */
int example_handler (struct k_alert *alert);

/*
 * Function declarations.
 */

/**
 *
 * @brief Event signal speed test
 *
 * @return N/A
 */
void event_test(void)
{
	int nreturn = 0;
	int ncounter;
	u32_t et; /* elapsed time */

	PRINT_STRING(dashline, output_file);
	et = BENCH_START();
	for (ncounter = 0; ncounter < NR_OF_EVENT_RUNS; ncounter++) {
		k_alert_send(&TEST_EVENT);
#ifdef EVENT_CHECK
		if (nreturn != 0) {
			PRINT_STRING(event_signal_err, output_file);
			return; /* error */
		}
#endif /* EVENT_CHECK */
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "Signal enabled event",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_EVENT_RUNS));

	et = BENCH_START();
	for (ncounter = 0; ncounter < NR_OF_EVENT_RUNS; ncounter++) {
		k_alert_send(&TEST_EVENT);
#ifdef EVENT_CHECK
		if (nreturn != 0) {
			PRINT_STRING(event_signal_err, output_file);
			k_sleep(1);
			return; /* error */
		}
#endif /* EVENT_CHECK */
		k_alert_recv(&TEST_EVENT, K_NO_WAIT);
#ifdef EVENT_CHECK
		if (nreturn != 0) {
			PRINT_STRING(event_test_err, output_file);
			k_sleep(1);
			return; /* error */
		}
#endif /* EVENT_CHECK */
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "Signal event & Test event",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_EVENT_RUNS));

	et = BENCH_START();
	for (ncounter = 0; ncounter < NR_OF_EVENT_RUNS; ncounter++) {
		k_alert_send(&TEST_EVENT);
#ifdef EVENT_CHECK
		if (nreturn != 0) {
			PRINT_STRING(event_signal_err, output_file);
			return; /* error */
		}
#endif /* EVENT_CHECK */
		k_alert_recv(&TEST_EVENT, K_FOREVER);
#ifdef EVENT_CHECK
		if (nreturn != 0) {
			PRINT_STRING(event_test_err, output_file);
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
	TEST_EVENT.handler = example_handler;
	if (nreturn != 0) {
		PRINT_F(output_file,
			"-------- Error installing event handler.\n");
		k_sleep(1);
		return; /* error */
	}

	for (ncounter = 0; ncounter < NR_OF_EVENT_RUNS; ncounter++) {
		k_alert_send(&TEST_EVENT);
#ifdef EVENT_CHECK
		if (nreturn != 0) {
			PRINT_STRING(event_signal_err, output_file);
			k_sleep(1);
			return; /* error */
		}
		if (nevent_value != TEST_EVENT.send_count + 1) {
			PRINT_STRING(event_handler_err, output_file);
			k_sleep(1);
			return; /* error */
		}
#endif /* EVENT_CHECK */
		nevent_value = 0;
	}

	TEST_EVENT.handler = NULL;
	if (nreturn != 0) {
		PRINT_F(output_file, "Error removing event handler.\n");
		k_sleep(1);
		return; /* error */
	}

	PRINT_STRING("|    Handler responds OK"
		 "                                                      |\n",
				 output_file);

	return; /* success */
}


/**
 *
 * @brief Event handler for the tests
 *
 * The event handler for the test. Sets up nEventValue global variable.
 * This variable is used in the main test.
 *
 * @return 1
 */
int example_handler (struct k_alert *alert)
{
	ARG_UNUSED(alert);
	nevent_value = alert->send_count + 1;

	return 1;
}

#endif /* EVENT_BENCH */
