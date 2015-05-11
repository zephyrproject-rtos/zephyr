/* lifo.c */

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

#include "syskernel.h"

struct nano_lifo nanoLifo1;
struct nano_lifo nanoLifo2;

static struct nano_fifo nanoFifo_sync; /* for synchronization */

/*******************************************************************************
 *
 * lifo_test_init - initialize LIFOs for the test
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void lifo_test_init (void)
    {
    nano_lifo_init (&nanoLifo1);
    nano_lifo_init (&nanoLifo2);
    }


/*******************************************************************************
 *
 * lifo_fiber1 - lifo test context
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void lifo_fiber1 (
    int par1, /* ignored parameter */
    int par2 /* number of test loops */
    )
    {
    int i;
    int element_a[2];
    int element_b[2];
    int * pelement;

    ARG_UNUSED (par1);

    for (i = 0; i < par2 / 2; i++) {
	pelement = (int *) nano_fiber_lifo_get_wait (&nanoLifo1);
	if (pelement[1] != 2 * i)
	    break;
	element_a[1] = 2 * i;
	nano_fiber_lifo_put (&nanoLifo2, element_a);
	pelement = (int *) nano_fiber_lifo_get_wait (&nanoLifo1);
	if (pelement[1] != 2 * i + 1)
	    break;
	element_b[1] = 2 * i + 1;
	nano_fiber_lifo_put (&nanoLifo2, element_b);
	}
    /* wait till it is safe to end: */
    nano_fiber_fifo_get_wait (&nanoFifo_sync);
    }


/*******************************************************************************
 *
 * lifo_fiber2 - lifo test context
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void lifo_fiber2 (
    int par1, /* address of the counter */
    int par2 /* number of test cycles */
    )
    {
    int i;
    int element[2];
    int * pelement;
    int * pcounter = (int *) par1;

    for (i = 0; i < par2; i++) {
	element[1] = i;
	nano_fiber_lifo_put (&nanoLifo1, element);
	pelement = (int *) nano_fiber_lifo_get_wait (&nanoLifo2);
	if (pelement[1] != i)
	    break;
	(*pcounter)++;
	}
    /* wait till it is safe to end: */
    nano_fiber_fifo_get_wait (&nanoFifo_sync);
    }

/*******************************************************************************
 *
 * lifo_fiber3 - lifo test context
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void lifo_fiber3 (
    int par1, /* address of the counter */
    int par2 /* number of test loops */
    )
    {
    int i;
    int element[2];
    int * pelement;
    int * pcounter = (int *) par1;

    for (i = 0; i < par2; i++) {
	element[1] = i;
	nano_fiber_lifo_put (&nanoLifo1, element);
	while (NULL == (pelement = (int *) nano_fiber_lifo_get (&nanoLifo2)))
	    fiber_yield ();
	if (pelement[1] != i)
	    break;
	(*pcounter)++;
	}
    /* wait till it is safe to end: */
    nano_fiber_fifo_get_wait (&nanoFifo_sync);
    }

/*******************************************************************************
 *
 * lifo_test - the main test entry
 *
 * RETURNS: 1 if success and 0 on failure
 *
 * \NOMANUAL
 */

int lifo_test (void)
    {
    uint32_t t;
    int i = 0;
    int return_value = 0;
    int element[2];
    int j;

    nano_fifo_init (&nanoFifo_sync);

    /* test get wait & put fiber functions */
    fprintf (output_file, sz_test_case_fmt,
	     "LIFO channel - 'nano_fiber_lifo_get_wait'");
    fprintf (output_file, sz_description,
	     "testing 'nano_lifo_init','nano_fiber_lifo_get_wait',"
	     " 'nano_fiber_lifo_put' functions;");
    printf (sz_test_start_fmt, "'nano_fiber_lifo_get_wait'");

    lifo_test_init ();

    t = BENCH_START ();

    task_fiber_start (fiber_stack1, STACK_SIZE, lifo_fiber1, 0,
		    NUMBER_OF_LOOPS, 3, 0);
    task_fiber_start (fiber_stack2, STACK_SIZE, lifo_fiber2, (int) &i,
		    NUMBER_OF_LOOPS, 3, 0);

    t = TIME_STAMP_DELTA_GET (t);

    return_value += check_result (i, t);

    /* fiber contexts have done their job, they can stop now safely: */
    for (j = 0; j < 2; j++)
	nano_task_fifo_put (&nanoFifo_sync, (void *) element);

    /* test get/yield & put fiber functions */
    fprintf (output_file, sz_test_case_fmt,
	     "LIFO channel - 'nano_fiber_lifo_get'");
    fprintf (output_file, sz_description,
	     "testing 'nano_lifo_init','nano_fiber_lifo_get_wait',"
	     " 'nano_fiber_lifo_get',\n");
    fprintf (output_file,
	     "\t'nano_fiber_lifo_put', 'fiber_yield' functions;");
    printf (sz_test_start_fmt, "'nano_fiber_lifo_get'");

    lifo_test_init ();

    t = BENCH_START ();

    i = 0;
    task_fiber_start (fiber_stack1, STACK_SIZE, lifo_fiber1, 0,
		    NUMBER_OF_LOOPS, 3, 0);
    task_fiber_start (fiber_stack2, STACK_SIZE, lifo_fiber3, (int) &i,
		    NUMBER_OF_LOOPS, 3, 0);

    t = TIME_STAMP_DELTA_GET (t);

    return_value += check_result (i, t);

    /* fiber contexts have done their job, they can stop now safely: */
    for (j = 0; j < 2; j++)
	nano_task_fifo_put (&nanoFifo_sync, (void *) element);

    /* test get wait & put fiber/task functions */
    fprintf (output_file, sz_test_case_fmt,
	     "LIFO channel - 'nano_task_lifo_get_wait'");
    fprintf (output_file, sz_description,
	     "testing 'nano_lifo_init','nano_fiber_lifo_get_wait',"
	     " 'nano_fiber_lifo_put',\n");
    fprintf (output_file, "\t'nano_task_lifo_get_wait', 'nano_task_lifo_put' functions;");
    printf (sz_test_start_fmt, "'nano_task_lifo_get_wait'");

    lifo_test_init ();

    t = BENCH_START ();

    task_fiber_start (fiber_stack1, STACK_SIZE, lifo_fiber1, 0,
		    NUMBER_OF_LOOPS, 3, 0);
    for (i = 0; i < NUMBER_OF_LOOPS / 2; i++) {
	int element[2];
	int *	pelement;
	element[1] = 2 * i;
	nano_task_lifo_put (&nanoLifo1, element);
	element[1] = 2 * i + 1;
	nano_task_lifo_put (&nanoLifo1, element);

	pelement = (int *) nano_task_lifo_get_wait (&nanoLifo2);
	if (pelement[1] != 2 * i + 1)
	    break;
	pelement = (int *) nano_task_lifo_get_wait (&nanoLifo2);
	if (pelement[1] != 2 * i)
	    break;
	}

    t = TIME_STAMP_DELTA_GET (t);

    return_value += check_result (i * 2, t);

    /* fiber contexts have done their job, they can stop now safely: */
    for (j = 0; j < 2; j++)
	nano_task_fifo_put (&nanoFifo_sync, (void *) element);

    return return_value;
    }
