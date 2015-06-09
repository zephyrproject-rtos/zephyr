/* pipe_b.c */

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

/*******************************************************************************
 *
 * pipe_test - test the pipes transfer speed
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void pipe_test(void)
{
	uint32_t	putsize;
	int         getsize;
	uint32_t	puttime[3];
	int		putcount;
	int		pipe;
	kpriority_t	TaskPrio;
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

			task_fifo_get_wait(CH_COMM, &getinfo); /* waiting for ack */
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
				task_fifo_get_wait(CH_COMM, &getinfo); /* waiting for ack */
				getsize = getinfo.size;
			}
			PRINT_1_TO_N();
		}
		PRINT_STRING(dashline, output_file);
		task_priority_set(task_id_get(), TaskPrio);
	}
}


/*******************************************************************************
 *
 * pipeput - write a data portion to the pipe and measure time
 *
 * RETURNS: 0 on success, 1 on error
 *
 * @param pipe     The pipe to be tested.
 * @param option   _ALL_TO_N or _1_TO_N.
 * @param size     Data chunk size.
 * @param count    Number of data chunks.
 * @param time     Total write time.
 *
 * \NOMANUAL
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

		ret = task_pipe_put_wait(pipe, data_bench, size2xfer,
								 &sizexferd, option);
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
			PRINT_STRING("| Tick occured. Results may be inaccurate        ",
						 output_file);
		}
		PRINT_STRING("                             |\n", output_file);
	}
	return 0;
}

#endif /* PIPE_BENCH */
