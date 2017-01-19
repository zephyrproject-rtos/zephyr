/* pipe_r.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "receiver.h"
#include "master.h"

#ifdef PIPE_BENCH

/*
 * Function prototypes.
 */
int pipeget(kpipe_t pipe, K_PIPE_OPTION option,
			int size, int count, unsigned int* time);

/*
 * Function declarations.
 */

/* pipes transfer speed test */

/**
 *
 * @brief Receive task
 *
 * @return N/A
 */
void piperecvtask(void)
{
	int getsize;
	unsigned int gettime;
	int getcount;
	int pipe;
	int prio;
	GetInfo getinfo;

	/* matching (ALL_N) */

	for (getsize = 8; getsize <= MESSAGE_SIZE_PIPE; getsize <<= 1) {
		for (pipe = 0; pipe < 3; pipe++) {
			getcount = NR_OF_PIPE_RUNS;
			pipeget(TestPipes[pipe], _ALL_N, getsize,
				getcount, &gettime);
			getinfo.time = gettime;
			getinfo.size = getsize;
			getinfo.count = getcount;
			/* acknowledge to master */
			task_fifo_put(CH_COMM, &getinfo, TICKS_UNLIMITED);
		}
	}

	for (prio = 0; prio < 2; prio++) {
		/* non-matching (1_TO_N) */
		for (getsize = (MESSAGE_SIZE_PIPE); getsize >= 8; getsize >>= 1) {
			getcount = MESSAGE_SIZE_PIPE / getsize;
			for (pipe = 0; pipe < 3; pipe++) {
				/* size*count == MESSAGE_SIZE_PIPE */
				pipeget(TestPipes[pipe], _1_TO_N,
						getsize, getcount, &gettime);
				getinfo.time = gettime;
				getinfo.size = getsize;
				getinfo.count = getcount;
				/* acknowledge to master */
				task_fifo_put(CH_COMM, &getinfo, TICKS_UNLIMITED);
			}
		}
	}

}


/**
 *
 * @brief Read a data portion from the pipe and measure time
 *
 * @return 0 on success, 1 on error
 *
 * @param pipe     Pipe to read data from.
 * @param option   _ALL_TO_N or _1_TO_N.
 * @param size     Data chunk size.
 * @param count    Number of data chunks.
 * @param time     Total write time.
 */
int pipeget(kpipe_t pipe, K_PIPE_OPTION option, int size, int count,
			unsigned int* time)
{
	int i;
	unsigned int t;
	int sizexferd_total = 0;
	int size2xfer_total = size * count;

	/* sync with the sender */
	task_sem_take(SEM0, TICKS_UNLIMITED);
	t = BENCH_START();
	for (i = 0; _1_TO_N == option || (i < count); i++) {
		int sizexferd = 0;
		int size2xfer = min(size, size2xfer_total - sizexferd_total);
		int ret;

		ret = task_pipe_get(pipe, data_recv, size2xfer,
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
		PRINT_STRING("                             |\n",
					 output_file);
	}
	return 0;
}

#endif /* PIPE_BENCH */
