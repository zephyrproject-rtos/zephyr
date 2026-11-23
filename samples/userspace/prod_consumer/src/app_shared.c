/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_shared.h"

/* Define the shared partition, which will contain a memory region that
 * will be accessible by both applications A and B.
 */
K_APPMEM_PARTITION_DEFINE(shared_partition);

/* Define a memory pool to place in the shared area.
 */
K_APP_DMEM(shared_partition) struct sys_heap shared_pool;
K_APP_DMEM(shared_partition) uint8_t shared_pool_mem[HEAP_BYTES];

/* queues for exchanging data between App A and App B */
K_QUEUE_DEFINE(shared_queue_incoming);
K_QUEUE_DEFINE(shared_queue_outgoing);
