/* nano_int_lock_unlock.c - measure time for interrupts lock and unlock */

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
 * DESCRIPTION
 * This file contains test that measures average time needed to do a call
 * to lock the interrupt lock and a call to unlock the interrupts. Typically
 * users calls both of these functions to ensure interrupts are lock while
 * some code executes. No explicit interrupts are generated during the test
 * so the interrupt handler does not run.
 */

#include "timestamp.h"
#include "utils.h"

#include <nanokernel/cpu.h>

/* total number of interrupt lock/unlock cycles */
#define NTESTS 100000

static uint32_t timestamp = 0;

/*******************************************************************************
 *
 * nanoIntLockUnlock - the test main function
 *
 * RETURNS: 0 on success
 *
 * \NOMANUAL
 */

int nanoIntLockUnlock (void)
    {
    int i;
    unsigned int mask;

    PRINT_FORMAT (" 5- Measure average time to lock then unlock interrupts");
    PRINT_FORMAT (" 5.1- When each lock and unlock is executed as a function"
		  " call");
    bench_test_start ();
    timestamp = TIME_STAMP_DELTA_GET (0);
    for (i = 0; i < NTESTS; i++)
	{
	mask = irq_lock ();
	irq_unlock (mask);
	}
    timestamp = TIME_STAMP_DELTA_GET (timestamp);
    if (bench_test_end () == 0)
	{
	PRINT_FORMAT (" Average time for lock then unlock "
		      "is %lu tcs = %lu nsec",
		      timestamp / NTESTS, SYS_CLOCK_HW_CYCLES_TO_NS_AVG (timestamp, NTESTS));
	}
    else
	{
	errorCount++;
	PRINT_OVERFLOW_ERROR ();
	}

    PRINT_FORMAT (" ");
    PRINT_FORMAT (" 5.2- When each lock and unlock is executed as inline"
		  " function call");
    bench_test_start ();
    timestamp = TIME_STAMP_DELTA_GET (0);
    for (i = 0; i < NTESTS; i++)
	{
	mask = irq_lock_inline ();
	irq_unlock_inline (mask);
	}
    timestamp = TIME_STAMP_DELTA_GET (timestamp);
    if (bench_test_end () == 0)
	{
	PRINT_FORMAT (" Average time for lock then unlock "
		      "is %lu tcs = %lu nsec",
		      timestamp / NTESTS, SYS_CLOCK_HW_CYCLES_TO_NS_AVG (timestamp, NTESTS));
	}
    else
	{
	errorCount++;
	PRINT_OVERFLOW_ERROR ();
	}
    return 0;
    }
