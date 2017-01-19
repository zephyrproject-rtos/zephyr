/* syskernel.h */

/*
 * Copyright (c) 1997-2010, 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYSKERNEK_H
#define SYSKERNEK_H

#include <timestamp.h>

#include <stdio.h>
#include <toolchain.h>

#define STACK_SIZE 2048
#define NUMBER_OF_LOOPS 5000

extern char fiber_stack1[STACK_SIZE];
extern char fiber_stack2[STACK_SIZE];

extern FILE *output_file;

extern const char sz_success[];
extern const char sz_partial[];
extern const char sz_fail[];

extern const char sz_module_title_fmt[];
extern const char sz_module_result_fmt[];
extern const char sz_module_end_fmt[];

extern const char sz_product_fmt[];
extern const char sz_kernel_ver_fmt[];
extern const char sz_description[];

extern const char sz_test_case_fmt[];
extern const char sz_test_start_fmt[];
extern const char sz_case_result_fmt[];
extern const char sz_case_details_fmt[];
extern const char sz_case_end_fmt[];
extern const char sz_case_timing_fmt[];

int check_result(int i, uint32_t ticks);

int sema_test(void);
int lifo_test(void);
int fifo_test(void);
int stack_test(void);
void begin_test(void);

static inline uint32_t BENCH_START(void)
{
	uint32_t et;

	begin_test();
	et = TIME_STAMP_DELTA_GET(0);
	return et;
}

#endif /* SYSKERNEK_H */
