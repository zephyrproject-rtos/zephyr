/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_SHARED_H
#define APP_SHARED_H

#include <zephyr/kernel.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/sys/sys_heap.h>

#include "sample_driver.h"

#define BLK_SIZE (SAMPLE_DRIVER_MSG_SIZE + sizeof(void *))

#define HEAP_BYTES (BLK_SIZE * 16)

extern struct k_mem_partition shared_partition;

extern struct sys_heap shared_pool;
extern uint8_t shared_pool_mem[HEAP_BYTES];
extern struct k_queue shared_queue_incoming;
extern struct k_queue shared_queue_outgoing;

#define NUM_LOOPS	10

#endif
