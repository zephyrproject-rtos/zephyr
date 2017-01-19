/* stackprot.c - test Stack Protector feature using canary */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/*
DESCRIPTION
  This is the test program to test stack protection using canary.

  The main task starts a second task, which generates a stack check failure.
  By design, the second task will not complete its execution and
  will not set tcRC to TC_FAIL.
 */

#include <tc_util.h>

#include <zephyr.h>

static int count = 0;
static int tcRC = TC_PASS;

void check_input(const char *name, const char *input);

/**
 *
 * printLoop
 *
 * This function calls check_input 6 times with the input name and a short
 * string, which is printed properly by check_input.
 *
 * @param name    caller identification string
 *
 * @return N/A
 */

void printLoop(const char *name)
{
	while (count < 6) {
		/* A short input string to check_input.  It will pass. */
		check_input(name, "Stack ok");
		count++;
	}
}

/**
 *
 * check_input
 *
 * This function copies the input string to a buffer of 16 characters and
 * prints the name and buffer as a string.  If the input string is longer
 * than the buffer, an error condition is detected.
 *
 * When stack protection feature is enabled (see prj.conf file), the
 * system error handler is invoked and reports a "Stack Check Fail" error.
 * When stack protection feature is not enabled, the system crashes with
 * error like: Trying to execute code outside RAM or ROM.
 *
 * @return N/A
 */

void check_input(const char *name, const char *input)
{
	/* Stack will overflow when input is more than 16 characters */
	char buf[16];

	strcpy(buf, input);
	TC_PRINT("%s: %s\n", name, buf);
}

/**
 *
 * This task passes a long string to check_input function.  It terminates due
 * to stack overflow and reports "Stack Check Fail" when stack protection
 * feature is enabled.  Hence it will not execute the printLoop function
 * and will not set tcRC to TC_FAIL.
 *
 * @return N/A
 */
void AlternateTask(void)
{
	TC_PRINT("Starts %s\n", __func__);
	check_input(__func__, "Input string is too long and stack overflowed!\n");
	/*
	 * Expect this task to terminate due to stack check fail and will not
	 * execute pass here.
	 */
	printLoop(__func__);

	tcRC = TC_FAIL;
}

/**
 *
 * This is the entry point to the test stack protection feature.
 * It starts the task that tests stack protection, then prints out
 * a few messages before terminating.
 *
 * @return N/A
 */

void RegressionTask(void)
{
	TC_START("Test Stack Protection Canary\n");
	TC_PRINT("Starts %s\n", __func__);

	/* Start task */
	task_start(ALTERNATETASK);       /* refer to prj.mdef file */

	if (tcRC == TC_FAIL) {
		goto errorExit;
	}

	printLoop(__func__);

errorExit:
	TC_END_RESULT(tcRC);
	TC_END_REPORT(tcRC);
}
