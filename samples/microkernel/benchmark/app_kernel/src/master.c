/* master.c */

/*
 * Copyright (c) 1997-2010,2013-2015 Wind River Systems, Inc.
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

/*
	File Naming information.
	------------------------
	Files that end with:
		_B : Is a file that contains a benchmark function
		_R : Is a file that contains the receiver task
			 of a benchmark function
 */

#include "master.h"

char Msg[MAX_MSG];
char data_bench[OCTET_TO_SIZEOFUNIT(MESSAGE_SIZE)];

#ifdef PIPE_BENCH
kpipe_t TestPipes[] = {PIPE_NOBUFF, PIPE_SMALLBUFF, PIPE_BIGBUFF};
#endif
const char dashline[] =
	"|--------------------------------------"
	"---------------------------------------|\n";
char sline[SLINE_LEN + 1];
const char newline[] = "\n";

FILE * output_file;

/*
 * Time in timer cycles necessary to read time.
 * Used for correction in time measurements.
 */
uint32_t tm_off;

/**
 *
 * kbhit - check for keypress
 *
 * @return 1 when a keyboard key is pressed, or 0 if no keyboard support
 *
 * \NOMANUAL
 */

int kbhit(void)
{
	return 0;
}


/**
 *
 * init_output - prepares the test output
 *
 * @return N/A
 *
 * @param continuously   Run test till the user presses the key.
 * @param autorun        Expect user input.
 *
 * \NOMANUAL
 */

void init_output(int *continuously, int *autorun)
{
	ARG_UNUSED(continuously);
	ARG_UNUSED(autorun);

	/*
	 * send all printf and fprintf to console
	 */
	output_file = stdout;
}

/**
 *
 * output_close - close output for the test
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void output_close(void)
{
}

/* no need to wait for user key press when using console */
#define WAIT_FOR_USER() {}


/**
 *
 * BenchTask - perform all selected benchmarks
 *
 * @return N/A
 *
 * \NOMANUAL
 */

/* see config.h to select or to unselect*/
void BenchTask(void)
{
	int autorun = 0, continuously = 0;

	init_output(&continuously, &autorun);
	bench_test_init();

	PRINT_STRING(newline, output_file);
	do {
		PRINT_STRING(dashline, output_file);
		PRINT_STRING("|          S I M P L E   S E R V I C E    "
					 "M E A S U R E M E N T S  |  nsec    |\n",
					 output_file);
		PRINT_STRING(dashline, output_file);
		task_start(RECVTASK);
		call_test();
		queue_test();
		sema_test();
		mutex_test();
		memorymap_test();
		mempool_test();
		event_test();
		mailbox_test();
		pipe_test();
		PRINT_STRING("|         END OF TESTS                     "
					 "                                   |\n",
					 output_file);
		PRINT_STRING(dashline, output_file);
		PRINT_STRING("PROJECT EXECUTION SUCCESSFUL\n",output_file);
	} while (continuously && !kbhit());

	WAIT_FOR_USER();

	/*
	 * Make a 2 second delay. sys_clock_ticks_per_sec in this context is
	 * a number of system times ticks in a second.
	 */
	if (autorun) {
		task_sleep(2 * sys_clock_ticks_per_sec);
	}

	output_close();
}


/**
 *
 * dummy_test - dummy test
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void dummy_test(void)
{
	return;
}
