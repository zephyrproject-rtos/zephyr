/* pipe_b.c */

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

#include "master.h"

#ifdef PIPE_BENCH

#ifdef FLOAT
#define PRINT_ALL_TO_N_HEADER_UNIT()                                       \
	PRINT_STRING("|   size(B) |       time/packet (usec)       |         "\
		  " MB/sec                |\n", output_file);

#define PRINT_ALL_TO_N() \
	PRINT_F(output_file,						\
	     "|%5lu|%5lu|%10.3f|%10.3f|%10.3f|%10.3f|%10.3f|%10.3f|\n",   \
	     putsize, putsize, puttime[0] / 1000.0, puttime[1] / 1000.0,  \
	     puttime[2] / 1000.0,                                         \
	     (1000.0 * putsize) / puttime[0],                             \
	     (1000.0 * putsize) / puttime[1],                             \
	     (1000.0 * putsize) / puttime[2])

#define PRINT_1_TO_N_HEADER()                                             \
	PRINT_STRING("|   size(B) |       time/packet (usec)       |        "\
		  "  MB/sec                |\n", output_file);            \
	PRINT_STRING(dashline, output_file);

#define  PRINT_1_TO_N()                                               \
	PRINT_F(output_file,						\
	     "|%5lu|%5d|%10.3f|%10.3f|%10.3f|%10.3f|%10.3f|%10.3f|\n",\
	     putsize,                                                 \
	     getsize,                                                 \
	     puttime[0] / 1000.0,                                     \
	     puttime[1] / 1000.0,                                     \
	     puttime[2] / 1000.0,                                     \
	     (1000.0 * putsize) / puttime[0],                         \
	     (1000.0 * putsize) / puttime[1],                         \
	     (1000.0 * putsize) / puttime[2])

#else
#define PRINT_ALL_TO_N_HEADER_UNIT()                                       \
	PRINT_STRING("|   size(B) |       time/packet (nsec)       |         "\
		  " KB/sec                |\n", output_file);

#define PRINT_ALL_TO_N() \
	PRINT_F(output_file,                                                 \
	     "|%5lu|%5lu|%10lu|%10lu|%10lu|%10lu|%10lu|%10lu|\n",         \
	     putsize, putsize, puttime[0], puttime[1],                    \
	     puttime[2],                                                  \
	     (1000000 * putsize) / puttime[0],                            \
	     (1000000 * putsize) / puttime[1],                            \
	     (1000000 * putsize) / puttime[2]);

#define PRINT_1_TO_N_HEADER()                                             \
	PRINT_STRING("|   size(B) |       time/packet (nsec)       |        "\
		  "  KB/sec                |\n", output_file);            \
	PRINT_STRING(dashline, output_file);

#define  PRINT_1_TO_N()                                              \
	PRINT_F(output_file,                                            \
	     "|%5lu|%5d|%10lu|%10lu|%10lu|%10lu|%10lu|%10lu|\n",     \
	     putsize,                                                \
	     getsize,                                                \
	     puttime[0],                                             \
	     puttime[1],                                             \
	     puttime[2],                                             \
	     (uint32_t)((1000000 * (uint64_t)putsize) / puttime[0]), \
	     (uint32_t)((1000000 * (uint64_t)putsize) / puttime[1]), \
	     (uint32_t)((1000000 * (uint64_t)putsize) / puttime[2]));
#endif /* FLOAT */

/*
 * Function prototypes.
 */
int pipeput(kpipe_t pipe, K_PIPE_OPTION
		 option, int size, int count, uint32_t *time);

/*
 * Function declarations.
 */

/**
 *
 * @brief Test the pipes transfer speed
 *
 * @return N/A
 */
