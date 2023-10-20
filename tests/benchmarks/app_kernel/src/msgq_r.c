/* msgq_r.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "receiver.h"
#include "master.h"

/* message queue transfer speed test */

static BENCH_BMEM char buffer[192];

/**
 * @brief Data receive task
 */
void dequtask(void)
{
	int i;

	for (i = 0; i < NR_OF_MSGQ_RUNS; i++) {
		k_msgq_get(&DEMOQX1, buffer, K_FOREVER);
	}

	for (i = 0; i < NR_OF_MSGQ_RUNS; i++) {
		k_msgq_get(&DEMOQX4, buffer, K_FOREVER);
	}

	for (i = 0; i < NR_OF_MSGQ_RUNS; i++) {
		k_msgq_get(&DEMOQX192, buffer, K_FOREVER);
	}
}
