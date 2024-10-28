/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __THREAD_MANAGEMENT_H
#define __THREAD_MANAGEMENT_H

#include <zephyr/kernel.h>

#define THR_COUNT    50
#define THR_DATA_LEN 200
#define THR_PRIORITY 5

struct thr_dynamic {
	struct k_thread thread;
	int id;
	int delay;
	int wasCreated;
	int wasHandled;
};

extern void thr_entry_point(void *arg1, void *arg2, void *arg3);
extern void thr_create(void);
extern void thr_join_all(void);
extern void thr_summary(void);

#endif
