/* stack.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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

#include "syskernel.h"

struct nano_stack nanoChannel1;
struct nano_stack nanoChannel2;

uint32_t stack1[2];
uint32_t stack2[2];

/**
 *
 * stack_test_init - initialize stacks for the test
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void stack_test_init(void)
{
	nano_stack_init(&nanoChannel1, stack1);
	nano_stack_init(&nanoChannel2, stack2);
}


/**
 *
 * stack_fiber1 - stack test context
 *
 * @param par1   Ignored parameter.
 * @param par2   Number of test loops.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void stack_fiber1(int par1, int par2)
{
	int i;
	uint32_t data;

	ARG_UNUSED(par1);

	for (i = 0; i < par2 / 2; i++) {
		data = nano_fiber_stack_pop_wait(&nanoChannel1);
		if (data != 2 * i) {
			break;
		}
		data = 2 * i;
		nano_fiber_stack_push(&nanoChannel2, data);
		data = nano_fiber_stack_pop_wait(&nanoChannel1);
		if (data != 2 * i + 1) {
			break;
		}
		data = 2 * i + 1;
		nano_fiber_stack_push(&nanoChannel2, data);
	}
}


/**
 *
 * stack_fiber2 - stack test context
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void stack_fiber2(int par1, int par2)
{
	int i;
	uint32_t data;
	int * pcounter = (int *) par1;

	for (i = 0; i < par2; i++) {
		data = i;
		nano_fiber_stack_push(&nanoChannel1, data);
		data = nano_fiber_stack_pop_wait(&nanoChannel2);
		if (data != i) {
			break;
		}
		(*pcounter)++;
	}
}


/**
 *
 * stack_fiber2 - stack test context
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void stack_fiber3(int par1, int par2)
{
	int i;
	uint32_t data;
	int * pcounter = (int *) par1;

	for (i = 0; i < par2; i++) {
		data = i;
		nano_fiber_stack_push(&nanoChannel1, data);
		data = 0xffffffff;
		while (!nano_fiber_stack_pop(&nanoChannel2, &data)) {
			fiber_yield();
		}
		if (data != i) {
			break;
		}
		(*pcounter)++;
	}
}


/**
 *
 * stack_test - the main test entry
 *
 * @return 1 if success and 0 on failure
 *
 * \NOMANUAL
 */

int stack_test(void)
{
	uint32_t t;
	int i = 0;
	int return_value = 0;

	/* test get wait & put fiber functions */
	fprintf(output_file, sz_test_case_fmt,
			"Stack #1");
	fprintf(output_file, sz_description,
			"\n\tnano_stack_init"
			"\n\tnano_fiber_stack_pop_wait"
			"\n\tnano_fiber_stack_push");
	printf(sz_test_start_fmt);

	stack_test_init();

	t = BENCH_START();

	task_fiber_start(fiber_stack1, STACK_SIZE, stack_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	task_fiber_start(fiber_stack2, STACK_SIZE, stack_fiber2, (int) &i,
					 NUMBER_OF_LOOPS, 3, 0);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	/* test get/yield & put fiber functions */
	fprintf(output_file, sz_test_case_fmt,
			"Stack #2");
	fprintf(output_file, sz_description,
			"\n\tnano_stack_init"
			"\n\tnano_fiber_stack_pop_wait"
			"\n\tnano_fiber_stack_pop"
			"\n\tnano_fiber_stack_push"
			"\n\tfiber_yield");
	printf(sz_test_start_fmt);

	stack_test_init();

	t = BENCH_START();

	i = 0;
	task_fiber_start(fiber_stack1, STACK_SIZE, stack_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	task_fiber_start(fiber_stack2, STACK_SIZE, stack_fiber3, (int) &i,
					 NUMBER_OF_LOOPS, 3, 0);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	/* test get wait & put fiber/task functions */
	fprintf(output_file, sz_test_case_fmt,
			"Stack #3");
	fprintf(output_file, sz_description,
			"\n\tnano_stack_init"
			"\n\tnano_fiber_stack_pop_wait"
			"\n\tnano_fiber_stack_push"
			"\n\tnano_task_stack_pop_wait"
			"\n\tnano_task_stack_push");
	printf(sz_test_start_fmt);

	stack_test_init();

	t = BENCH_START();

	task_fiber_start(fiber_stack1, STACK_SIZE, stack_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	for (i = 0; i < NUMBER_OF_LOOPS / 2; i++) {
		uint32_t data;
		data = 2 * i;
		nano_task_stack_push(&nanoChannel1, data);
		data = 2 * i + 1;
		nano_task_stack_push(&nanoChannel1, data);

		data = nano_task_stack_pop_wait(&nanoChannel2);
		if (data != 2 * i + 1) {
			break;
		}
		data = nano_task_stack_pop_wait(&nanoChannel2);
		if (data != 2 * i) {
			break;
		}
	}

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i * 2, t);

	return return_value;
}
