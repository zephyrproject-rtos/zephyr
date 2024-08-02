/* pipe_b.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "master.h"

#define PRINT_ALL_TO_N_HEADER_UNIT()                                       \
	PRINT_STRING("|   size(B) |       time/packet (nsec)       |     " \
		     "     KB/sec                |\n")

#define PRINT_ALL_TO_N() \
	PRINT_F("|%5u|%5u|%10u|%10u|%10u|%10u|%10u|%10u|\n",                 \
		putsize, putsize, puttime[0], puttime[1],                    \
		puttime[2],                                                  \
		(1000000 * putsize) / SAFE_DIVISOR(puttime[0]),              \
		(1000000 * putsize) / SAFE_DIVISOR(puttime[1]),              \
		(1000000 * putsize) / SAFE_DIVISOR(puttime[2]))

#define PRINT_1_TO_N_HEADER()                                                 \
	do {                                                                  \
		PRINT_STRING("|   size(B) |       time/packet (nsec)       |" \
			     "          KB/sec                |\n");          \
		PRINT_STRING(dashline);                                       \
	} while (0)

#define  PRINT_1_TO_N()                                                 \
	PRINT_F("|%5u|%5d|%10u|%10u|%10u|%10u|%10u|%10u|\n",	        \
		putsize,                                                \
		getsize,                                                \
		puttime[0],                                             \
		puttime[1],                                             \
		puttime[2],                                             \
		(uint32_t)(((uint64_t)putsize * 1000000U) /             \
			   SAFE_DIVISOR(puttime[0])),                   \
		(uint32_t)(((uint64_t)putsize * 1000000U) /             \
			   SAFE_DIVISOR(puttime[1])),                   \
		(uint32_t)(((uint64_t)putsize * 1000000U) /             \
			   SAFE_DIVISOR(puttime[2])))

/*
 * Function prototypes.
 */
int pipeput(struct k_pipe *pipe, enum pipe_options
		 option, int size, int count, uint32_t *time);

/*
 * Function declarations.
 */

/**
 * @brief Test the pipes transfer speed
 */
void pipe_test(void)
{
	uint32_t	putsize;
	int         getsize;
	uint32_t	puttime[3];
	int		putcount;
	int		pipe;
	uint32_t	TaskPrio = UINT32_MAX;
	int		prio;
	struct getinfo	getinfo;

	k_sem_reset(&SEM0);
	k_sem_give(&STARTRCV);

	/* action: */

	/* non-buffered operation, matching (ALL_N) */
	PRINT_STRING(dashline);
	PRINT_STRING("|                   "
		     "P I P E   M E A S U R E M E N T S"
		     "                         |\n");
	PRINT_STRING(dashline);
	PRINT_STRING("| Send data into a pipe towards a "
		     "receiving high priority task and wait       |\n");
	PRINT_STRING(dashline);
	PRINT_STRING("|                          "
		     "matching sizes (_ALL_N)"
		     "                            |\n");
	PRINT_STRING(dashline);
	PRINT_ALL_TO_N_HEADER_UNIT();
	PRINT_STRING(dashline);
	PRINT_STRING("| put | get |  no buf  | small buf| big buf  |"
		     "  no buf  | small buf| big buf  |\n");
	PRINT_STRING(dashline);

	for (putsize = 8U; putsize <= MESSAGE_SIZE_PIPE; putsize <<= 1) {
		for (pipe = 0; pipe < 3; pipe++) {
			putcount = NR_OF_PIPE_RUNS;
			pipeput(test_pipes[pipe], _ALL_N, putsize, putcount,
				 &puttime[pipe]);

			/* waiting for ack */
			k_msgq_get(&CH_COMM, &getinfo, K_FOREVER);
		}
		PRINT_ALL_TO_N();
	}
	PRINT_STRING(dashline);

	/* Test with two different sender priorities */
	for (prio = 0; prio < 2; prio++) {
		/* non-buffered operation, non-matching (1_TO_N) */
		if (prio == 0) {
			PRINT_STRING("|                      "
			 "non-matching sizes (1_TO_N) to higher priority"
						 "         |\n");
			TaskPrio = k_thread_priority_get(k_current_get());
		}
		if (prio == 1) {
			PRINT_STRING("|                      "
				 "non-matching sizes (1_TO_N) to lower priority"
						 "          |\n");
			k_thread_priority_set(k_current_get(), TaskPrio - 2);
		}
		PRINT_STRING(dashline);
		PRINT_1_TO_N_HEADER();
		PRINT_STRING("| put | get |  no buf  | small buf| big buf  |  "
			     "no buf  | small buf| big buf  |\n");
		PRINT_STRING(dashline);

	for (putsize = 8U; putsize <= (MESSAGE_SIZE_PIPE); putsize <<= 1) {
		putcount = MESSAGE_SIZE_PIPE / putsize;
		for (pipe = 0; pipe < 3; pipe++) {
			pipeput(test_pipes[pipe], _1_TO_N, putsize,
					 putcount, &puttime[pipe]);
			/* size*count == MESSAGE_SIZE_PIPE */
			/* waiting for ack */
			k_msgq_get(&CH_COMM, &getinfo, K_FOREVER);
			getsize = getinfo.size;
		}
		PRINT_1_TO_N();
	}
		PRINT_STRING(dashline);
		k_thread_priority_set(k_current_get(), TaskPrio);
	}
}


/**
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
int pipeput(struct k_pipe *pipe,
	    enum pipe_options option,
	    int size,
	    int count,
	    uint32_t *time)
{
	int i;
	unsigned int t;
	timing_t  start;
	timing_t  end;
	size_t sizexferd_total = 0;
	size_t size2xfer_total = size * count;

	/* first sync with the receiver */
	k_sem_give(&SEM0);
	start = timing_timestamp_get();
	for (i = 0; option == _1_TO_N || (i < count); i++) {
		size_t sizexferd = 0;
		size_t size2xfer = MIN(size, size2xfer_total - sizexferd_total);
		int ret;
		size_t mim_num_of_bytes = 0;

		if (option == _ALL_N) {
			mim_num_of_bytes = size2xfer;
		}
		ret = k_pipe_put(pipe, data_bench, size2xfer,
				&sizexferd, mim_num_of_bytes, K_FOREVER);

		if (ret != 0) {
			return 1;
		}
		if (option == _ALL_N && sizexferd != size2xfer) {
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
	t = (unsigned int)timing_cycles_get(&start, &end);

	*time = SYS_CLOCK_HW_CYCLES_TO_NS_AVG(t, count);

	return 0;
}
