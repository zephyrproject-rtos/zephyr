/* fifo_r.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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

#include "receiver.h"
#include "master.h"

#ifdef FIFO_BENCH

/* queue transfer speed test */
/**
 *
 * @brief Data receive task
 *
 * @return N/A
 */
void dequtask(void)
{
	int x, i;

	for (i = 0; i < NR_OF_FIFO_RUNS; i++) {
		task_fifo_get(DEMOQX1, &x, TICKS_UNLIMITED);
	}

	for (i = 0; i < NR_OF_FIFO_RUNS; i++) {
		task_fifo_get(DEMOQX4, &x, TICKS_UNLIMITED);
	}
}

#endif /* FIFO_BENCH */
