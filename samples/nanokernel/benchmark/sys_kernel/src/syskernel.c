/* syskernel.c */

/*
 * Copyright (c) 1997-2010, 2012-2014 Wind River Systems, Inc.
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

#ifdef CONFIG_MICROKERNEL
#include <vxmicro.h>
#else
#include <nanokernel.h>
#include <nanokernel/cpu.h>
#endif

#include "syskernel.h"

#include <string.h>

/* #define FLOAT */

char fiber_stack1[STACK_SIZE];
char fiber_stack2[STACK_SIZE];

char Msg[256];

FILE * output_file;

const char sz_success[] = "SUCCESSFUL";
const char sz_partial[] = "PARTIAL";
const char sz_fail[] = "FAILED";

const char sz_module_title_fmt[] = "\nMODULE: %s";
const char sz_module_result_fmt[] = "\n\nVXMICRO PROJECT EXECUTION %s\n";
const char sz_module_end_fmt[] = "\nEND MODULE";

const char sz_date_fmt[] = "\nBUILD_DATE: %s %s";
const char sz_kernel_ver_fmt[] = "\nKERNEL VERSION: 0x%x";
const char sz_description[] = "\nDESCRIPTION: %s";

const char sz_test_case_fmt[] = "\n\nTEST CASE: %s";
const char sz_test_start_fmt[] = "\nStarting test %s. Please wait...";
const char sz_case_result_fmt[] = "\nTEST RESULT: %s";
const char sz_case_details_fmt[] = "\nDETAILS: %s";
const char sz_case_end_fmt[] = "\nEND TEST CASE";
#ifdef FLOAT
const char sz_case_timing_fmt[] = "%6.3f nSec";
#else
const char sz_case_timing_fmt[] = "%ld nSec";
#endif

/* time necessary to read the time */
uint32_t tm_off;

/*******************************************************************************
 *
 * begin_test - get the time ticks before test starts
 *
 * Routine does necessary preparations for the test to start
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */
void begin_test (void)
    {
    /*
       Invoke bench_test_start in order to be able to use
       tCheck static variable.
    */
    bench_test_start ();
    }

/*******************************************************************************
 *
 * check_result - checks number of tests and calculate average time
 *
 * RETURNS: 1 if success and 0 on failure
 *
 * \NOMANUAL
 */

int check_result (
    int i, /* number of tests */
    uint32_t t /* time in ticks for the whole test */
    )
    {
    /*
       bench_test_end checks tCheck static variable.
       bench_test_start modifies it
    */
    if (bench_test_end () != 0)
	{
	fprintf (output_file, sz_case_result_fmt, sz_fail);
	fprintf (output_file, sz_case_details_fmt,
		 "timer tick happened. Results are inaccurate");
	fprintf (output_file, sz_case_end_fmt);
	return 0;
	}
    if (i != NUMBER_OF_LOOPS)
	{
	fprintf (output_file, sz_case_result_fmt, sz_fail);
	fprintf (output_file, sz_case_details_fmt, "loop counter = ");
	fprintf (output_file, "%i !!!", i);
	fprintf (output_file, sz_case_end_fmt);
	return 0;
	}
    fprintf (output_file, sz_case_result_fmt, sz_success);
    fprintf (output_file, sz_case_details_fmt,
	     "Average time for 1 iteration: ");
    fprintf (output_file, sz_case_timing_fmt,
	     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(t, NUMBER_OF_LOOPS));

    fprintf (output_file, sz_case_end_fmt);
    return 1;
    }


/*******************************************************************************
 *
 * kbhit - check for a key press
 *
 * RETURNS: 1 when a keyboard key is pressed, or 0 if no keyboard support
 *
 * \NOMANUAL
 */

int kbhit (void)
{
    return 0;
}


/*******************************************************************************
 *
 * init_output - prepares the test output
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void init_output (
    int *continuously /* run test till the user presses the key */
    )
    {
    ARG_UNUSED (continuously);

    /*
     * send all printf and fprintf to console
     */
    output_file = stdout;
    }


/*******************************************************************************
 *
 * output_close - close output for the test
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void output_close (void)
    {
    }

/*******************************************************************************
 *
 * SysKernelBench - perform all selected benchmarks
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

#ifdef CONFIG_MICROKERNEL
void SysKernelBench (void)
#else
void main (void)
#endif
    {
    int	    continuously = 0;
    int	    test_result;

    init_output (&continuously);
    bench_test_init ();

    do
	{
	fprintf (output_file, sz_module_title_fmt, "Nanokernel API test");
	fprintf (output_file, sz_kernel_ver_fmt, kernel_version_get ());
	fprintf (output_file,
		 "\n\nEach test below are repeated %d times and the average\n"
		 "time for one iteration is displayed.", NUMBER_OF_LOOPS);

	test_result = 0;

	test_result += sema_test ();
	test_result += lifo_test ();
	test_result += fifo_test ();
	test_result += stack_test ();

	if (test_result)
	    {
	    /* sema, lifo, fifo, stack account for twelve tests in total */
	    if (test_result == 12)
		fprintf (output_file, sz_module_result_fmt, sz_success);
	    else
		fprintf (output_file, sz_module_result_fmt, sz_partial);
	    }
	else
	    fprintf (output_file, sz_module_result_fmt, sz_fail);

	}
    while (continuously && !kbhit ());

    output_close ();
    }
