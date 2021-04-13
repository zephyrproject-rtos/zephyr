/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PROD_CONSMUER_H
#define PROD_CONSMUER_H

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


void app_a_entry(void *p1, void *p2, void *p3);

extern struct k_mem_partition app_a_partition;
#define APP_A_DATA      K_APP_DMEM(app_a_partition)
#define APP_A_BSS       K_APP_BMEM(app_a_partition)


void app_b_entry(void *p1, void *p2, void *p3);

extern struct k_mem_partition app_b_partition;
#define APP_B_DATA      K_APP_DMEM(app_b_partition)
#define APP_B_BSS       K_APP_BMEM(app_b_partition)

#endif
