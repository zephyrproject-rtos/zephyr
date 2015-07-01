/* pipe_r.c */

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
 * piperecvtask - receive task
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
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
			task_fifo_put_wait(CH_COMM, &getinfo); /* acknowledge to master */
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
				task_fifo_put_wait(CH_COMM, &getinfo); /* acknowledge to master */
			}
		}
	}

}


/**
 *
 * pipeget - read a data portion from the pipe and measure time
 *
 * RETURNS: 0 on success, 1 on error
 *
 * @param pipe     Pipe to read data from.
 * @param option   _ALL_TO_N or _1_TO_N.
 * @param size     Data chunk size.
 * @param count    Number of data chunks.
 * @param time     Total write time.
 *
 * \NOMANUAL
 */

int pipeget(kpipe_t pipe, K_PIPE_OPTION option, int size, int count,
			unsigned int* time)
{
	int i;
	unsigned int t;
	int sizexferd_total = 0;
	int size2xfer_total = size * count;

	/* sync with the sender */
	task_sem_take_wait(SEM0);
	t = BENCH_START();
	for (i = 0; _1_TO_N == option || (i < count); i++) {
		int sizexferd = 0;
		int size2xfer = min(size, size2xfer_total - sizexferd_total);
		int ret;

		ret = task_pipe_get_wait(pipe, data_recv, size2xfer,
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
		PRINT_STRING("                             |\n",
					 output_file);
	}
	return 0;
}

#endif /* PIPE_BENCH */
