/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <ztest.h>

#define SYNC_BARRIER_SEMAPHORE_INIT_COUNT (0)
#define SYNC_BARRIER_SEMAPHORE_MAX_COUNT (1)

K_SEM_DEFINE(sync_sem,
	     SYNC_BARRIER_SEMAPHORE_INIT_COUNT,
	     SYNC_BARRIER_SEMAPHORE_MAX_COUNT);

K_SEM_DEFINE(barrier_sem,
	     SYNC_BARRIER_SEMAPHORE_INIT_COUNT,
	     SYNC_BARRIER_SEMAPHORE_MAX_COUNT);

ZTEST_BMEM bool valid_fault;

void _SysFatalErrorHandler(unsigned int reason, const NANO_ESF *pEsf)
{
	printk("Caught system error -- reason %d %d\n", reason, valid_fault);
	if (valid_fault) {
		valid_fault = false; /* reset back to normal */
		ztest_test_pass();
	} else {
		ztest_test_fail();
	}

#if !(defined(CONFIG_ARM) || defined(CONFIG_ARC))
	CODE_UNREACHABLE;
#endif
}
