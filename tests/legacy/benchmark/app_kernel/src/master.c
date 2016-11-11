/* master.c */

/*
 * Copyright (c) 1997-2010,2013-2015 Wind River Systems, Inc.
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

/*
	File Naming information.
	------------------------
	Files that end with:
		_B : Is a file that contains a benchmark function
		_R : Is a file that contains the receiver task
			 of a benchmark function
 */
#include <tc_util.h>
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
 * @brief Check for keypress
 *
 * @return 1 when a keyboard key is pressed, or 0 if no keyboard support
 */
int kbhit(void)
{
	return 0;
}


/**
 *
 * @brief Prepares the test output
 *
 * @return N/A
 *
 * @param continuously   Run test till the user presses the key.
 * @param autorun        Expect user input.
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
 * @brief Close output for the test
 *
 * @return N/A
 */
void output_close(void)
{
}

/* no need to wait for user key press when using console */
#define WAIT_FOR_USER() {}


/**
 *
 * @brief Perform all selected benchmarks
 * see config.h to select or to unselect
 *
 * @return N/A
 */
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
		TC_PRINT_RUNID;
	} while (continuously && !kbhit());

	WAIT_FOR_USER();

	if (autorun) {
		task_sleep(SECONDS(2));
	}

	output_close();
}


/**
 *
 * @brief Dummy test
 *
 * @return N/A
 */
void dummy_test(void)
{
	return;
}
