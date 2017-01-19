/* main.c - main testing module */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DESCRIPTION
 * This file contains the main testing module that invokes all the tests.
 */

#include "timestamp.h"
#include "utils.h"
#include <tc_util.h>

#include <arch/cpu.h>

uint32_t tm_off; /* time necessary to read the time */
int errorCount; /* track number of errors */

/**
 *
 * @brief Test latency of nanokernel
 *
 * @return N/A
 */
void nanoTest(void)
{
	PRINT_NANO_BANNER();
	PRINT_TIME_BANNER();

	nanoIntLatency();
	printDashLine();

	nanoIntToFiber();
	printDashLine();

	nanoIntToFiberSem();
	printDashLine();

	nanoCtxSwitch();
	printDashLine();

	nanoIntLockUnlock();
	printDashLine();
}

int microIntToTaskEvt(void);
int microIntToTask(void);
int microSemaLockUnlock(void);
int microMutexLockUnlock(void);
void microTaskSwitchYield(void);

/**
 *
 * @brief Test latency of microkernel
 *
 * @return N/A
 */
void microTest(void)
{
	PRINT_MICRO_BANNER();
	PRINT_TIME_BANNER();

	microIntToTask();
	printDashLine();

	microIntToTaskEvt();
	printDashLine();

	microSemaLockUnlock();
	printDashLine();

	microMutexLockUnlock();
	printDashLine();

	microTaskSwitchYield();
	printDashLine();
}

/**
 *
 * @brief Microkernel testing entry point
 *
 * @return N/A
 */
void microMain(void)
{
	bench_test_init();

	nanoTest();
	microTest();

	PRINT_END_BANNER();
	TC_END_REPORT(errorCount);
}
