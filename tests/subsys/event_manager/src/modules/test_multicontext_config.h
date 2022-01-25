/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* TEST_MULTICONTEXT */

#include <zephyr.h>

#define THREAD1_PRIORITY K_PRIO_COOP(1)
#define THREAD2_PRIORITY K_PRIO_COOP(2)

enum source_id {
	SOURCE_T1,
	SOURCE_T2,
	SOURCE_ISR,

	SOURCE_CNT
};
