/* syskernel.h */

/*
 * Copyright (c) 1997-2010, 2014 Wind River Systems, Inc.
 *
 *SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYSKERNEK_H
#define SYSKERNEK_H

#include <timestamp.h>

#include <stdio.h>
#include <toolchain.h>

#define STACK_SIZE 2048
#define NUMBER_OF_LOOPS 5000

extern K_THREAD_STACK_DEFINE(thread_stack1, STACK_SIZE);
extern K_THREAD_STACK_DEFINE(thread_stack2, STACK_SIZE);
extern struct k_thread thread_data1;
extern struct k_thread thread_data2;

extern FILE *output_file;

extern const char sz_success[];
extern const char sz_partial[];
extern const char sz_fail[];

extern u32_t number_of_loops;

#define sz_module_title_fmt	"\nMODULE: %s"
#define sz_module_result_fmt	"\n\nPROJECT EXECUTION %s\n"
#define sz_module_end_fmt	"\nEND MODULE"

#define sz_date_fmt		"\nBUILD_DATE: %s %s"
#define sz_kernel_ver_fmt	"\nKERNEL VERSION: 0x%x"
#define sz_description		"\nTEST COVERAGE: %s"

#define sz_test_case_fmt	"\n\nTEST CASE: %s"
#define sz_test_start_fmt	"\nStarting test. Please wait..."
#define sz_case_result_fmt	"\nTEST RESULT: %s"
#define sz_case_details_fmt	"\nDETAILS: %s"
#define sz_case_end_fmt		"\nEND TEST CASE"
#define sz_case_timing_fmt	"%u nSec"

int check_result(int i, u32_t ticks);

int sema_test(void);
int lifo_test(void);
int fifo_test(void);
int stack_test(void);
void begin_test(void);

static inline u32_t BENCH_START(void)
{
	u32_t et;

	begin_test();
	et = TIME_STAMP_DELTA_GET(0);
	return et;
}

#endif /* SYSKERNEK_H */
