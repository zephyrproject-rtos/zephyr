/* main.c - main testing module */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
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
 * This file contains the main testing module that invokes all the tests.
 */

#include "timestamp.h"
#include "utils.h"
#include <tc_util.h>

#include <nanokernel/cpu.h>

uint32_t tm_off; /* time necessary to read the time */
int errorCount = 0; /* track number of errors */

/*******************************************************************************
 *
 * nanoTest - test latency of nanokernel
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void nanoTest (void)
	{
	PRINT_NANO_BANNER ();
	PRINT_TIME_BANNER();

	nanoIntLatency ();
	printDashLine ();

	nanoIntToFiber ();
	printDashLine ();

	nanoIntToFiberSem ();
	printDashLine ();

	nanoCtxSwitch ();
	printDashLine ();

	nanoIntLockUnlock ();
	printDashLine ();
	}

#ifdef CONFIG_NANOKERNEL
/*******************************************************************************
 *
 * main - nanokernel-only testing entry point
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void main (void)
	{
	bench_test_init ();

	nanoTest ();

	PRINT_END_BANNER ();
	TC_END_REPORT (errorCount);
	}

#else

int microIntToTaskEvt (void);
int microIntToTask (void);
int microSemaLockUnlock (void);
int microMutexLockUnlock (void);
void microTaskSwitchYield (void);

/*******************************************************************************
 *
 * microTest - test latency of microkernel
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void microTest (void)
	{
	PRINT_MICRO_BANNER ();
	PRINT_TIME_BANNER();

	microIntToTask ();
	printDashLine ();

	microIntToTaskEvt ();
	printDashLine ();

	microSemaLockUnlock ();
	printDashLine ();

	microMutexLockUnlock ();
	printDashLine ();

	microTaskSwitchYield ();
	printDashLine ();
	}

/*******************************************************************************
 *
 * main - microkernel testing entry point
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void microMain (void)
	{
	bench_test_init ();

	nanoTest ();
	microTest ();

	PRINT_END_BANNER ();
	TC_END_REPORT (errorCount);
	}
#endif /* CONFIG_NANOKERNEL */
