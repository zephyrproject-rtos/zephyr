/* pipe_r.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "receiver.h"
#include "master.h"

/*
 * Function prototypes.
 */
int pipeget(struct k_pipe *pipe, enum pipe_options option,
			int size, int count, unsigned int *time);

/*
 * Function declarations.
 */

/* pipes transfer speed test */

/**
 * @brief Receive task
 */
void piperecvtask(void)
{
	int getsize;
	unsigned int gettime;
	int getcount;
	int pipe;
	int prio;
	struct getinfo getinfo;

	/* matching (ALL_N) */

	for (getsize = 8; getsize <= MESSAGE_SIZE_PIPE; getsize <<= 1) {
		for (pipe = 0; pipe < 3; pipe++) {
			getcount = NR_OF_PIPE_RUNS;
			pipeget(test_pipes[pipe], _ALL_N, getsize,
				getcount, &gettime);
			getinfo.time = gettime;
			getinfo.size = getsize;
			getinfo.count = getcount;
			/* acknowledge to master */
			k_msgq_put(&CH_COMM, &getinfo, K_FOREVER);
		}
	}

	for (prio = 0; prio < 2; prio++) {
		/* non-matching (1_TO_N) */
		for (getsize = (MESSAGE_SIZE_PIPE); getsize >= 8; getsize >>= 1) {
			getcount = MESSAGE_SIZE_PIPE / getsize;
			for (pipe = 0; pipe < 3; pipe++) {
				/* size*count == MESSAGE_SIZE_PIPE */
				pipeget(test_pipes[pipe], _1_TO_N,
						getsize, getcount, &gettime);
				getinfo.time = gettime;
				getinfo.size = getsize;
				getinfo.count = getcount;
				/* acknowledge to master */
				k_msgq_put(&CH_COMM, &getinfo, K_FOREVER);
			}
		}
	}

}


/**
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
int pipeget(struct k_pipe *pipe, enum pipe_options option, int size, int count,
			unsigned int *time)
{
	int i;
	unsigned int t;
	timing_t  start;
	timing_t  end;
	size_t sizexferd_total = 0;
	size_t size2xfer_total = size * count;

	/* sync with the sender */
	k_sem_take(&SEM0, K_FOREVER);
	start = timing_timestamp_get();
	for (i = 0; option == _1_TO_N || (i < count); i++) {
		size_t sizexferd = 0;
		size_t size2xfer = MIN(size, size2xfer_total - sizexferd_total);

		sizexferd = k_pipe_read(pipe, data_recv, size2xfer, K_FOREVER);
		if (sizexferd  < 0) {
			return 1;
		}

		if (option == _ALL_N  && sizexferd != size2xfer) {
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

	end = timing_timestamp_get();
	t = (unsigned int) timing_cycles_get(&start, &end);
	*time = SYS_CLOCK_HW_CYCLES_TO_NS_AVG(t, count);

	return 0;
}