void pipe_test(void)
{
	uint32_t	putsize;
	int         getsize;
	uint32_t	puttime[3];
	int		putcount;
	int		pipe;
	kpriority_t	TaskPrio = UINT32_MAX;
	int		prio;
	GetInfo	getinfo;

	task_sem_reset(SEM0);
	task_sem_give(STARTRCV);

	/* action: */

	/* non-buffered operation, matching (ALL_N) */
	PRINT_STRING(dashline, output_file);
	PRINT_STRING("|                   "
				 "P I P E   M E A S U R E M E N T S"
				 "                         |\n", output_file);
	PRINT_STRING(dashline, output_file);
	PRINT_STRING("| Send data into a pipe towards a "
				 "receiving high priority task and wait       |\n",
				 output_file);
	PRINT_STRING(dashline, output_file);
	PRINT_STRING("|                          "
				 "matching sizes (_ALL_N)"
				 "                            |\n", output_file);
	PRINT_STRING(dashline, output_file);
	PRINT_ALL_TO_N_HEADER_UNIT();
	PRINT_STRING(dashline, output_file);
	PRINT_STRING("| put | get |  no buf  | small buf| big buf  |"
				 "  no buf  | small buf| big buf  |\n", output_file);
	PRINT_STRING(dashline, output_file);

	for (putsize = 8; putsize <= MESSAGE_SIZE_PIPE; putsize <<= 1) {
		for (pipe = 0; pipe < 3; pipe++) {
			putcount = NR_OF_PIPE_RUNS;
			pipeput(TestPipes[pipe], _ALL_N, putsize, putcount,
				 &puttime[pipe]);

			/* waiting for ack */
			task_fifo_get(CH_COMM, &getinfo, TICKS_UNLIMITED);
		}
		PRINT_ALL_TO_N();
	}
	PRINT_STRING(dashline, output_file);

	/* Test with two different sender priorities */
	for (prio = 0; prio < 2; prio++) {
		/* non-buffered operation, non-matching (1_TO_N) */
		if (prio == 0) {
			PRINT_STRING("|                      "
						 "non-matching sizes (1_TO_N) to higher priority"
						 "         |\n", output_file);
			TaskPrio = task_priority_get();
		}
		if (prio == 1) {
			PRINT_STRING("|                      "
						 "non-matching sizes (1_TO_N) to lower priority"
						 "          |\n", output_file);
			task_priority_set(task_id_get(), TaskPrio - 2);
		}
		PRINT_STRING(dashline, output_file);
		PRINT_1_TO_N_HEADER();
		PRINT_STRING("| put | get |  no buf  | small buf| big buf  |  "
					 "no buf  | small buf| big buf  |\n", output_file);
		PRINT_STRING(dashline, output_file);

		for (putsize = 8; putsize <= (MESSAGE_SIZE_PIPE); putsize <<= 1) {
			putcount = MESSAGE_SIZE_PIPE / putsize;
			for (pipe = 0; pipe < 3; pipe++) {
				pipeput(TestPipes[pipe], _1_TO_N, putsize,
						 putcount, &puttime[pipe]);
				/* size*count == MESSAGE_SIZE_PIPE */
				/* waiting for ack */
				task_fifo_get(CH_COMM, &getinfo, TICKS_UNLIMITED);
				getsize = getinfo.size;
			}
			PRINT_1_TO_N();
		}
		PRINT_STRING(dashline, output_file);
		task_priority_set(task_id_get(), TaskPrio);
	}
}


/**
 *
 * @brief Write a data portion to the pipe and measure time
 *
 * @return 0 on success, 1 on error
 *
 * @param pipe     The pipe to be tested.
 * @param option   _ALL_TO_N or _1_TO_N.
 * @param size     Data chunk size.
 * @param count    Number of data chunks.
 * @param time     Total write time.
 */
int pipeput(kpipe_t pipe, K_PIPE_OPTION option, int size, int count, uint32_t *time)
{
	int i;
	unsigned int t;
	int sizexferd_total = 0;
	int size2xfer_total = size * count;

	/* first sync with the receiver */
	task_sem_give(SEM0);
	t = BENCH_START();
	for (i = 0; _1_TO_N == option || (i < count); i++) {
		int sizexferd = 0;
		int size2xfer = min(size, size2xfer_total - sizexferd_total);
		int ret;

		ret = task_pipe_put(pipe, data_bench, size2xfer,
							 &sizexferd, option, TICKS_UNLIMITED);
		if (RC_OK != ret) {
			return 1;
		}
		if (_ALL_N == option && sizexferd != size2xfer) {
			return 1;
		}

		sizexferd_total += sizexferd;
		if (size2xfer_total == sizexferd_total) {
			break;
		}

		if (size2xfer_total < sizexferd_total) {
			return 1;
		}
	}

	t = TIME_STAMP_DELTA_GET(t);
	*time = SYS_CLOCK_HW_CYCLES_TO_NS_AVG(t, count);
	if (bench_test_end() < 0) {
		if (high_timer_overflow()) {
			PRINT_STRING("| Timer overflow. Results are invalid            ",
						 output_file);
		} else {
			PRINT_STRING("| Tick occurred. Results may be inaccurate       ",
						 output_file);
		}
		PRINT_STRING("                             |\n", output_file);
	}
	return 0;
}

#endif /* PIPE_BENCH */
