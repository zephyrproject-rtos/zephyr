/* stackprot.c - test Stack Protector feature using canary */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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
DESCRIPTION
  This is the test program to test stack protection using canary.

  For the microkernel version (nanokernel version in brackets):
  The regression (main) task starts alternate task (fiber1), which tries
  to cause a stack check failure scenario, to test this stack check protection
  feature.

  This alternate task (fiber) will not complete it's execution by design and
  thus will not set tcRC to TC_FAIL.  When this alternate task (fiber)
  terminates, control is returned back to the regression (main) task which
  prints out a short string couple times.
 */

#include <tc_util.h>

#ifdef CONFIG_MICROKERNEL
#include <zephyr.h>

#else
#include <arch/cpu.h>

#define STACKSIZE               1024
char __stack fiberStack[STACKSIZE];

#endif /* CONFIG_MICROKERNEL */

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
 * \param name    task or fiber identification string
 *
 * RETURNS: N/A
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
 * RETURNS: N/A
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
 * Microkernel: AlternateTask
 * Nanokernel:  fiber1
 *
 * This task/fiber passes a long string to check_input function.  It terminates due
 * to stack overflow and reports "Stack Check Fail" when stack protection
 * feature is enabled.  Hence it will not execute the printLoop function and will
 * not set tcRC to TC_FAIL.  Control is transferred back to the other task.
 *
 * RETURNS: N/A
 */
#ifdef CONFIG_MICROKERNEL
void AlternateTask(void)
#else
void fiber1(void)
#endif /* ! CONFIG_MICROKERNEL */
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
 * Microkernel: RegressionTask
 * Nanokernel:  main
 *
 * This is the entry point to the test stack protection feature.  It calls
 * printLoop to print a string and alternates execution with AlternateTask
 * when the task goes to sleep in printLoop.
 *
 * RETURNS: N/A
 */

#ifdef CONFIG_MICROKERNEL
void RegressionTask(void)
#else
void main(void)
#endif /* ! CONFIG_MICROKERNEL */
{
	TC_START("Test Stack Protection Canary\n");
	TC_PRINT("Starts %s\n", __func__);

#ifdef CONFIG_MICROKERNEL
	/* Start task */
	task_start(ALTERNATETASK);       /* refer to prj.mdef file */
#else
	/* Start fiber */
	task_fiber_start(&fiberStack[0], STACKSIZE,
					 (nano_fiber_entry_t) fiber1, 0, 0, 7, 0);
#endif /* ! CONFIG_MICROKERNEL */

	if (tcRC == TC_FAIL) {
		goto errorExit;
	}

	printLoop(__func__);

errorExit:
	TC_END_RESULT(tcRC);
	TC_END_REPORT(tcRC);
}
