/* nop_b.c */

/*
 * Copyright (c) 1997-2010,2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "master.h"

#ifdef MICROKERNEL_CALL_BENCH

/* utilize non-public microkernel API for test purposes */

extern void _task_nop(void);

/**
 *
 * @brief Kernel entry timing test
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void call_test(void)
{
	uint32_t et; /* Elapsed Time */
	int i;

	et = BENCH_START();
	for (i = 0; i < NR_OF_NOP_RUNS; i++) {
		_task_nop();
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "kernel service request overhead",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_NOP_RUNS));
}

#endif /* MICROKERNEL_CALL_BENCH */
