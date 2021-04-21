/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MAIN_H
#define MAIN_H

#include <kernel.h>
#include <app_memory/app_memdomain.h>
#include <sys/sys_heap.h>

extern struct k_mem_partition shared_partition;
#define SHARED_DATA	K_APP_DMEM(shared_partition)
#define SHARED_BSS	K_APP_BMEM(shared_partition)

extern struct sys_heap shared_pool;
extern struct k_queue shared_queue_incoming;
extern struct k_queue shared_queue_outgoing;

#define NUM_LOOPS	10

#endif
