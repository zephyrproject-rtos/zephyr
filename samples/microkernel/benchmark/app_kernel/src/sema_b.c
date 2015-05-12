/* sema_b.c */

/*
 * Copyright (c) 1997-2010, 2013-2015 Wind River Systems, Inc.
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

#ifdef SEMA_BENCH


/*******************************************************************************
 *
 * sema_test - semaphore signal speed test
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void sema_test(void)
	{
	uint32_t et; /* elapsed Time */
	int i;

	PRINT_STRING(dashline, output_file);
	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
	task_sem_give(SEM0);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal semaphore",
	     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	task_sem_reset(SEM1);
	task_sem_give(STARTRCV);

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
	task_sem_give(SEM1);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waiting high pri task",
	     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
	task_sem_give(SEM1);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT,
	     "signal to waiting high pri task, with timeout",
	     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
	task_sem_give(SEM2);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waitm (2)",
	     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
	task_sem_give(SEM2);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waitm (2), with timeout",
	     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
	task_sem_give(SEM3);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waitm (3)",
	     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
	task_sem_give(SEM3);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waitm (3), with timeout",
	     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
	task_sem_give(SEM4);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waitm (4)",
	     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
	task_sem_give(SEM4);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waitm (4), with timeout",
	     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));
	}

#endif /* SEMA_BENCH */
