/* syskernel.h */

/*
 * Copyright (c) 1997-2010, 2014 Wind River Systems, Inc.
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

#ifndef SYSKERNEK_H
#define SYSKERNEK_H

#include <timestamp.h>

#include <stdio.h>
#include <toolchain.h>

#define STACK_SIZE 2048
#define NUMBER_OF_LOOPS 5000

extern char fiber_stack1[STACK_SIZE];
extern char fiber_stack2[STACK_SIZE];

extern FILE *	output_file;

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
